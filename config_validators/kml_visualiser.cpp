// FERS input Validator Function sub-system
// Outputs KML GIS readable file.
// Script written by Michael Altshuler
// University of Cape Town: ALTMIC003

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numbers>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "config.h"
#include "serial/libxml_wrapper.h"

using namespace std;

// Helper function to find all elements with a given tag name recursively under a node
void getElementsByTagName(xmlNode* node, const std::string& tagName, std::vector<XmlElement>& elements)
{
	for (xmlNode* cur_node = node; cur_node; cur_node = cur_node->next)
	{
		if (cur_node->type == XML_ELEMENT_NODE)
		{
			if (cur_node->name && tagName == (const char*)cur_node->name) { elements.push_back(XmlElement(cur_node)); }
			getElementsByTagName(cur_node->children, tagName, elements);
		}
	}
}

// The following two functions calculate the 3dB drop from the max gain of the antenna gain pattern
/*Start*/
double sinc_antenna_gain(double theta, double alpha, double beta, double gamma)
{
	if (theta == 0.0)
	{
		return alpha; // Avoid division by zero at theta=0
	}
	double gain = alpha * std::pow(std::sin(beta * theta) / (beta * theta), gamma);
	return gain;
}

double find_3db_drop_angle(double alpha, double beta, double gamma)
{
	const int num_points = 1000;
	const double pi = std::numbers::pi;
	std::vector<double> theta(num_points);
	std::vector<double> gain(num_points);

	// Calculate gain values for each angle
	for (int i = 0; i < num_points; ++i)
	{
		theta[i] = -pi + 2.0 * pi * i / (num_points - 1);
		gain[i] = sinc_antenna_gain(theta[i], alpha, beta, gamma);
	}

	// Find the maximum gain
	double max_gain = *std::max_element(gain.begin() + num_points / 2, gain.end());

	// Convert the maximum gain to dB
	double max_gain_dB = 10.0 * std::log10(max_gain);

	// Find the target gain (3dB drop)
	double target_gain_dB = max_gain_dB - 3.0;
	double target_gain = std::pow(10.0, target_gain_dB / 10.0);

	// Find the angle index where the gain is closest to the target gain
	// Considering only positive angles (from 0 to pi)
	int idx = std::distance
		(
			gain.begin() + num_points / 2, std::min_element
			(
				gain.begin() + num_points / 2, gain.end(),
				[target_gain](double a, double b) { return std::abs(a - target_gain) < std::abs(b - target_gain); }
				)
			);

	// Get the angle at which the 3dB drop occurs
	double angle_3dB_drop = theta[idx + num_points / 2];

	// Convert the angle to degrees
	double angle_3dB_drop_deg = angle_3dB_drop * 180.0 / pi;

	return angle_3dB_drop_deg;
}

/*End*/

// Function returns coordinates from positionWayPoint values
std::string getCoordinatesFromPositionWaypoint
(
	const XmlElement& positionWaypointElement, double referenceLatitude, double referenceLongitude,
	double referenceAltitude
	)
{
	double x = std::stod(positionWaypointElement.childElement("x", 0).getText());
	double y = std::stod(positionWaypointElement.childElement("y", 0).getText());
	double altitude = std::stod(positionWaypointElement.childElement("altitude", 0).getText());

	double longitude = referenceLongitude + x / (cos(referenceLatitude * std::numbers::pi / 180) * 111319.9);
	double latitude = referenceLatitude + y / 111319.9;
	double altitudeAboveGround = altitude - referenceAltitude;

	std::stringstream coordinates;
	coordinates << std::fixed << std::setprecision(6) << longitude << "," << latitude << "," << altitudeAboveGround;

	return coordinates.str();
}

// Function to calculate the destination coordinate given starting coordinate, angle (in degrees), and distance (in meters)
void calculateDestinationCoordinate
(
	double startLatitude, double startLongitude, double angle, double distance, double& destLatitude,
	double& destLongitude
	)
{
	const double R = 6371000; // Earth's radius in meters
	double d = distance / R; // Angular distance in radians

	// Convert degrees to radians
	double startLatRad = startLatitude * std::numbers::pi / 180;
	double startLonRad = startLongitude * std::numbers::pi / 180;
	double angleRad = angle * std::numbers::pi / 180;

	// Calculate destination latitude and longitude in radians
	double destLatRad = asin(sin(startLatRad) * cos(d) + cos(startLatRad) * sin(d) * cos(angleRad));
	double destLonRad = startLonRad + atan2
		(sin(angleRad) * sin(d) * cos(startLatRad), cos(d) - sin(startLatRad) * sin(destLatRad));

	// Convert radians to degrees
	destLatitude = destLatRad * 180 / std::numbers::pi;
	destLongitude = destLonRad * 180 / std::numbers::pi;
}

// Function calculates the cubic path and updates the longitude and latitude values.
void updateLongitudeLatitudeCubic
(
	double& newLongitude, double& newLatitude, double t, double longitude1, double latitude1,
	double longitude4,
	double latitude4
	)
{
	double controlPointAngle = 45.0 * std::numbers::pi / 180.0; // 45 degrees in radians
	double controlPointDistance = 111319.9; // Fixed distance for control points (e.g. 111319.9 meters or 1 degree)

	// Calculate control points based on fixed angle and distance
	double x2 = longitude1 + controlPointDistance * cos(controlPointAngle) / (cos(latitude1 * std::numbers::pi / 180) *
		111319.9);
	double y2 = latitude1 + controlPointDistance * sin(controlPointAngle) / 111319.9;

	double x3 = longitude4 - controlPointDistance * cos(controlPointAngle) / (cos(latitude4 * std::numbers::pi / 180) *
		111319.9);
	double y3 = latitude4 - controlPointDistance * sin(controlPointAngle) / 111319.9;

	// Perform cubic interpolation
	double one_minus_t = 1 - t;
	double t2 = t * t;
	double t3 = t2 * t;
	double one_minus_t2 = one_minus_t * one_minus_t;
	double one_minus_t3 = one_minus_t2 * one_minus_t;

	newLongitude = one_minus_t3 * longitude1 + 3 * one_minus_t2 * t * x2 + 3 * one_minus_t * t2 * x3 + t3 * longitude4;
	newLatitude = one_minus_t3 * latitude1 + 3 * one_minus_t2 * t * y2 + 3 * one_minus_t * t2 * y3 + t3 * latitude4;
}

// Function to populate antenna maps
void populateAntennaMaps
(
	const XmlElement& element, std::map<std::string, XmlElement>& isotropic_antennas,
	std::map<std::string, XmlElement>& patterned_antennas
	)
{
	std::vector<XmlElement> antennaElements;
	getElementsByTagName(element.getNode()->children, "antenna", antennaElements);
	for (const auto& antennaElement : antennaElements)
	{
		std::string nameStr = XmlElement::getSafeAttribute(antennaElement, "name");
		std::string patternValue = XmlElement::getSafeAttribute(antennaElement, "pattern");

		if (patternValue == "isotropic") { isotropic_antennas.insert({nameStr, antennaElement}); }
		else { patterned_antennas.insert({nameStr, antennaElement}); }
	}
}

// Function to check if an antenna name is isotropic
bool isAntennaIsotropic
(const std::string& antennaName, const std::map<std::string, XmlElement>& isotropic_antennas)
{
	return isotropic_antennas.find(antennaName) != isotropic_antennas.end();
}

// Function that converts degrees to radians
double deg2rad(double degrees) { return degrees * std::numbers::pi / 180.0; }

// Function to generate coordinates for a circle around a given latitude, longitude and radius
std::vector<std::pair<double, double>> generate_circle_coordinates
(double lat, double lon, double radius_km, int num_points = 100)
{
	std::vector<std::pair<double, double>> circle_coordinates;
	double radius_earth = 6371.0; // Earth's radius in km

	for (int i = 0; i < num_points; i++)
	{
		double bearing = deg2rad(i * 360.0 / num_points);
		double lat_rad = deg2rad(lat);
		double lon_rad = deg2rad(lon);
		double angular_distance = radius_km / radius_earth;

		double new_lat_rad = asin
			(
				sin(lat_rad) * cos(angular_distance) +
				cos(lat_rad) * sin(angular_distance) * cos(bearing)
				);
		double new_lon_rad = lon_rad + atan2
			(
				sin(bearing) * sin(angular_distance) * cos(lat_rad),
				cos(angular_distance) - sin(lat_rad) * sin(new_lat_rad)
				);

		double new_lat = new_lat_rad * 180.0 / std::numbers::pi;
		double new_lon = new_lon_rad * 180.0 / std::numbers::pi;

		circle_coordinates.push_back(std::make_pair(new_lat, new_lon));
	}

	return circle_coordinates;
}

// Function to return antenna element with 'sinc' pattern
std::optional<XmlElement> getAntennaElementWithSincPattern(const XmlElement& rootElement)
{
	std::vector<XmlElement> antennaList;
	getElementsByTagName(rootElement.getNode()->children, "antenna", antennaList);

	for (const auto& currentAntennaElement : antennaList)
	{
		if (XmlElement::getSafeAttribute(currentAntennaElement, "pattern") == "sinc") { return currentAntennaElement; }
	}
	return std::nullopt;
}

// Function to process the XmlElement, extract necessary data from FERSXML file, and output accordingly to KML file
void processElement
(
	const XmlElement& element, std::ofstream& kmlFile, double referenceLatitude, double referenceLongitude,
	double referenceAltitude, const XmlDocument& document
	)
{
	// Define maps to store isotropic and patterned antennas
	std::map<std::string, XmlElement> isotropic_antennas;
	std::map<std::string, XmlElement> patterned_antennas;
	populateAntennaMaps(document.getRootElement(), isotropic_antennas, patterned_antennas);

	// Check if the element is a platform
	if (element.name() == "platform")
	{
		// Get the positionwaypoint element
		std::vector<XmlElement> positionWaypointList;
		getElementsByTagName(element.getNode()->children, "positionwaypoint", positionWaypointList);
		if (positionWaypointList.empty()) { return; }

		// Sort waypoints by time to ensure chronological path rendering.
		std::sort
			(
				positionWaypointList.begin(), positionWaypointList.end(),
				[](const XmlElement& a, const XmlElement& b)
				{
					double timeA = std::stod(a.childElement("time", 0).getText());
					double timeB = std::stod(b.childElement("time", 0).getText());
					return timeA < timeB;
				}
				);

		const XmlElement& positionWaypointElement = positionWaypointList.front();

		// Extract the position coordinates
		double x = std::stod(positionWaypointElement.childElement("x", 0).getText());
		double y = std::stod(positionWaypointElement.childElement("y", 0).getText());
		double altitude = std::stod(positionWaypointElement.childElement("altitude", 0).getText());

		// Rough estimation equirectangular projection method.
		double longitude = referenceLongitude + x / (cos(referenceLatitude * std::numbers::pi / 180) * 111319.9);
		double latitude = referenceLatitude + y / 111319.9;
		double altitudeAboveGround = altitude - referenceAltitude;

		// Get the motionpath element
		XmlElement motionPathElement = element.childElement("motionpath", 0);

		// Extract the interpolation attribute
		string interpolation = XmlElement::getSafeAttribute(motionPathElement, "interpolation");

		// Determine if the interpolation is linear or cubic
		bool is_static = interpolation == "static";
		bool is_linear = interpolation == "linear";
		bool is_cubic = interpolation == "cubic";
		if (interpolation == "python")
		{
			std::cerr << "Warning: 'python' interpolation is not supported by the visualizer and will not be rendered."
				<< std::endl;
		}

		// Determine the type of placemark to use
		std::string placemarkStyle;
		if (element.childElement("receiver", 0).isValid()) { placemarkStyle = "receiver"; }
		else if (element.childElement("transmitter", 0).isValid()) { placemarkStyle = "transmitter"; }
		else if (element.childElement("target", 0).isValid()) { placemarkStyle = "target"; }

		// Determine if antenna 'pattern' is isotropic or patterned
		bool isIsotropic = false;

		if (XmlElement receiverElement = element.childElement("receiver", 0); receiverElement.isValid())
		{
			std::string antennaName = XmlElement::getSafeAttribute(receiverElement, "antenna");
			isIsotropic = isAntennaIsotropic(antennaName, isotropic_antennas);
		}
		else if (XmlElement transmitterElement = element.childElement
			(
				"transmitter",
				0
				); transmitterElement.isValid())
		{
			std::string antennaName = XmlElement::getSafeAttribute(transmitterElement, "antenna");
			isIsotropic = isAntennaIsotropic(antennaName, isotropic_antennas);
		}

		// Testing if 'isIsotropic' set to True if pattern = isotropic
		// std::cout << isIsotropic << std::endl;

		// If the associated pattern is isotropic, add a circular ring of radius 20 km
		if (isIsotropic)
		{
			double circle_radius = 20; // Radius in km
			int num_points = 100; // Number of points to form the circle

			kmlFile << "<Placemark>\n";
			kmlFile << "    <name>Isotropic pattern range</name>\n";
			kmlFile << "    <styleUrl>#translucentPolygon</styleUrl>\n";
			std::vector<std::pair<double, double>> circle_coordinates = generate_circle_coordinates
				(latitude, longitude, circle_radius, num_points);
			kmlFile << "    <Polygon>\n";
			kmlFile << "        <extrude>1</extrude>\n";
			kmlFile << "        <altitudeMode>relativeToGround</altitudeMode>\n";
			kmlFile << "        <outerBoundaryIs>\n";
			kmlFile << "            <LinearRing>\n";
			kmlFile << "                <coordinates>\n";

			for (const auto& coord : circle_coordinates)
			{
				kmlFile << "                    " << coord.second << "," << coord.first << "," << altitudeAboveGround <<
					"\n";
			}
			// Close the circle by repeating the first point
			kmlFile << "                    " << circle_coordinates[0].second << "," << circle_coordinates[0].first <<
				"," << altitudeAboveGround << "\n";

			kmlFile << "                </coordinates>\n";
			kmlFile << "            </LinearRing>\n";
			kmlFile << "        </outerBoundaryIs>\n";
			kmlFile << "    </Polygon>\n";
			kmlFile << "</Placemark>\n";
		}
		else if (!isIsotropic && (element.childElement("transmitter", 0).isValid() || element.childElement
			("receiver", 0)
			.isValid()))
		{
			// Extract necessary values
			double startAzimuth = std::stod(element.childElement("startazimuth", 0).getText());
			XmlElement posWaypointElement = element.childElement("positionwaypoint", 0);
			if (!posWaypointElement.isValid())
			{
				posWaypointElement = element.childElement("motionpath", 0).childElement("positionwaypoint", 0);
			}

			// Convert positionWaypoint to coordinates
			std::string coordinates = getCoordinatesFromPositionWaypoint
				(posWaypointElement, referenceLatitude, referenceLongitude, referenceAltitude);

			// Calculate end coordinates
			double arrowLength = 20000; // Adjust this value according to the desired length of the arrow

			// Parse coordinates from the positionWaypoint
			double startLatitude, startLongitude, startAltitude;
			std::istringstream coordinatesStream(coordinates);
			coordinatesStream >> startLongitude;
			coordinatesStream.ignore(1); // skip the comma
			coordinatesStream >> startLatitude;
			coordinatesStream.ignore(1); // skip the comma
			coordinatesStream >> startAltitude;

			// Adjust startAzimuth to make 0 degrees point East
			startAzimuth = startAzimuth + 180;

			// Calculate end coordinates
			double destLatitude, destLongitude;
			calculateDestinationCoordinate
				(startLatitude, startLongitude, startAzimuth, arrowLength, destLatitude, destLongitude);

			std::stringstream endCoordinatesStream;
			endCoordinatesStream << std::fixed << std::setprecision
				(6) << destLongitude << "," << destLatitude << "," << startAltitude;
			std::string endCoordinates = endCoordinatesStream.str();

			// Extract the antenna element with pattern="sinc"
			if (auto sincAntennaElementOpt = getAntennaElementWithSincPattern(document.getRootElement()))
			{
				XmlElement sincAntennaElement = sincAntennaElementOpt.value();
				double alpha = std::stod(sincAntennaElement.childElement("alpha", 0).getText());
				double beta = std::stod(sincAntennaElement.childElement("beta", 0).getText());
				double gamma = std::stod(sincAntennaElement.childElement("gamma", 0).getText());

				double angle_3dB_drop_deg = find_3db_drop_angle(alpha, beta, gamma);

				// Calculate end coordinates for both side lines
				double sideLine1Azimuth = startAzimuth - angle_3dB_drop_deg;
				double sideLine2Azimuth = startAzimuth + angle_3dB_drop_deg;
				double sideLine1DestLatitude, sideLine1DestLongitude;
				double sideLine2DestLatitude, sideLine2DestLongitude;

				calculateDestinationCoordinate
					(
						startLatitude, startLongitude, sideLine1Azimuth, arrowLength, sideLine1DestLatitude,
						sideLine1DestLongitude
						);
				calculateDestinationCoordinate
					(
						startLatitude, startLongitude, sideLine2Azimuth, arrowLength, sideLine2DestLatitude,
						sideLine2DestLongitude
						);

				std::stringstream sideLine1EndCoordinatesStream, sideLine2EndCoordinatesStream;
				sideLine1EndCoordinatesStream << std::fixed << std::setprecision
					(6) << sideLine1DestLongitude << "," << sideLine1DestLatitude << "," << startAltitude;
				sideLine2EndCoordinatesStream << std::fixed << std::setprecision
					(6) << sideLine2DestLongitude << "," << sideLine2DestLatitude << "," << startAltitude;
				std::string sideLine1EndCoordinates = sideLine1EndCoordinatesStream.str();
				std::string sideLine2EndCoordinates = sideLine2EndCoordinatesStream.str();

				// Add placemarks for side lines
				for (int i = 1; i <= 2; ++i)
				{
					std::string sideLineName = "Antenna Side Line " + std::to_string(i);
					std::string sideLineEndCoordinates = (i == 1) ? sideLine1EndCoordinates : sideLine2EndCoordinates;

					kmlFile << "<Placemark>\n";
					kmlFile << "      <name>" << sideLineName << "</name>\n";
					kmlFile << "      <styleUrl>#lineStyleBlue</styleUrl>\n";
					kmlFile << "      <LineString>\n";
					kmlFile << "            <tessellate>1</tessellate>\n";
					kmlFile << "            <coordinates>\n";
					kmlFile << "            " + coordinates + " " + sideLineEndCoordinates + "\n";
					kmlFile << "            </coordinates>\n";
					kmlFile << "      </LineString>\n";
					kmlFile << "</Placemark>\n";
				}
			}
			else { std::cerr << "Error: Antenna element with pattern='sinc' not found in the XML file" << std::endl; }

			kmlFile << "<Placemark>\n";
			kmlFile << "      <name>Antenna Direction</name>\n";
			kmlFile << "      <styleUrl>#lineStyle</styleUrl>\n";
			kmlFile << "      <LineString>\n";
			kmlFile << "            <tessellate>1</tessellate>\n";
			kmlFile << "            <coordinates>\n";
			kmlFile << "            " + coordinates + " " + endCoordinates + "\n";
			kmlFile << "            </coordinates>\n";
			kmlFile << "      </LineString>\n";
			kmlFile << "</Placemark>\n";

			kmlFile << "<Placemark>\n";
			kmlFile << "      <name>Antenna Arrow</name>\n";
			kmlFile << "      <styleUrl>#arrowStyle</styleUrl>\n";
			kmlFile << "      <Point>\n";
			kmlFile << "          <coordinates>" + endCoordinates + "</coordinates>\n";
			kmlFile << "      </Point>\n";
			kmlFile << "      <IconStyle>\n";
			kmlFile << "          <heading>" << startAzimuth << "</heading>\n";
			kmlFile << "      </IconStyle>\n";
			kmlFile << "</Placemark>\n";
		}

		// Write the placemark data to the KML file
		kmlFile << "<Placemark>\n";
		kmlFile << "    <name>" << XmlElement::getSafeAttribute(element, "name") << "</name>\n";
		// kmlFile << "    <description>" << XmlElement::getSafeAttribute(element, "description") << "</description>\n";

		// Check for monostatic element to assign a style URL.
		if (element.childElement("receiver", 0).isValid() || element.childElement("monostatic", 0).isValid())
		{
			kmlFile << "    <styleUrl>#receiver</styleUrl>\n";
		}
		else if (element.childElement("transmitter", 0).isValid())
		{
			kmlFile << "    <styleUrl>#transmitter</styleUrl>\n";
		}
		else if (element.childElement("target", 0).isValid()) { kmlFile << "    <styleUrl>#target</styleUrl>\n"; }

		// If the interpolation is dynamic, use the gx:Track element
		if (is_static || is_linear || is_cubic)
		{
			kmlFile << "    <gx:Track>\n";
			if (altitudeAboveGround > 0)
			{
				kmlFile << "        <altitudeMode>relativeToGround</altitudeMode>\n";
				kmlFile << "        <extrude>1</extrude>\n";
			}
			else { kmlFile << "        <altitudeMode>clampToGround</altitudeMode>\n"; }

			// Iterate through the position waypoints
			for (size_t i = 0; i < positionWaypointList.size(); ++i)
			{
				const XmlElement& pWaypointElement = positionWaypointList[i];

				// Extract the position coordinates
				double p_x = std::stod(pWaypointElement.childElement("x", 0).getText());
				double p_y = std::stod(pWaypointElement.childElement("y", 0).getText());
				double p_alt = std::stod(pWaypointElement.childElement("altitude", 0).getText());

				// Convert the position coordinates to geographic coordinates
				double p_lon = referenceLongitude + p_x / (cos(referenceLatitude * std::numbers::pi / 180) * 111319.9);
				double p_lat = referenceLatitude + p_y / 111319.9;
				double p_alt_ag = p_alt - referenceAltitude;

				// Extract the time value
				double time = std::stod(pWaypointElement.childElement("time", 0).getText());

				// Check if interpolation is cubic
				if (is_cubic && i + 1 < positionWaypointList.size())
				{
					const XmlElement& nextPositionWaypointElement = positionWaypointList[i + 1];
					double nextTime = std::stod(nextPositionWaypointElement.childElement("time", 0).getText());
					double time_diff = nextTime - time;

					double nextX = std::stod(nextPositionWaypointElement.childElement("x", 0).getText());
					double nextY = std::stod(nextPositionWaypointElement.childElement("y", 0).getText());
					double nextAltitude = std::stod(nextPositionWaypointElement.childElement("altitude", 0).getText());

					double nextLongitude = referenceLongitude + nextX / (cos
						(
							referenceLatitude * std::numbers::pi /
							180
							) * 111319.9);
					double nextLatitude = referenceLatitude + nextY / 111319.9;
					double nextAltitudeAboveGround = nextAltitude - referenceAltitude;

					int num_divisions = 100;
					for (int j = 0; j <= num_divisions; ++j)
					{
						double t = static_cast<double>(j) / num_divisions;
						double newLongitude, newLatitude;
						updateLongitudeLatitudeCubic
							(
								newLongitude, newLatitude, t, p_lon, p_lat, nextLongitude,
								nextLatitude
								);
						double newAltitudeAboveGround = p_alt_ag + t * (nextAltitudeAboveGround - p_alt_ag);
						kmlFile << "        <when>" << time + (static_cast<double>(j) * time_diff) / num_divisions <<
							"</when>\n";
						kmlFile << "        <gx:coord>" << newLongitude << " " << newLatitude << " " <<
							newAltitudeAboveGround << "</gx:coord>\n";
					}
				}
				else if (is_linear || is_static)
				{
					// For linear and static, just write the waypoint. Google Earth will connect for linear.
					// For static, this creates "jumps".
					kmlFile << "        <when>" << time << "</when>\n";
					kmlFile << "        <gx:coord>" << p_lon << " " << p_lat << " " << p_alt_ag << "</gx:coord>\n";
				}
			}

			kmlFile << "    </gx:Track>\n";
		}
		else
		{
			// Fallback for single-point platforms or unhandled interpolations
			kmlFile << "    <LookAt>\n";
			kmlFile << "        <longitude>" << longitude << "</longitude>\n";
			kmlFile << "        <latitude>" << latitude << "</latitude>\n";
			kmlFile << "        <altitude>" << altitudeAboveGround << "</altitude>\n";
			kmlFile << "        <heading>-148.4122922628044</heading>\n";
			kmlFile << "        <tilt>40.5575073395506</tilt>\n";
			kmlFile << "        <range>500.6566641072245</range>\n";
			kmlFile << "    </LookAt>\n";

			kmlFile << "    <Point>\n";
			kmlFile << "        <coordinates>" << longitude << "," << latitude << "," << altitudeAboveGround <<
				"</coordinates>\n";

			if (altitudeAboveGround > 0)
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
			// Get the first and last position waypoints
			const XmlElement& firstPositionWaypointElement = positionWaypointList.front();
			const XmlElement& lastPositionWaypointElement = positionWaypointList.back();

			// Extract the start and end coordinates
			std::string startCoordinates = getCoordinatesFromPositionWaypoint
				(firstPositionWaypointElement, referenceLatitude, referenceLongitude, referenceAltitude);
			std::string endCoordinates = getCoordinatesFromPositionWaypoint
				(lastPositionWaypointElement, referenceLatitude, referenceLongitude, referenceAltitude);

			// Start point placemark
			kmlFile << "<Placemark>\n";
			kmlFile << "    <name>Start: " << XmlElement::getSafeAttribute(element, "name") << "</name>\n";
			kmlFile << "    <styleUrl>#target</styleUrl>\n"; // Replace with your desired style URL for the start icon
			kmlFile << "    <Point>\n";
			kmlFile << "        <coordinates>" << startCoordinates << "</coordinates>\n";
			if (altitudeAboveGround > 0)
			{
				kmlFile << "        <altitudeMode>relativeToGround</altitudeMode>\n";
				kmlFile << "        <extrude>1</extrude>\n";
			}
			else { kmlFile << "        <altitudeMode>clampToGround</altitudeMode>\n"; }
			kmlFile << "    </Point>\n";
			kmlFile << "</Placemark>\n";

			// End point placemark
			kmlFile << "<Placemark>\n";
			kmlFile << "    <name>End: " << XmlElement::getSafeAttribute(element, "name") << "</name>\n";
			kmlFile << "    <styleUrl>#target</styleUrl>\n"; // Replace with your desired style URL for the end icon
			kmlFile << "    <Point>\n";
			kmlFile << "        <coordinates>" << endCoordinates << "</coordinates>\n";
			if (altitudeAboveGround > 0)
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

// Function to traverse the DOMNode by recursively calling itself and processElement()
void traverseDOMNode
(
	xmlNode* node, std::ofstream& kmlFile, double referenceLatitude, double referenceLongitude,
	double referenceAltitude, const XmlDocument& document
	)
{
	for (xmlNode* cur_node = node; cur_node; cur_node = cur_node->next)
	{
		if (cur_node->type == XML_ELEMENT_NODE)
		{
			const XmlElement element(cur_node);
			if (element.name() == "platform")
			{
				processElement(element, kmlFile, referenceLatitude, referenceLongitude, referenceAltitude, document);
			}
			else
			{
				traverseDOMNode
					(
						cur_node->children, kmlFile, referenceLatitude, referenceLongitude, referenceAltitude,
						document
						);
			}
		}
	}
}

// Main function
int main(int argc, char* argv[])
{
	if (argc < 3 || argc > 6)
	{
		std::cerr << "Usage: " << argv[0] <<
			" <input XML file> <output KML file> [<referenceLatitude> <referenceLongitude> <referenceAltitude>]"
			<<
			std::endl;
		return 1;
	}

	// Setting default geographical and altitude coordinates
	double referenceLatitude = -33.9545;
	double referenceLongitude = 18.4563;
	double referenceAltitude = 0;

	// Update file_path with command line argument
	string file_path = argv[1];
	// Setting mode to evironment variable from command line
	string output_file = argv[2];
	// Setting georgraphical coordinates to command line input
	if (argc == 6)
	{
		try
		{
			referenceLatitude = std::stod(argv[3]);
			referenceLongitude = std::stod(argv[4]);
			referenceAltitude = std::stod(argv[5]);
		}
		catch (const std::invalid_argument& e)
		{
			std::cerr <<
				"Error: Invalid argument. Please provide valid numbers for referenceLatitude, referenceLongitude, and referenceAltitude.\n";
			return 1;
		}
		catch (const std::out_of_range& e)
		{
			std::cerr <<
				"Error: Out of range. Please provide valid numbers for referenceLatitude, referenceLongitude, and referenceAltitude.\n";
			return 1;
		}
	}

	try
	{
		XmlDocument document;
		if (!document.loadFile(file_path))
		{
			std::cerr << "Error: Could not load or parse XML file " << file_path << std::endl;
			return 1;
		}

		XmlElement rootElement = document.getRootElement();
		if (!rootElement.isValid())
		{
			std::cerr << "Error: root element not found" << std::endl;
			return 1;
		}

		std::ofstream kmlFile(output_file.c_str());
		if (!kmlFile.is_open())
		{
			std::cerr << "Error opening output KML file" << std::endl;
			return 1;
		}

		// Write the KML header
		kmlFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
		kmlFile << "<kml xmlns=\"http://www.opengis.net/kml/2.2\" xmlns:gx=\"http://www.google.com/kml/ext/2.2\">\n";
		kmlFile << "<Document>\n";
		kmlFile << "<name>" << file_path << "</name>\n";

		// KML styles appended to document
		kmlFile << "<Style id=\"receiver\">\n";
		kmlFile << "  <IconStyle>\n";
		kmlFile << "    <Icon>\n";
		kmlFile << "      <href>https://cdn-icons-png.flaticon.com/512/645/645436.png</href>\n";
		kmlFile << "    </Icon>\n";
		kmlFile << "  </IconStyle>\n";
		kmlFile << "</Style>\n";
		kmlFile << "<Style id=\"transmitter\">\n";
		kmlFile << "  <IconStyle>\n";
		kmlFile << "    <Icon>\n";
		kmlFile << "      <href>https://cdn-icons-png.flaticon.com/128/224/224666.png</href>\n";
		kmlFile << "    </Icon>\n";
		kmlFile << "  </IconStyle>\n";
		kmlFile << "</Style>\n";
		kmlFile << "<Style id=\"target\">\n";
		kmlFile << "  <IconStyle>\n";
		kmlFile << "    <Icon>\n";
		kmlFile <<
			"      <href>https://upload.wikimedia.org/wikipedia/commons/thumb/a/ad/Target_red_dot1.svg/1200px-Target_red_dot1.svg.png</href>\n";
		kmlFile << "    </Icon>\n";
		kmlFile << "  </IconStyle>\n";
		kmlFile << "  <LineStyle>\n";
		kmlFile << "    <width>2</width>\n";
		kmlFile << "  </LineStyle>\n";
		kmlFile << "</Style>\n";
		kmlFile << "<Style id=\"translucentPolygon\">\n";
		kmlFile << "    <LineStyle>\n";
		kmlFile << "        <color>ff0000ff</color>\n";
		kmlFile << "        <width>2</width>\n";
		kmlFile << "    </LineStyle>\n";
		kmlFile << "    <PolyStyle>\n";
		kmlFile << "        <color>00ffffff</color> <!-- RGBA: 50% transparent white --> \n";
		kmlFile << "     </PolyStyle>\n";
		kmlFile << "</Style>\n";
		kmlFile << "<Style id=\"arrowStyle\">\n";
		kmlFile << "    <IconStyle>\n";
		kmlFile << "        <Icon>\n";
		kmlFile << "            <href>http://maps.google.com/mapfiles/kml/shapes/arrow.png</href>\n";
		kmlFile << "        </Icon>\n";
		kmlFile << "        <scale>0.5</scale>\n";
		kmlFile << "    </IconStyle>\n";
		kmlFile << "</Style>\n";
		kmlFile << "<Style id=\"lineStyle\">\n";
		kmlFile << "    <LineStyle>\n";
		kmlFile << "        <color>ff0000ff</color>\n";
		kmlFile << "        <width>2</width>\n";
		kmlFile << "     </LineStyle>\n";
		kmlFile << "</Style>\n";
		kmlFile << "<Style id=\"lineStyleBlue\">\n";
		kmlFile << "    <LineStyle>\n";
		kmlFile << "        <color>ffff0000</color>\n";
		kmlFile << "        <width>2</width>\n";
		kmlFile << "     </LineStyle>\n";
		kmlFile << "</Style>\n";

		// Folder element appended
		kmlFile << "<Folder>\n";
		kmlFile << "  <name>Reference Coordinate</name>\n";
		kmlFile <<
			"  <description>Placemarks for various elements in the FERSXML file. All Placemarks are situated relative to this reference point.</description>\n";

		// Add the LookAt element with given values
		kmlFile << "  <LookAt>\n";
		kmlFile << "    <longitude>" << referenceLongitude << "</longitude>\n";
		kmlFile << "    <latitude>" << referenceLatitude << "</latitude>\n";
		kmlFile << "    <altitude>" << referenceAltitude << "</altitude>\n";
		kmlFile << "    <heading>-148.4122922628044</heading>\n";
		kmlFile << "    <tilt>40.5575073395506</tilt>\n";
		kmlFile << "    <range>10000</range>\n";
		kmlFile << "  </LookAt>\n";

		// Traverse DOMNode and output extracted FERSXML data:
		traverseDOMNode
			(
				rootElement.getNode(), kmlFile, referenceLatitude, referenceLongitude, referenceAltitude,
				document
				);

		// Close the Folder and Document elements
		kmlFile << "</Folder>\n";
		kmlFile << "</Document>\n";
		kmlFile << "</kml>\n";

		kmlFile.close();
	}
	catch (const XmlException& e)
	{
		cerr << "Error parsing XML: " << e.what() << endl;
		return 1;
	}
	catch (...)
	{
		cerr << "Unknown error occurred while parsing XML." << endl;
		return 1;
	}

	return 0;
}
