/**
* @file kml_generator.cpp
* @brief Source file for KML file generation from FERS simulation scenarios.
*
* @authors David Young
* @date 2025-07-31
*/

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

// TODO: The KML generator assumes all scenarios use a local ENU tangent plane, which conflicts with FERS's
//       coordinate-system-agnostic design. This can lead to incorrect visualizations if a user designs a
//       scenario in a different frame (e.g., ECEF). Consider adding support for other common coordinate
//       systems or prominently documenting this limitation.

// TODO: KML visualization for antenna patterns is incomplete. This function only supports 'isotropic' and 'sinc'
//       antennas, while FERS supports many others (gaussian, parabolic, etc.). To provide a more complete
//       scenario preview, add symbolic representations for the other FERS antenna types.

namespace
{
	using namespace std;

	// --- Constants ---
	constexpr int TRACK_NUM_DIVISIONS = 100;

	constexpr int ISOTROPIC_PATTERN_POINTS = 100;

	constexpr double ISOTROPIC_PATTERN_RADIUS_KM = 20.0;

	constexpr double SINC_ARROW_LENGTH_M = 20000.0;

	// TODO: The visual size of this antenna beam is hardcoded to a symbolic 20km radius/length and has no
	//       connection to the antenna's actual performance (e.g., gain, power). To be more representative,
	//       the beam symbology could be scaled based on a calculated metric like the antenna's directivity or a
	//       nominal range.

	// --- Geodetic and Coordinate Helpers ---
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

	std::string formatCoordinates(const double lon, const double lat, const double alt)
	{
		std::stringstream ss;
		ss << std::fixed << std::setprecision(6) << lon << "," << lat << "," << alt;
		return ss.str();
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
		const double lat, const double lon, const double radius_km
		)
	{
		std::vector<std::pair<double, double>> circle_coordinates;
		for (int i = 0; i < ISOTROPIC_PATTERN_POINTS; i++)
		{
			const double bearing = i * 360.0 / ISOTROPIC_PATTERN_POINTS;
			double new_lat, new_lon;
			calculateDestinationCoordinate(lat, lon, bearing, radius_km * 1000.0, new_lat, new_lon);
			circle_coordinates.emplace_back(new_lat, new_lon);
		}
		return circle_coordinates;
	}

	/**
	 * @brief Converts FERS internal azimuth to KML heading.
	 *
	 * FERS uses an East-North-Up (ENU) coordinate system, where azimuth is measured
	 * in radians counter-clockwise from the East (+X) axis. This is confirmed by the
	 * SVec3(Vec3) constructor which uses `atan2(y, x)`.
	 *
	 * KML heading is specified in degrees, measured clockwise from North (0 is North,
	 * 90 is East, 180 is South, 270 is West).
	 *
	 * The conversion formula is: heading_kml_deg = 90.0 - azimuth_fers_deg.
	 * This function performs the conversion and normalizes the result to [0, 360).
	 *
	 * @param fersAzimuthRad The FERS azimuth in radians.
	 * @return The corresponding KML heading in degrees.
	 */
	double convertFersAzimuthToKmlHeading(const double fersAzimuthRad)
	{
		const double fersAzimuthDeg = fersAzimuthRad * 180.0 / PI;
		double kmlHeading = 90.0 - fersAzimuthDeg;
		// Normalize to [0, 360) range
		kmlHeading = std::fmod(kmlHeading, 360.0);
		if (kmlHeading < 0.0)
		{
			kmlHeading += 360.0;
		}
		return kmlHeading;
	}

	// --- KML Generation Helpers ---
	void writeKmlHeaderAndStyles(std::ofstream& kmlFile)
	{
		kmlFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
		kmlFile <<
			"<kml xmlns=\"http://www.opengis.net/kml/2.2\" xmlns:gx=\"http://www.google.com/kml/ext/2.2\">\n";
		kmlFile << "<Document>\n\n";
		if (params::params.simulation_name.empty())
		{
			kmlFile << "<name>FERS Simulation Visualization</name>\n";
		}
		else
		{
			kmlFile << "<name>" << params::params.simulation_name << "</name>\n";
		}
		kmlFile <<
			"<Style id=\"receiver\"><IconStyle><Icon><href>https://cdn-icons-png.flaticon.com/512/645/645436.png</href></Icon></IconStyle></Style>\n";
		kmlFile <<
			"<Style id=\"transmitter\"><IconStyle><Icon><href>https://cdn-icons-png.flaticon.com/128/224/224666.png</href></Icon></IconStyle></Style>\n";
		kmlFile <<
			"<Style id=\"target\"><IconStyle><Icon><href>https://upload.wikimedia.org/wikipedia/commons/thumb/a/ad/Target_red_dot1.svg/1200px-Target_red_dot1.svg.png</href></Icon></IconStyle><LineStyle><width>2</width></LineStyle></Style>\n";
		kmlFile <<
			"<Style id=\"translucentPolygon\"><LineStyle><color>ff0000ff</color><width>2</width></LineStyle><PolyStyle><color>00ffffff</color></PolyStyle></Style>\n";
		kmlFile <<
			"<Style id=\"arrowStyle\"><IconStyle><Icon><href>http://maps.google.com/mapfiles/kml/shapes/arrow.png</href></Icon><scale>0.5</scale></IconStyle></Style>\n";
		kmlFile << "<Style id=\"lineStyle\"><LineStyle><color>ff0000ff</color><width>2</width></LineStyle></Style>\n";
		kmlFile <<
			"<Style id=\"lineStyleBlue\"><LineStyle><color>ffff0000</color><width>2</width></LineStyle></Style>\n";
	}

	void writePoint
	(
		std::ofstream& kmlFile, const std::string& name, const std::string& styleUrl, const std::string& coordinates,
		const double objectAltitude, const double referenceAltitude
		)
	{
		kmlFile << "<Placemark>\n";
		kmlFile << "    <name>" << name << "</name>\n";
		kmlFile << "    <styleUrl>" << styleUrl << "</styleUrl>\n";
		kmlFile << "    <Point>\n";
		kmlFile << "        <coordinates>" << coordinates << "</coordinates>\n";
		kmlFile << "        <altitudeMode>absolute</altitudeMode>\n";
		if (objectAltitude > referenceAltitude) { kmlFile << "        <extrude>1</extrude>\n"; }
		kmlFile << "    </Point>\n";
		kmlFile << "</Placemark>\n";
	}

	void writeAntennaBeamLine
	(
		std::ofstream& kmlFile, const std::string& name, const std::string& style, const std::string& startCoords,
		const std::string& endCoords
		)
	{
		kmlFile << "<Placemark>\n";
		kmlFile << "    <name>" << name << "</name>\n";
		kmlFile << "    <styleUrl>" << style << "</styleUrl>\n";
		kmlFile << "    <LineString>\n";
		kmlFile << "        <altitudeMode>absolute</altitudeMode>\n";
		kmlFile << "        <tessellate>1</tessellate>\n";
		kmlFile << "        <coordinates>" << startCoords << " " << endCoords << "</coordinates>\n";
		kmlFile << "    </LineString>\n";
		kmlFile << "</Placemark>\n";
	}

	// --- Platform Processing Logic ---
	std::string getPlacemarkStyleForPlatform(const std::vector<const radar::Object*>& objects)
	{
		bool has_receiver = false;
		bool has_transmitter = false;
		for (const auto* obj : objects)
		{
			if (dynamic_cast<const radar::Receiver*>(obj)) { has_receiver = true; }
			if (dynamic_cast<const radar::Transmitter*>(obj)) { has_transmitter = true; }
		}

		if (has_receiver) { return "#receiver"; }
		if (has_transmitter) { return "#transmitter"; }
		return "#target";
	}

	const radar::Radar* getPrimaryRadar(const std::vector<const radar::Object*>& objects)
	{
		for (const auto* obj : objects) { if (const auto r = dynamic_cast<const radar::Radar*>(obj)) { return r; } }
		return nullptr;
	}

	void generateIsotropicAntennaKml
	(std::ofstream& kmlFile, const math::Vec3& position, const GeographicLib::LocalCartesian& proj)
	{
		double lat, lon, alt_abs;
		proj.Reverse(position.x, position.y, position.z, lat, lon, alt_abs);

		const auto circle_coordinates = generateCircleCoordinates(lat, lon, ISOTROPIC_PATTERN_RADIUS_KM);

		kmlFile << "<Placemark>\n";
		kmlFile << "    <name>Isotropic pattern range</name>\n";
		kmlFile << "    <styleUrl>#translucentPolygon</styleUrl>\n";
		kmlFile << "    <Polygon>\n";
		kmlFile << "        <extrude>1</extrude>\n";
		kmlFile << "        <altitudeMode>absolute</altitudeMode>\n";
		kmlFile << "        <outerBoundaryIs><LinearRing><coordinates>\n";
		for (const auto& [pt_lat, pt_lon] : circle_coordinates)
		{
			kmlFile << "            " << formatCoordinates(pt_lon, pt_lat, alt_abs) << "\n";
		}
		kmlFile << "            " << formatCoordinates
			(circle_coordinates[0].second, circle_coordinates[0].first, alt_abs) << "\n";
		kmlFile << "        </coordinates></LinearRing></outerBoundaryIs>\n";
		kmlFile << "    </Polygon>\n";
		kmlFile << "</Placemark>\n";
	}

	void generateSincAntennaKml
	(
		std::ofstream& kmlFile, const antenna::Sinc* sinc, const radar::Platform* platform,
		const GeographicLib::LocalCartesian& proj
		)
	{
		const auto& first_wp_pos = platform->getMotionPath()->getCoords().front().pos;
		double start_lat, start_lon, start_alt;
		proj.Reverse(first_wp_pos.x, first_wp_pos.y, first_wp_pos.z, start_lat, start_lon, start_alt);
		const std::string start_coords_str = formatCoordinates(start_lon, start_lat, start_alt);

		const math::SVec3 initial_rotation = platform->getRotationPath()->getPosition(params::startTime());
		// The conversion from FERS internal azimuth to KML heading is now handled by a dedicated
		// function that correctly transforms from FERS's coordinate system (azimuth counter-clockwise
		// from East) to KML's heading convention (clockwise from North).
		const double start_azimuth_deg_kml = convertFersAzimuthToKmlHeading(initial_rotation.azimuth);

		// TODO: Antenna beam visualization is static, showing only the orientation at the simulation's start time.
		//       This does not represent dynamic scanning defined by a platform's <rotationpath>. The KML should
		//       ideally visualize the scan volume or animate the beam's movement using a <gx:Track> to match FERS's
		//       capabilities.
		// Main beam
		double dest_lat, dest_lon;
		calculateDestinationCoordinate
			(start_lat, start_lon, start_azimuth_deg_kml, SINC_ARROW_LENGTH_M, dest_lat, dest_lon);
		const std::string end_coords_str = formatCoordinates(dest_lon, dest_lat, start_alt);
		writeAntennaBeamLine(kmlFile, "Antenna Direction", "#lineStyle", start_coords_str, end_coords_str);

		// 3dB beamwidth lines
		const double angle_3db_drop_deg = find3DbDropAngle(sinc->getAlpha(), sinc->getBeta(), sinc->getGamma());

		double side1_lat, side1_lon;
		calculateDestinationCoordinate
			(
				start_lat, start_lon, start_azimuth_deg_kml - angle_3db_drop_deg, SINC_ARROW_LENGTH_M, side1_lat,
				side1_lon
				);
		const std::string side1_coords_str = formatCoordinates(side1_lon, side1_lat, start_alt);
		writeAntennaBeamLine(kmlFile, "Antenna Side Line 1", "#lineStyleBlue", start_coords_str, side1_coords_str);

		double side2_lat, side2_lon;
		calculateDestinationCoordinate
			(
				start_lat, start_lon, start_azimuth_deg_kml + angle_3db_drop_deg, SINC_ARROW_LENGTH_M, side2_lat,
				side2_lon
				);
		const std::string side2_coords_str = formatCoordinates(side2_lon, side2_lat, start_alt);
		writeAntennaBeamLine(kmlFile, "Antenna Side Line 2", "#lineStyleBlue", start_coords_str, side2_coords_str);

		// Arrow placemark
		kmlFile << "<Placemark>\n";
		kmlFile << "    <name>Antenna Arrow</name>\n";
		kmlFile << "    <styleUrl>#arrowStyle</styleUrl>\n";
		kmlFile << "    <Point><coordinates>" << end_coords_str <<
			"</coordinates><altitudeMode>absolute</altitudeMode></Point>\n";
		kmlFile << "    <IconStyle><heading>" << start_azimuth_deg_kml << "</heading></IconStyle>\n";
		kmlFile << "</Placemark>\n";
	}

	void generateAntennaKml
	(
		std::ofstream& kmlFile, const radar::Platform* platform, const radar::Radar* radar,
		const GeographicLib::LocalCartesian& proj
		)
	{
		const antenna::Antenna* ant = radar->getAntenna();
		if (!ant || platform->getMotionPath()->getCoords().empty()) { return; }

		if (dynamic_cast<const antenna::Isotropic*>(ant))
		{
			const math::Vec3 initial_pos = platform->getMotionPath()->getCoords().front().pos;
			generateIsotropicAntennaKml(kmlFile, initial_pos, proj);
		}
		else if (const auto* sinc_ant = dynamic_cast<const antenna::Sinc*>(ant))
		{
			generateSincAntennaKml(kmlFile, sinc_ant, platform, proj);
		}
	}

	void generateDynamicPathKml
	(
		std::ofstream& kmlFile, const radar::Platform* platform, const std::string& styleUrl, const double refAlt,
		const GeographicLib::LocalCartesian& proj
		)
	{
		const math::Path* path = platform->getMotionPath();
		const auto& waypoints = path->getCoords();

		double first_alt_abs;
		{
			double lat, lon;
			proj.Reverse
				(waypoints.front().pos.x, waypoints.front().pos.y, waypoints.front().pos.z, lat, lon, first_alt_abs);
		}

		kmlFile << "<Placemark>\n";
		kmlFile << "    <name>" << platform->getName() << "</name>\n";
		kmlFile << "    <styleUrl>" << styleUrl << "</styleUrl>\n";
		kmlFile << "    <gx:Track>\n";
		kmlFile << "        <altitudeMode>absolute</altitudeMode>\n";
		if (first_alt_abs > refAlt) { kmlFile << "        <extrude>1</extrude>\n"; }

		// The sampling time range is now based on the platform's specific motion path duration,
		// ensuring accurate track resolution for objects with short lifespans.
		const double start_time = waypoints.front().t;
		const double end_time = waypoints.back().t;
		const double time_diff = end_time - start_time;

		// Handle single-point paths or paths with zero duration by emitting a single coordinate.
		if (time_diff <= 0.0)
		{
			const math::Vec3 p_local_pos = path->getPosition(start_time);
			double p_lon, p_lat, p_alt_abs;
			proj.Reverse(p_local_pos.x, p_local_pos.y, p_local_pos.z, p_lat, p_lon, p_alt_abs);
			kmlFile << "        <when>" << start_time << "</when>\n";
			kmlFile << "        <gx:coord>" << p_lon << " " << p_lat << " " << p_alt_abs << "</gx:coord>\n";
		}
		else
		{
			const double time_step = time_diff / TRACK_NUM_DIVISIONS;
			for (int i = 0; i <= TRACK_NUM_DIVISIONS; ++i)
			{
				const double current_time = start_time + i * time_step;
				const math::Vec3 p_local_pos = path->getPosition(current_time);

				double p_lon, p_lat, p_alt_abs;
				proj.Reverse(p_local_pos.x, p_local_pos.y, p_local_pos.z, p_lat, p_lon, p_alt_abs);

				kmlFile << "        <when>" << current_time << "</when>\n";
				kmlFile << "        <gx:coord>" << p_lon << " " << p_lat << " " << p_alt_abs << "</gx:coord>\n";
			}
		}

		kmlFile << "    </gx:Track>\n";
		kmlFile << "</Placemark>\n";
	}

	void generateTrackEndpointsKml
	(
		std::ofstream& kmlFile, const radar::Platform* platform, const double refAlt,
		const GeographicLib::LocalCartesian& proj
		)
	{
		const math::Path* path = platform->getMotionPath();
		if (path->getCoords().size() <= 1) { return; }

		const auto& [start_wp_pos, start_wp_t] = path->getCoords().front();
		const auto& [end_wp_pos, end_wp_t] = path->getCoords().back();

		double start_lat, start_lon, start_alt_abs;
		proj.Reverse(start_wp_pos.x, start_wp_pos.y, start_wp_pos.z, start_lat, start_lon, start_alt_abs);
		const std::string start_coordinates = formatCoordinates(start_lon, start_lat, start_alt_abs);

		double end_lat, end_lon, end_alt_abs;
		proj.Reverse(end_wp_pos.x, end_wp_pos.y, end_wp_pos.z, end_lat, end_lon, end_alt_abs);
		const std::string end_coordinates = formatCoordinates(end_lon, end_lat, end_alt_abs);

		writePoint(kmlFile, "Start: " + platform->getName(), "#target", start_coordinates, start_alt_abs, refAlt);
		writePoint(kmlFile, "End: " + platform->getName(), "#target", end_coordinates, end_alt_abs, refAlt);
	}

	void generateStaticPlacemarkKml
	(
		std::ofstream& kmlFile, const radar::Platform* platform, const std::string& styleUrl, const double refAlt,
		const GeographicLib::LocalCartesian& proj
		)
	{
		const auto& [first_wp_pos, first_wp_t] = platform->getMotionPath()->getCoords().front();
		double lat, lon, alt_abs;
		proj.Reverse(first_wp_pos.x, first_wp_pos.y, first_wp_pos.z, lat, lon, alt_abs);
		const std::string coordinates = formatCoordinates(lon, lat, alt_abs);

		kmlFile << "<Placemark>\n";
		kmlFile << "    <name>" << platform->getName() << "</name>\n";
		kmlFile << "    <styleUrl>" << styleUrl << "</styleUrl>\n";
		kmlFile << "    <LookAt>\n";
		kmlFile << "        <longitude>" << lon << "</longitude>\n";
		kmlFile << "        <latitude>" << lat << "</latitude>\n";
		kmlFile << "        <altitude>" << alt_abs << "</altitude>\n";
		kmlFile << "        <heading>-148.41</heading><tilt>40.55</tilt><range>500.65</range>\n";
		kmlFile << "    </LookAt>\n";
		kmlFile << "    <Point>\n";
		kmlFile << "        <coordinates>" << coordinates << "</coordinates>\n";
		kmlFile << "        <altitudeMode>absolute</altitudeMode>\n";
		if (alt_abs > refAlt) { kmlFile << "        <extrude>1</extrude>\n"; }
		kmlFile << "    </Point>\n";
		kmlFile << "</Placemark>\n";
	}

	void generatePlatformPathKml
	(
		std::ofstream& kmlFile, const radar::Platform* platform, const std::string& style, const double refAlt,
		const GeographicLib::LocalCartesian& proj
		)
	{
		const auto path_type = platform->getMotionPath()->getType();
		const bool is_dynamic = (path_type == math::Path::InterpType::INTERP_LINEAR || path_type ==
			math::Path::InterpType::INTERP_CUBIC);

		if (is_dynamic)
		{
			generateDynamicPathKml(kmlFile, platform, style, refAlt, proj);
			generateTrackEndpointsKml(kmlFile, platform, refAlt, proj);
		}
		else { generateStaticPlacemarkKml(kmlFile, platform, style, refAlt, proj); }
	}

	void processPlatform
	(
		const radar::Platform* platform, const std::vector<const radar::Object*>& objects, std::ofstream& kmlFile,
		const GeographicLib::LocalCartesian& proj, const double referenceAltitude
		)
	{
		if (platform->getMotionPath()->getCoords().empty()) { return; }

		const auto placemark_style = getPlacemarkStyleForPlatform(objects);

		if (const auto* radar_obj = getPrimaryRadar
			(objects)) { generateAntennaKml(kmlFile, platform, radar_obj, proj); }

		generatePlatformPathKml(kmlFile, platform, placemark_style, referenceAltitude, proj);
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

			writeKmlHeaderAndStyles(kml_file);

			kml_file << "<Folder>\n";
			kml_file << "  <name>Reference Coordinate</name>\n";
			kml_file <<
				"  <description>Placemarks for various elements in the FERSXML file. All Placemarks are situated relative to this reference point.</description>\n";
			kml_file << "  <LookAt>\n";
			kml_file << "    <longitude>" << reference_longitude << "</longitude>\n";
			kml_file << "    <latitude>" << reference_latitude << "</latitude>\n";
			kml_file << "    <altitude>" << reference_altitude << "</altitude>\n";
			kml_file << "    <heading>-148.41</heading><tilt>40.55</tilt><range>10000</range>\n";
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
		catch (const std::exception& e)
		{
			LOG(logging::Level::ERROR, "Error generating KML file: {}", e.what());
			return false;
		}
		catch (...)
		{
			LOG(logging::Level::ERROR, "Unknown error occurred while generating KML file.");
			return false;
		}
		return true;
	}
}
