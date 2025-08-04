#include "serial/kml_generator.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <iomanip>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/LocalCartesian.hpp>

#include "config.h"
#include "antenna/antenna_factory.h"
#include "core/logging.h"
#include "core/parameters.h"
#include "core/world.h"
#include "math/coord.h"
#include "math/path.h"
#include "radar/platform.h"
#include "radar/radar_obj.h"
#include "radar/receiver.h"
#include "radar/target.h"
#include "radar/transmitter.h"

// TODO: Numerous magic numbers are used in this file, such as the number of divisions for the waypoint path
// TODO: Assumes that every simulation will use local Cartesian coordinates centered at the origin.
//		However, because FERS doesn't care what your coordinate system is (provided it is in meters and xyz),
//		this is not a problem for FERS. But for a KML visualizer, the coordinate system used while creating a scenario
//		is extremely important.

namespace
{
	using namespace std;

	double sincAntennaGain(const double theta, const double alpha, const double beta, const double gamma)
	{
		if (theta == 0.0) { return alpha; }
		const double gain = alpha * std::pow(std::sin(beta * theta) / (beta * theta), gamma);
		return gain;
	}

	double find3DbDropAngle(const double alpha, const double beta, const double gamma)
	{
		constexpr int num_points = 1000;
		std::vector<double> theta(num_points);
		std::vector<double> gain(num_points);
		for (int i = 0; i < num_points; ++i)
		{
			theta[i] = -PI + 2.0 * PI * i / (num_points - 1);
			gain[i] = sincAntennaGain(theta[i], alpha, beta, gamma);
		}
		const double max_gain = *std::max_element(gain.begin() + num_points / 2, gain.end());
		const double max_gain_db = 10.0 * std::log10(max_gain);
		const double target_gain_db = max_gain_db - 3.0;
		double target_gain = std::pow(10.0, target_gain_db / 10.0);
		const int idx = std::distance
			(
				gain.begin() + num_points / 2,
				std::min_element
				(
					gain.begin() + num_points / 2, gain.end(), [target_gain](const double a, const double b)
					{
						return std::abs(a - target_gain) < std::abs(b - target_gain);
					}
					)
				);
		const double angle_3db_drop = theta[idx + num_points / 2];
		return angle_3db_drop * 180.0 / PI;
	}

	std::string getCoordinatesFromPositionWaypoint(const math::Coord& coord, const GeographicLib::LocalCartesian& proj)
	{
		// The 'altitude' from XML is treated as the local ENU 'up' coordinate (z_enu).
		// GeographicLib::LocalCartesian handles the conversion to absolute MSL altitude
		// based on the origin's reference altitude.
		double lat, lon, alt_abs;
		proj.Reverse(coord.pos.x, coord.pos.y, coord.pos.z, lat, lon, alt_abs);

		std::stringstream coordinates;
		coordinates << std::fixed << std::setprecision(6) << lon << "," << lat << "," << alt_abs;
		return coordinates.str();
	}

	std::string getCoordinatesFromPosition(const math::Vec3& pos, const GeographicLib::LocalCartesian& proj)
	{
		return getCoordinatesFromPositionWaypoint({pos, 0.0}, proj);
	}

	void calculateDestinationCoordinate
	(
		const double startLatitude, const double startLongitude, const double angle, const double distance,
		double& destLatitude, double& destLongitude
		)
	{
		const GeographicLib::Geodesic& geod = GeographicLib::Geodesic::WGS84();
		geod.Direct(startLatitude, startLongitude, angle, distance, destLatitude, destLongitude);
	}

	std::vector<std::pair<double, double>> generateCircleCoordinates
	(
		const double lat, const double lon, const double radius_km,
		const int num_points = 100
		)
	{
		std::vector<std::pair<double, double>> circle_coordinates;
		for (int i = 0; i < num_points; i++)
		{
			const double bearing = i * 360.0 / num_points;
			double new_lat, new_lon;
			calculateDestinationCoordinate(lat, lon, bearing, radius_km * 1000.0, new_lat, new_lon);
			circle_coordinates.emplace_back(new_lat, new_lon);
		}
		return circle_coordinates;
	}

	void processPlatform
	(
		const radar::Platform* platform,
		const std::vector<const radar::Object*>& objectsOnPlatform,
		std::ofstream& kmlFile,
		const GeographicLib::LocalCartesian& proj,
		const double referenceAltitude
		)
	{
		const math::Path* path = platform->getMotionPath();
		if (path->getCoords().empty()) { return; }

		const auto& [first_wp_pos, first_wp_t] = path->getCoords().front();
		double x = first_wp_pos.x;
		double y = first_wp_pos.y;
		double altitude = first_wp_pos.z;
		double longitude, latitude, alt_abs;
		proj.Reverse(x, y, altitude, latitude, longitude, alt_abs);

		const auto interp_type = path->getType();
		const bool is_static = interp_type == math::Path::InterpType::INTERP_STATIC;
		const bool is_linear = interp_type == math::Path::InterpType::INTERP_LINEAR;
		const bool is_cubic = interp_type == math::Path::InterpType::INTERP_CUBIC;

		// Determine icon style based on objects on the platform
		string placemark_style = "#target"; // Default
		const radar::Radar* radar_obj = nullptr;
		bool has_receiver = false;
		bool has_transmitter = false;
		for (const auto* obj : objectsOnPlatform)
		{
			if (dynamic_cast<const radar::Receiver*>(obj)) { has_receiver = true; }
			if (dynamic_cast<const radar::Transmitter*>(obj)) { has_transmitter = true; }
			if (const auto r = dynamic_cast<const radar::Radar*>(obj)) { if (!radar_obj) { radar_obj = r; } }
		}

		if (has_receiver) { placemark_style = "#receiver"; }
		else if (has_transmitter) { placemark_style = "#transmitter"; }

		bool is_isotropic = false;
		const antenna::Sinc* sinc_antenna = nullptr;
		if (radar_obj)
		{
			if (const antenna::Antenna* ant = radar_obj->getAntenna(); dynamic_cast<const antenna::Isotropic*>(ant))
			{
				is_isotropic = true;
			}
			else if (const auto* sinc_ant = dynamic_cast<const antenna::Sinc*>(ant)) { sinc_antenna = sinc_ant; }
		}

		if (is_isotropic)
		{
			constexpr double circle_radius = 20;
			constexpr int num_points = 100;
			proj.Reverse(x, y, altitude, latitude, longitude, alt_abs);

			// TODO: UNUSED
			//std::string placemarkStyle;
			//if (element.childElement("receiver", 0).isValid()) { placemarkStyle = "receiver"; }
			//else if (element.childElement("transmitter", 0).isValid()) { placemarkStyle = "transmitter"; }
			//else if (element.childElement("target", 0).isValid()) { placemarkStyle = "target"; }

			kmlFile << "<Placemark>\n";
			kmlFile << "    <name>Isotropic pattern range</name>\n";
			kmlFile << "    <styleUrl>#translucentPolygon</styleUrl>\n";
			std::vector<std::pair<double, double>> circle_coordinates =
				generateCircleCoordinates(latitude, longitude, circle_radius, num_points);
			kmlFile << "    <Polygon>\n";
			kmlFile << "        <extrude>1</extrude>\n";
			kmlFile << "        <altitudeMode>absolute</altitudeMode>\n";
			kmlFile << "        <outerBoundaryIs>\n";
			kmlFile << "            <LinearRing>\n";
			kmlFile << "                <coordinates>\n";
			for (const auto& [fst, snd] : circle_coordinates)
			{
				kmlFile << "                    " << snd << "," << fst << "," <<
					alt_abs << "\n";
			}
			kmlFile << "                    " << circle_coordinates[0].second << "," << circle_coordinates[0].first
				<< ","
				<< alt_abs << "\n";
			kmlFile << "                </coordinates>\n";
			kmlFile << "            </LinearRing>\n";
			kmlFile << "        </outerBoundaryIs>\n";
			kmlFile << "    </Polygon>\n";
			kmlFile << "</Placemark>\n";
		}
		else if (sinc_antenna)
		{
			math::SVec3 initial_rotation = platform->getRotationPath()->getPosition(params::startTime());
			double start_azimuth_rad = initial_rotation.azimuth;

			string coordinates = getCoordinatesFromPosition(first_wp_pos, proj);
			constexpr double arrow_length = 20000;
			double start_latitude, start_longitude, start_altitude;
			std::istringstream coordinates_stream(coordinates);
			coordinates_stream >> start_longitude;
			coordinates_stream.ignore(1);
			coordinates_stream >> start_latitude;
			coordinates_stream.ignore(1);
			coordinates_stream >> start_altitude;
			double start_azimuth_deg_kml = (start_azimuth_rad * 180.0 / PI) + 180;
			double dest_latitude, dest_longitude;
			calculateDestinationCoordinate
				(
					start_latitude, start_longitude, start_azimuth_deg_kml, arrow_length, dest_latitude,
					dest_longitude
					);
			std::stringstream end_coordinates_stream;
			end_coordinates_stream << std::fixed << std::setprecision
				(6) << dest_longitude << "," << dest_latitude << ","
				<< start_altitude;
			std::string end_coordinates = end_coordinates_stream.str();
			const double angle_3d_b_drop_deg = find3DbDropAngle
				(
					sinc_antenna->getAlpha(), sinc_antenna->getBeta(),
					sinc_antenna->getGamma()
					);
			const double side_line1_azimuth = start_azimuth_deg_kml - angle_3d_b_drop_deg;
			const double side_line2_azimuth = start_azimuth_deg_kml + angle_3d_b_drop_deg;
			double side_line_1dest_latitude, side_line_1dest_longitude;
			double side_line_2dest_latitude, side_line_2dest_longitude;
			calculateDestinationCoordinate
				(
					start_latitude, start_longitude, side_line1_azimuth, arrow_length,
					side_line_1dest_latitude, side_line_1dest_longitude
					);
			calculateDestinationCoordinate
				(
					start_latitude, start_longitude, side_line2_azimuth, arrow_length,
					side_line_2dest_latitude, side_line_2dest_longitude
					);
			std::stringstream side_line1_end_coordinates_stream, side_line2_end_coordinates_stream;
			side_line1_end_coordinates_stream << std::fixed << std::setprecision
				(6) << side_line_1dest_longitude << ","
				<< side_line_1dest_latitude << "," << start_altitude;
			side_line2_end_coordinates_stream << std::fixed << std::setprecision
				(6) << side_line_2dest_longitude << ","
				<< side_line_2dest_latitude << "," << start_altitude;
			std::string side_line1_end_coordinates = side_line1_end_coordinates_stream.str();
			std::string side_line2_end_coordinates = side_line2_end_coordinates_stream.str();
			for (int i = 1; i <= 2; ++i)
			{
				std::string side_line_name = "Antenna Side Line " + std::to_string(i);
				std::string side_line_end_coordinates = i == 1
					? side_line1_end_coordinates
					: side_line2_end_coordinates;
				kmlFile << "<Placemark>\n";
				kmlFile << "      <name>" << side_line_name << "</name>\n";
				kmlFile << "      <styleUrl>#lineStyleBlue</styleUrl>\n";
				kmlFile << "      <LineString>\n";
				kmlFile << "            <altitudeMode>absolute</altitudeMode>\n";
				kmlFile << "            <tessellate>1</tessellate>\n";
				kmlFile << "            <coordinates>\n";
				kmlFile << "            " + coordinates + " " + side_line_end_coordinates + "\n";
				kmlFile << "            </coordinates>\n";
				kmlFile << "      </LineString>\n";
				kmlFile << "</Placemark>\n";
			}
			kmlFile << "<Placemark>\n";
			kmlFile << "      <name>Antenna Direction</name>\n";
			kmlFile << "      <styleUrl>#lineStyle</styleUrl>\n";
			kmlFile << "      <LineString>\n";
			kmlFile << "            <altitudeMode>absolute</altitudeMode>\n";
			kmlFile << "            <tessellate>1</tessellate>\n";
			kmlFile << "            <coordinates>\n";
			kmlFile << "            " + coordinates + " " + end_coordinates + "\n";
			kmlFile << "            </coordinates>\n";
			kmlFile << "      </LineString>\n";
			kmlFile << "</Placemark>\n";
			kmlFile << "<Placemark>\n";
			kmlFile << "      <name>Antenna Arrow</name>\n";
			kmlFile << "      <styleUrl>#arrowStyle</styleUrl>\n";
			kmlFile << "      <Point>\n";
			kmlFile << "          <coordinates>" + end_coordinates + "</coordinates>\n";
			kmlFile << "          <altitudeMode>absolute</altitudeMode>\n";
			kmlFile << "      </Point>\n";
			kmlFile << "      <IconStyle>\n";
			kmlFile << "          <heading>" << start_azimuth_deg_kml << "</heading>\n";
			kmlFile << "      </IconStyle>\n";
			kmlFile << "</Placemark>\n";
		}
		kmlFile << "<Placemark>\n";
		kmlFile << "    <name>" << platform->getName() << "</name>\n";
		kmlFile << "    <styleUrl>" << placemark_style << "</styleUrl>\n";
		if (is_linear || is_cubic)
		{
			kmlFile << "    <gx:Track>\n";
			// The altitude from the XML is a relative offset (local z). GeographicLib converts this
			// to an absolute MSL altitude for KML rendering.
			kmlFile << "        <altitudeMode>absolute</altitudeMode>\n";
			if (alt_abs > referenceAltitude) { kmlFile << "        <extrude>1</extrude>\n"; }

			if (const auto& waypoints = path->getCoords(); !waypoints.empty())
			{
				if (waypoints.size() == 1 || is_static)
				{
					const auto& [pos, t] = waypoints[0];
					math::Vec3 p_local_pos = path->getPosition(t);
					double p_lon, p_lat, p_alt_abs;
					proj.Reverse
						(
							p_local_pos.x, p_local_pos.y, p_local_pos.z,
							p_lat, p_lon, p_alt_abs
							);
					kmlFile << "        <when>" << t << "</when>\n";
					kmlFile << "        <gx:coord>" << p_lon << " " << p_lat << " " << p_alt_abs << "</gx:coord>\n";
				}
				else
				{
					for (size_t i = 0; i < waypoints.size() - 1; ++i)
					{
						const auto& [start_pos, start_t] = waypoints[i];
						const auto& [end_pos, end_t] = waypoints[i + 1];
						const double start_time = start_t;
						const double end_time = end_t;
						const double time_diff = end_time - start_time;
						if (time_diff <= 0)
						{
							if (i == 0)
							{
								math::Vec3 p_local_pos = path->getPosition(start_time);
								double p_lon, p_lat, p_alt_abs;
								proj.Reverse
									(
										p_local_pos.x, p_local_pos.y, p_local_pos.z, p_lat,
										p_lon,
										p_alt_abs
										);
								kmlFile << "        <when>" << start_time << "</when>\n";
								kmlFile << "        <gx:coord>" << p_lon << " " << p_lat << " " << p_alt_abs <<
									"</gx:coord>\n";
							}
							continue;
						}
						constexpr int num_divisions = 100;
						for (int j = 0; j < num_divisions; ++j)
						{
							const double t_ratio = static_cast<double>(j) / num_divisions;
							const double current_time = start_time + t_ratio * time_diff;
							math::Vec3 p_local_pos = path->getPosition(current_time);
							double p_lon, p_lat, p_alt_abs;
							proj.Reverse
								(
									p_local_pos.x, p_local_pos.y, p_local_pos.z, p_lat, p_lon,
									p_alt_abs
									);
							kmlFile << "        <when>" << current_time << "</when>\n";
							kmlFile << "        <gx:coord>" << p_lon << " " << p_lat << " " << p_alt_abs <<
								"</gx:coord>\n";
						}
					}
					const auto& [pos, t] = waypoints.back();
					math::Vec3 p_local_pos = path->getPosition(t);
					double p_lon, p_lat, p_alt_abs;
					proj.Reverse
						(
							p_local_pos.x, p_local_pos.y, p_local_pos.z, p_lat, p_lon, p_alt_abs
							);
					kmlFile << "        <when>" << t << "</when>\n";
					kmlFile << "        <gx:coord>" << p_lon << " " << p_lat << " " << p_alt_abs << "</gx:coord>\n";
				}
			}
			kmlFile << "    </gx:Track>\n";
		}
		else // Not a gx:Track compatible path, just a single point
		{
			kmlFile << "    <LookAt>\n";
			kmlFile << "        <longitude>" << longitude << "</longitude>\n";
			kmlFile << "        <latitude>" << latitude << "</latitude>\n";
			kmlFile << "        <altitude>" << alt_abs << "</altitude>\n";
			kmlFile << "        <heading>-148.4122922628044</heading>\n";
			kmlFile << "        <tilt>40.5575073395506</tilt>\n";
			kmlFile << "        <range>500.6566641072245</range>\n";
			kmlFile << "    </LookAt>\n";
			kmlFile << "    <Point>\n";
			kmlFile << "        <coordinates>" << longitude << "," << latitude << "," << alt_abs <<
				"</coordinates>\n";
			kmlFile << "        <altitudeMode>absolute</altitudeMode>\n";
			if (alt_abs > referenceAltitude) { kmlFile << "        <extrude>1</extrude>\n"; }
			kmlFile << "    </Point>\n";
		}
		kmlFile << "</Placemark>\n";
		if (is_linear || is_cubic)
		{
			const math::Coord& start_coord = path->getCoords().front();
			const math::Coord& end_coord = path->getCoords().back();
			std::string start_coordinates = getCoordinatesFromPositionWaypoint(start_coord, proj);
			std::string end_coordinates = getCoordinatesFromPositionWaypoint(end_coord, proj);

			// Extract absolute altitude from the coordinate string for the extrude check
			double start_altitude_abs, end_altitude_abs;
			try
			{
				start_altitude_abs = std::stod(start_coordinates.substr(start_coordinates.rfind(',') + 1));
				end_altitude_abs = std::stod(end_coordinates.substr(end_coordinates.rfind(',') + 1));
			}
			catch (const std::exception& e)
			{
				LOG(logging::Level::WARNING, "Could not parse altitude from coordinate string: {}", e.what());
				start_altitude_abs = end_altitude_abs = referenceAltitude; // Default to reference altitude on error
			}

			kmlFile << "<Placemark>\n";
			kmlFile << "    <name>Start: " << platform->getName() << "</name>\n";
			kmlFile << "    <styleUrl>#target</styleUrl>\n";
			kmlFile << "    <Point>\n";
			kmlFile << "        <coordinates>" << start_coordinates << "</coordinates>\n";
			kmlFile << "        <altitudeMode>absolute</altitudeMode>\n";
			if (start_altitude_abs > referenceAltitude) { kmlFile << "        <extrude>1</extrude>\n"; }
			kmlFile << "    </Point>\n";
			kmlFile << "</Placemark>\n";
			kmlFile << "<Placemark>\n";
			kmlFile << "    <name>End: " << platform->getName() << "</name>\n";
			kmlFile << "    <styleUrl>#target</styleUrl>\n";
			kmlFile << "    <Point>\n";
			kmlFile << "        <coordinates>" << end_coordinates << "</coordinates>\n";
			kmlFile << "        <altitudeMode>absolute</altitudeMode>\n";
			if (end_altitude_abs > referenceAltitude) { kmlFile << "        <extrude>1</extrude>\n"; }
			kmlFile << "    </Point>\n";
			kmlFile << "</Placemark>\n";
		}
	}
}

namespace serial
{
	bool KmlGenerator::generateKml(const core::World& world, const std::string& outputKmlPath)
	{
		try
		{
			const double reference_latitude = params::originLatitude();
			const double reference_longitude = params::originLongitude();
			const double reference_altitude = params::originAltitude();

			GeographicLib::LocalCartesian proj(reference_latitude, reference_longitude, reference_altitude);

			// Group objects by their platform
			map<const radar::Platform*, vector<const radar::Object*>> platform_to_objects;
			const auto group_objects = [&](const auto& objectCollection)
			{
				for (const auto& obj_ptr : objectCollection)
				{
					platform_to_objects[obj_ptr->getPlatform()].push_back(obj_ptr.get());
				}
			};

			group_objects(world.getReceivers());
			group_objects(world.getTransmitters());
			group_objects(world.getTargets());

			std::ofstream kml_file(outputKmlPath.c_str());
			if (!kml_file.is_open())
			{
				LOG(logging::Level::ERROR, "Error opening output KML file {}", outputKmlPath);
				return false;
			}
			kml_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
			kml_file <<
				"<kml xmlns=\"http://www.opengis.net/kml/2.2\" xmlns:gx=\"http://www.google.com/kml/ext/2.2\">\n";
			kml_file << "<Document>\n\n";
			kml_file << "<name>FERS Simulation Visualization</name>\n";
			kml_file << "<Style id=\"receiver\">\n";
			kml_file << "  <IconStyle>\n";
			kml_file << "    <Icon>\n";
			kml_file << "      <href>https://cdn-icons-png.flaticon.com/512/645/645436.png</href>\n";
			kml_file << "    </Icon>\n";
			kml_file << "  </IconStyle>\n";
			kml_file << "</Style>\n";
			kml_file << "<Style id=\"transmitter\">\n";
			kml_file << "  <IconStyle>\n";
			kml_file << "    <Icon>\n";
			kml_file << "      <href>https://cdn-icons-png.flaticon.com/128/224/224666.png</href>\n";
			kml_file << "    </Icon>\n";
			kml_file << "  </IconStyle>\n";
			kml_file << "</Style>\n";
			kml_file << "<Style id=\"target\">\n";
			kml_file << "  <IconStyle>\n";
			kml_file << "    <Icon>\n";
			kml_file <<
				"      <href>https://upload.wikimedia.org/wikipedia/commons/thumb/a/ad/Target_red_dot1.svg/1200px-Target_red_dot1.svg.png</href>\n";
			kml_file << "    </Icon>\n";
			kml_file << "  </IconStyle>\n";
			kml_file << "  <LineStyle>\n";
			kml_file << "    <width>2</width>\n";
			kml_file << "  </LineStyle>\n";
			kml_file << "</Style>\n";
			kml_file << "<Style id=\"translucentPolygon\">\n";
			kml_file << "    <LineStyle>\n";
			kml_file << "        <color>ff0000ff</color>\n";
			kml_file << "        <width>2</width>\n";
			kml_file << "    </LineStyle>\n";
			kml_file << "    <PolyStyle>\n";
			kml_file << "        <color>00ffffff</color> <!-- RGBA: 50% transparent white --> \n";
			kml_file << "     </PolyStyle>\n";
			kml_file << "</Style>\n";
			kml_file << "<Style id=\"arrowStyle\">\n";
			kml_file << "    <IconStyle>\n";
			kml_file << "        <Icon>\n";
			kml_file << "            <href>http://maps.google.com/mapfiles/kml/shapes/arrow.png</href>\n";
			kml_file << "        </Icon>\n";
			kml_file << "        <scale>0.5</scale>\n";
			kml_file << "    </IconStyle>\n";
			kml_file << "</Style>\n";
			kml_file << "<Style id=\"lineStyle\">\n";
			kml_file << "    <LineStyle>\n";
			kml_file << "        <color>ff0000ff</color>\n";
			kml_file << "        <width>2</width>\n";
			kml_file << "     </LineStyle>\n";
			kml_file << "</Style>\n";
			kml_file << "<Style id=\"lineStyleBlue\">\n";
			kml_file << "    <LineStyle>\n";
			kml_file << "        <color>ffff0000</color>\n";
			kml_file << "        <width>2</width>\n";
			kml_file << "     </LineStyle>\n";
			kml_file << "</Style>\n";
			kml_file << "<Folder>\n";
			kml_file << "  <name>Reference Coordinate</name>\n";
			kml_file <<
				"  <description>Placemarks for various elements in the FERSXML file. All Placemarks are situated relative to this reference point.</description>\n";
			kml_file << "  <LookAt>\n";
			kml_file << "    <longitude>" << reference_longitude << "</longitude>\n";
			kml_file << "    <latitude>" << reference_latitude << "</latitude>\n";
			kml_file << "    <altitude>" << reference_altitude << "</altitude>\n";
			kml_file << "    <heading>-148.4122922628044</heading>\n";
			kml_file << "    <tilt>40.5575073395506</tilt>\n";
			kml_file << "    <range>10000</range>\n";
			kml_file << "  </LookAt>\n";
			for (const auto& [platform, objects] : platform_to_objects)
			{
				processPlatform(platform, objects, kml_file, proj, reference_altitude);
			}
			kml_file << "</Folder>\n";
			kml_file << "</Document>\n";
			kml_file << "</kml>\n";
			kml_file.close();
		}
		catch (...)
		{
			LOG(logging::Level::ERROR, "Unknown error occurred while generating KML file.");
			return false;
		}
		return true;
	}
}
