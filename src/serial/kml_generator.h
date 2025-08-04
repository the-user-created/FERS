#pragma once

#include <string>

namespace serial
{
	/**
	 * @class KmlGenerator
	 * @brief Generates KML files from FERS simulation scenarios for geographical visualization.
	 *
	 * This class reads a FERS XML scenario and interprets its local Cartesian coordinates
	 * within a geodetic (WGS84) framework. It makes the following key assumptions:
	 *
	 * - Geodetic Origin: The simulation's local origin (0,0,0) is anchored to a
	 *   geographical point defined by the `<origin>` tag in the XML's `<parameters>` section.
	 *   If this tag is not present, a default location is used.
	 *
	 * - Coordinate Interpretation: The local `(x, y)` coordinates from `<positionwaypoint>`
	 *   are treated as meters in an East-North-Up (ENU) tangent plane centered at the origin.
	 *
	 * - Altitude Interpretation: The `<altitude>` value in a `<positionwaypoint>` is interpreted
	 *   as the absolute altitude in meters (e.g., above Mean Sea Level or the WGS84 ellipsoid).
	 *   It is NOT a height relative to the ground.
	 */
	class KmlGenerator
	{
	public:
		/**
		 * @brief Generates a KML file for the given simulation scenario.
		 *
		 * @param inputXmlPath The path to the input FERS XML file.
		 * @param outputKmlPath The path for the output KML file.
		 * @return True on success, false on failure.
		 */
		static bool generateKml(const std::string& inputXmlPath, const std::string& outputKmlPath);
	};

} // namespace serial
