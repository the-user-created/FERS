#include "serial/kml_generator.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "config.h"
#include "core/logging.h"
#include "serial/libxml_wrapper.h"

namespace
{
	using namespace std;

	// Helper function to find all elements with a given tag name recursively under a node
	void getElementsByTagName(xmlNode* node, const std::string& tagName, std::vector<XmlElement>& elements)
	{
		for (xmlNode* cur_node = node; cur_node; cur_node = cur_node->next)
		{
			if (cur_node->type == XML_ELEMENT_NODE)
			{
				if (cur_node->name && tagName == reinterpret_cast<const char*>(cur_node->name))
				{
					elements.emplace_back(cur_node);
				}
				getElementsByTagName(cur_node->children, tagName, elements);
			}
		}
	}

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

	std::string getCoordinatesFromPositionWaypoint
	(
		const XmlElement& positionWaypointElement, const double referenceLatitude,
		const double referenceLongitude, const double referenceAltitude
		)
	{
		const double x = std::stod(positionWaypointElement.childElement("x", 0).getText());
		const double y = std::stod(positionWaypointElement.childElement("y", 0).getText());
		const double altitude = std::stod(positionWaypointElement.childElement("altitude", 0).getText());
		const double longitude = referenceLongitude + x / (cos(referenceLatitude * PI / 180) * 111319.9);
		const double latitude = referenceLatitude + y / 111319.9;
		const double altitude_above_ground = altitude - referenceAltitude;
		std::stringstream coordinates;
		coordinates << std::fixed << std::setprecision
			(6) << longitude << "," << latitude << "," << altitude_above_ground;
		return coordinates.str();
	}

	void calculateDestinationCoordinate
	(
		const double startLatitude, const double startLongitude, const double angle, double distance,
		double& destLatitude, double& destLongitude
		)
	{
		constexpr double r = 6371000; // Earth's radius in meters
		const double d = distance / r; // Angular distance in radians
		const double start_lat_rad = startLatitude * PI / 180;
		const double start_lon_rad = startLongitude * PI / 180;
		const double angle_rad = angle * PI / 180;
		const double dest_lat_rad = asin(sin(start_lat_rad) * cos(d) + cos(start_lat_rad) * sin(d) * cos(angle_rad));
		const double dest_lon_rad = start_lon_rad +
			atan2(sin(angle_rad) * sin(d) * cos(start_lat_rad), cos(d) - sin(start_lat_rad) * sin(dest_lat_rad));
		destLatitude = dest_lat_rad * 180 / PI;
		destLongitude = dest_lon_rad * 180 / PI;
	}

	void updateLongitudeLatitudeCubic
	(
		double& newLongitude, double& newLatitude, const double t, const double longitude1,
		const double latitude1, const double longitude4, const double latitude4
		)
	{
		constexpr double control_point_angle = 45.0 * PI / 180.0;
		constexpr double control_point_distance = 111319.9;
		const double x2 = longitude1 +
			control_point_distance * cos(control_point_angle) / (cos(latitude1 * PI / 180) * 111319.9);
		const double y2 = latitude1 + control_point_distance * sin(control_point_angle) / 111319.9;
		const double x3 = longitude4 -
			control_point_distance * cos(control_point_angle) / (cos(latitude4 * PI / 180) * 111319.9);
		const double y3 = latitude4 - control_point_distance * sin(control_point_angle) / 111319.9;
		const double one_minus_t = 1 - t;
		const double t2 = t * t;
		const double t3 = t2 * t;
		const double one_minus_t2 = one_minus_t * one_minus_t;
		const double one_minus_t3 = one_minus_t2 * one_minus_t;
		newLongitude = one_minus_t3 * longitude1 + 3 * one_minus_t2 * t * x2 + 3 * one_minus_t * t2 * x3 + t3 *
			longitude4;
		newLatitude = one_minus_t3 * latitude1 + 3 * one_minus_t2 * t * y2 + 3 * one_minus_t * t2 * y3 + t3 * latitude4;
	}

	void populateAntennaMaps
	(
		const XmlElement& element, std::map<std::string, XmlElement>& isotropicAntennas,
		std::map<std::string, XmlElement>& patternedAntennas
		)
	{
		std::vector<XmlElement> antenna_elements;
		getElementsByTagName(element.getNode()->children, "antenna", antenna_elements);
		for (const auto& antenna_element : antenna_elements)
		{
			std::string name_str = XmlElement::getSafeAttribute(antenna_element, "name");
			if (std::string pattern_value = XmlElement::getSafeAttribute
				(antenna_element, "pattern"); pattern_value == "isotropic")
			{
				isotropicAntennas.insert({name_str, antenna_element});
			}
			else { patternedAntennas.insert({name_str, antenna_element}); }
		}
	}

	bool isAntennaIsotropic(const std::string& antennaName, const std::map<std::string, XmlElement>& isotropicAntennas)
	{
		return isotropicAntennas.contains(antennaName);
	}

	double deg2Rad(const double degrees) { return degrees * PI / 180.0; }

	std::vector<std::pair<double, double>> generateCircleCoordinates
	(
		const double lat, const double lon, double radius_km,
		int num_points = 100
		)
	{
		std::vector<std::pair<double, double>> circle_coordinates;
		for (int i = 0; i < num_points; i++)
		{
			constexpr double radius_earth = 6371.0;
			const double bearing = deg2Rad(i * 360.0 / num_points);
			const double lat_rad = deg2Rad(lat);
			const double lon_rad = deg2Rad(lon);
			const double angular_distance = radius_km / radius_earth;
			const double new_lat_rad = asin
				(
					sin(lat_rad) * cos(angular_distance) +
					cos(lat_rad) * sin(angular_distance) * cos(bearing)
					);
			const double new_lon_rad = lon_rad +
				atan2
				(
					sin(bearing) * sin(angular_distance) * cos(lat_rad),
					cos(angular_distance) - sin(lat_rad) * sin(new_lat_rad)
					);
			double new_lat = new_lat_rad * 180.0 / PI;
			double new_lon = new_lon_rad * 180.0 / PI;
			circle_coordinates.emplace_back(new_lat, new_lon);
		}
		return circle_coordinates;
	}

	std::optional<XmlElement> getAntennaElementWithSincPattern(const XmlElement& rootElement)
	{
		std::vector<XmlElement> antenna_list;
		getElementsByTagName(rootElement.getNode()->children, "antenna", antenna_list);
		for (const auto& current_antenna_element : antenna_list)
		{
			if (XmlElement::getSafeAttribute
				(current_antenna_element, "pattern") == "sinc") { return current_antenna_element; }
		}
		return std::nullopt;
	}

	void processElement
	(
		const XmlElement& element, std::ofstream& kmlFile, double referenceLatitude,
		double referenceLongitude, double referenceAltitude, const XmlDocument& document
		)
	{
		std::map<std::string, XmlElement> isotropic_antennas;
		std::map<std::string, XmlElement> patterned_antennas;
		populateAntennaMaps(document.getRootElement(), isotropic_antennas, patterned_antennas);
		if (element.name() == "platform")
		{
			std::vector<XmlElement> position_waypoint_list;
			getElementsByTagName(element.getNode()->children, "positionwaypoint", position_waypoint_list);
			if (position_waypoint_list.empty()) { return; }
			ranges::sort
				(
					position_waypoint_list
					,
					[](const XmlElement& a, const XmlElement& b)
					{
						const double time_a = std::stod(a.childElement("time", 0).getText());
						const double time_b = std::stod(b.childElement("time", 0).getText());
						return time_a < time_b;
					}
					);
			const XmlElement& position_waypoint_element = position_waypoint_list.front();
			double x = std::stod(position_waypoint_element.childElement("x", 0).getText());
			double y = std::stod(position_waypoint_element.childElement("y", 0).getText());
			double altitude = std::stod(position_waypoint_element.childElement("altitude", 0).getText());
			double longitude = referenceLongitude + x / (cos(referenceLatitude * PI / 180) * 111319.9);
			double latitude = referenceLatitude + y / 111319.9;
			double altitude_above_ground = altitude - referenceAltitude;
			XmlElement motion_path_element = element.childElement("motionpath", 0);
			string interpolation = XmlElement::getSafeAttribute(motion_path_element, "interpolation");
			bool is_static = interpolation == "static";
			bool is_linear = interpolation == "linear";
			bool is_cubic = interpolation == "cubic";
			if (interpolation == "python")
			{
				LOG
				(
					logging::Level::WARNING,
					"'python' interpolation is not supported by the visualizer and will not be rendered."
					);
			}

			// TODO: UNUSED
			//std::string placemarkStyle;
			//if (element.childElement("receiver", 0).isValid()) { placemarkStyle = "receiver"; }
			//else if (element.childElement("transmitter", 0).isValid()) { placemarkStyle = "transmitter"; }
			//else if (element.childElement("target", 0).isValid()) { placemarkStyle = "target"; }

			bool is_isotropic = false;
			if (XmlElement receiver_element = element.childElement("receiver", 0); receiver_element.isValid())
			{
				std::string antenna_name = XmlElement::getSafeAttribute(receiver_element, "antenna");
				is_isotropic = isAntennaIsotropic(antenna_name, isotropic_antennas);
			}
			else if (XmlElement transmitter_element = element.childElement
				("transmitter", 0); transmitter_element.isValid())
			{
				std::string antenna_name = XmlElement::getSafeAttribute(transmitter_element, "antenna");
				is_isotropic = isAntennaIsotropic(antenna_name, isotropic_antennas);
			}
			if (is_isotropic)
			{
				double circle_radius = 20;
				int num_points = 100;
				kmlFile << "<Placemark>\n";
				kmlFile << "    <name>Isotropic pattern range</name>\n";
				kmlFile << "    <styleUrl>#translucentPolygon</styleUrl>\n";
				std::vector<std::pair<double, double>> circle_coordinates =
					generateCircleCoordinates(latitude, longitude, circle_radius, num_points);
				kmlFile << "    <Polygon>\n";
				kmlFile << "        <extrude>1</extrude>\n";
				kmlFile << "        <altitudeMode>relativeToGround</altitudeMode>\n";
				kmlFile << "        <outerBoundaryIs>\n";
				kmlFile << "            <LinearRing>\n";
				kmlFile << "                <coordinates>\n";
				for (const auto& [fst, snd] : circle_coordinates)
				{
					kmlFile << "                    " << snd << "," << fst << "," <<
						altitude_above_ground << "\n";
				}
				kmlFile << "                    " << circle_coordinates[0].second << "," << circle_coordinates[0].first
					<< ","
					<< altitude_above_ground << "\n";
				kmlFile << "                </coordinates>\n";
				kmlFile << "            </LinearRing>\n";
				kmlFile << "        </outerBoundaryIs>\n";
				kmlFile << "    </Polygon>\n";
				kmlFile << "</Placemark>\n";
			}
			else if (!is_isotropic &&
				(element.childElement("transmitter", 0).isValid() || element.childElement("receiver", 0).isValid()))
			{
				double start_azimuth = std::stod(element.childElement("startazimuth", 0).getText());
				XmlElement pos_waypoint_element = element.childElement("positionwaypoint", 0);
				if (!pos_waypoint_element.isValid())
				{
					pos_waypoint_element = element.childElement("motionpath", 0).childElement("positionwaypoint", 0);
				}
				std::string coordinates =
					getCoordinatesFromPositionWaypoint
					(
						pos_waypoint_element, referenceLatitude, referenceLongitude,
						referenceAltitude
						);
				double arrow_length = 20000;
				double start_latitude, start_longitude, start_altitude;
				std::istringstream coordinates_stream(coordinates);
				coordinates_stream >> start_longitude;
				coordinates_stream.ignore(1);
				coordinates_stream >> start_latitude;
				coordinates_stream.ignore(1);
				coordinates_stream >> start_altitude;
				start_azimuth = start_azimuth + 180;
				double dest_latitude, dest_longitude;
				calculateDestinationCoordinate
					(
						start_latitude, start_longitude, start_azimuth, arrow_length, dest_latitude,
						dest_longitude
						);
				std::stringstream end_coordinates_stream;
				end_coordinates_stream << std::fixed << std::setprecision
					(6) << dest_longitude << "," << dest_latitude << ","
					<< start_altitude;
				std::string end_coordinates = end_coordinates_stream.str();
				if (auto sinc_antenna_element_opt = getAntennaElementWithSincPattern(document.getRootElement()))
				{
					XmlElement sinc_antenna_element = sinc_antenna_element_opt.value();
					double alpha = std::stod(sinc_antenna_element.childElement("alpha", 0).getText());
					double beta = std::stod(sinc_antenna_element.childElement("beta", 0).getText());
					double gamma = std::stod(sinc_antenna_element.childElement("gamma", 0).getText());
					double angle_3d_b_drop_deg = find3DbDropAngle(alpha, beta, gamma);
					double side_line1_azimuth = start_azimuth - angle_3d_b_drop_deg;
					double side_line2_azimuth = start_azimuth + angle_3d_b_drop_deg;
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
						std::string side_line_end_coordinates = (i == 1)
							? side_line1_end_coordinates
							: side_line2_end_coordinates;
						kmlFile << "<Placemark>\n";
						kmlFile << "      <name>" << side_line_name << "</name>\n";
						kmlFile << "      <styleUrl>#lineStyleBlue</styleUrl>\n";
						kmlFile << "      <LineString>\n";
						kmlFile << "            <tessellate>1</tessellate>\n";
						kmlFile << "            <coordinates>\n";
						kmlFile << "            " + coordinates + " " + side_line_end_coordinates + "\n";
						kmlFile << "            </coordinates>\n";
						kmlFile << "      </LineString>\n";
						kmlFile << "</Placemark>\n";
					}
				}
				else { LOG(logging::Level::ERROR, "Antenna element with pattern='sinc' not found in the XML file"); }
				kmlFile << "<Placemark>\n";
				kmlFile << "      <name>Antenna Direction</name>\n";
				kmlFile << "      <styleUrl>#lineStyle</styleUrl>\n";
				kmlFile << "      <LineString>\n";
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
				kmlFile << "      </Point>\n";
				kmlFile << "      <IconStyle>\n";
				kmlFile << "          <heading>" << start_azimuth << "</heading>\n";
				kmlFile << "      </IconStyle>\n";
				kmlFile << "</Placemark>\n";
			}
			kmlFile << "<Placemark>\n";
			kmlFile << "    <name>" << XmlElement::getSafeAttribute(element, "name") << "</name>\n";
			if (element.childElement("receiver", 0).isValid() || element.childElement("monostatic", 0).isValid())
			{
				kmlFile << "    <styleUrl>#receiver</styleUrl>\n";
			}
			else if (element.childElement
				("transmitter", 0).isValid()) { kmlFile << "    <styleUrl>#transmitter</styleUrl>\n"; }
			else if (element.childElement("target", 0).isValid()) { kmlFile << "    <styleUrl>#target</styleUrl>\n"; }
			if (is_static || is_linear || is_cubic)
			{
				kmlFile << "    <gx:Track>\n";
				if (altitude_above_ground > 0)
				{
					kmlFile << "        <altitudeMode>relativeToGround</altitudeMode>\n";
					kmlFile << "        <extrude>1</extrude>\n";
				}
				else { kmlFile << "        <altitudeMode>clampToGround</altitudeMode>\n"; }
				for (size_t i = 0; i < position_waypoint_list.size(); ++i)
				{
					const XmlElement& p_waypoint_element = position_waypoint_list[i];
					double p_x = std::stod(p_waypoint_element.childElement("x", 0).getText());
					double p_y = std::stod(p_waypoint_element.childElement("y", 0).getText());
					double p_alt = std::stod(p_waypoint_element.childElement("altitude", 0).getText());
					double p_lon = referenceLongitude + p_x / (cos
						(referenceLatitude * PI / 180) * 111319.9);
					double p_lat = referenceLatitude + p_y / 111319.9;
					double p_alt_ag = p_alt - referenceAltitude;
					double time = std::stod(p_waypoint_element.childElement("time", 0).getText());
					if (is_cubic && i + 1 < position_waypoint_list.size())
					{
						const XmlElement& next_position_waypoint_element = position_waypoint_list[i + 1];
						double next_time = std::stod(next_position_waypoint_element.childElement("time", 0).getText());
						double time_diff = next_time - time;
						double next_x = std::stod(next_position_waypoint_element.childElement("x", 0).getText());
						double next_y = std::stod(next_position_waypoint_element.childElement("y", 0).getText());
						double next_altitude = std::stod
							(next_position_waypoint_element.childElement("altitude", 0).getText());
						double next_longitude =
							referenceLongitude + next_x / (cos(referenceLatitude * PI / 180) * 111319.9);
						double next_latitude = referenceLatitude + next_y / 111319.9;
						double next_altitude_above_ground = next_altitude - referenceAltitude;
						int num_divisions = 100;
						for (int j = 0; j <= num_divisions; ++j)
						{
							double t = static_cast<double>(j) / num_divisions;
							double new_longitude, new_latitude;
							updateLongitudeLatitudeCubic
								(
									new_longitude, new_latitude, t, p_lon, p_lat, next_longitude,
									next_latitude
									);
							double new_altitude_above_ground = p_alt_ag + t * (next_altitude_above_ground - p_alt_ag);
							kmlFile << "        <when>" << time + (static_cast<double>(j) * time_diff) / num_divisions
								<<
								"</when>\n";
							kmlFile << "        <gx:coord>" << new_longitude << " " << new_latitude << " " <<
								new_altitude_above_ground << "</gx:coord>\n";
						}
					}
					else if (is_linear || is_static)
					{
						kmlFile << "        <when>" << time << "</when>\n";
						kmlFile << "        <gx:coord>" << p_lon << " " << p_lat << " " << p_alt_ag << "</gx:coord>\n";
					}
				}
				kmlFile << "    </gx:Track>\n";
			}
			else
			{
				kmlFile << "    <LookAt>\n";
				kmlFile << "        <longitude>" << longitude << "</longitude>\n";
				kmlFile << "        <latitude>" << latitude << "</latitude>\n";
				kmlFile << "        <altitude>" << altitude_above_ground << "</altitude>\n";
				kmlFile << "        <heading>-148.4122922628044</heading>\n";
				kmlFile << "        <tilt>40.5575073395506</tilt>\n";
				kmlFile << "        <range>500.6566641072245</range>\n";
				kmlFile << "    </LookAt>\n";
				kmlFile << "    <Point>\n";
				kmlFile << "        <coordinates>" << longitude << "," << latitude << "," << altitude_above_ground <<
					"</coordinates>\n";
				if (altitude_above_ground > 0)
				{
					kmlFile << "        <altitudeMode>relativeToGround</altitudeMode>\n";
					kmlFile << "        <extrude>1</extrude>\n";
				}
				else { kmlFile << "        <altitudeMode>clampToGround</altitudeMode>\n"; }
				kmlFile << "    </Point>\n";
			}
			kmlFile << "</Placemark>\n";
			if (is_static || is_linear || is_cubic)
			{
				const XmlElement& first_position_waypoint_element = position_waypoint_list.front();
				const XmlElement& last_position_waypoint_element = position_waypoint_list.back();
				std::string start_coordinates = getCoordinatesFromPositionWaypoint
					(
						first_position_waypoint_element, referenceLatitude, referenceLongitude, referenceAltitude
						);
				std::string end_coordinates = getCoordinatesFromPositionWaypoint
					(
						last_position_waypoint_element, referenceLatitude, referenceLongitude, referenceAltitude
						);
				kmlFile << "<Placemark>\n";
				kmlFile << "    <name>Start: " << XmlElement::getSafeAttribute(element, "name") << "</name>\n";
				kmlFile << "    <styleUrl>#target</styleUrl>\n";
				kmlFile << "    <Point>\n";
				kmlFile << "        <coordinates>" << start_coordinates << "</coordinates>\n";
				if (altitude_above_ground > 0)
				{
					kmlFile << "        <altitudeMode>relativeToGround</altitudeMode>\n";
					kmlFile << "        <extrude>1</extrude>\n";
				}
				else { kmlFile << "        <altitudeMode>clampToGround</altitudeMode>\n"; }
				kmlFile << "    </Point>\n";
				kmlFile << "</Placemark>\n";
				kmlFile << "<Placemark>\n";
				kmlFile << "    <name>End: " << XmlElement::getSafeAttribute(element, "name") << "</name>\n";
				kmlFile << "    <styleUrl>#target</styleUrl>\n";
				kmlFile << "    <Point>\n";
				kmlFile << "        <coordinates>" << end_coordinates << "</coordinates>\n";
				if (altitude_above_ground > 0)
				{
					kmlFile << "        <altitudeMode>relativeToGround</altitudeMode>\n";
					kmlFile << "        <extrude>1</extrude>\n";
				}
				else { kmlFile << "        <altitudeMode>clampToGround</altitudeMode>\n"; }
				kmlFile << "    </Point>\n";
				kmlFile << "</Placemark>\n";
			}
		}
	}

	void traverseDomNode
	(
		xmlNode* node, std::ofstream& kmlFile, const double referenceLatitude, const double referenceLongitude,
		const double referenceAltitude, const XmlDocument& document
		)
	{
		for (const xmlNode* cur_node = node; cur_node; cur_node = cur_node->next)
		{
			if (cur_node->type == XML_ELEMENT_NODE)
			{
				if (const XmlElement element(cur_node); element.name() == "platform")
				{
					processElement
						(element, kmlFile, referenceLatitude, referenceLongitude, referenceAltitude, document);
				}
				else
				{
					traverseDomNode
						(
							cur_node->children, kmlFile, referenceLatitude, referenceLongitude, referenceAltitude,
							document
							);
				}
			}
		}
	}
}

namespace serial
{
	bool KmlGenerator::generateKml(const std::string& inputXmlPath, const std::string& outputKmlPath)
	{
		try
		{
			// Setting default geographical and altitude coordinates
			constexpr double reference_altitude = 0;
			constexpr double reference_longitude = 18.4563;
			constexpr double reference_latitude = -33.9545;
			XmlDocument document;
			if (!document.loadFile(inputXmlPath))
			{
				LOG(logging::Level::ERROR, "Could not load or parse XML file {}", inputXmlPath);
				return false;
			}
			const XmlElement root_element = document.getRootElement();
			if (!root_element.isValid())
			{
				LOG(logging::Level::ERROR, "Root element not found in XML file");
				return false;
			}
			std::ofstream kml_file(outputKmlPath.c_str());
			if (!kml_file.is_open())
			{
				LOG(logging::Level::ERROR, "Error opening output KML file {}", outputKmlPath);
				return false;
			}
			kml_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
			kml_file <<
				"<kml xmlns=\"http://www.opengis.net/kml/2.2\" xmlns:gx=\"http://www.google.com/kml/ext/2.2\">\n";
			kml_file << "<Document>\n";
			kml_file << "<name>" << inputXmlPath << "</name>\n";
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
			traverseDomNode
				(
					root_element.getNode(), kml_file, reference_latitude, reference_longitude, reference_altitude,
					document
					);
			kml_file << "</Folder>\n";
			kml_file << "</Document>\n";
			kml_file << "</kml>\n";
			kml_file.close();
		}
		catch (const XmlException& e)
		{
			LOG(logging::Level::ERROR, "Error parsing XML: {}", e.what());
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
