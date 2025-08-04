#pragma once

#include <string>

namespace core
{
	class World;
}

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
	 *   The origin's altitude is considered its Mean Sea Level (MSL) altitude.
	 *   If this tag is not present, a default location is used.
	 *
	 * - Coordinate Interpretation: The local `(x, y)` coordinates from `<positionwaypoint>`
	 *   are treated as meters in an East-North-Up (ENU) tangent plane centered at the origin.
	 *
	 * - Altitude Interpretation: The `<altitude>` value in a `<positionwaypoint>` is interpreted
	 *   as a relative offset in the local "up" direction (z-axis) from the geodetic origin.
	 *   The final absolute altitude (MSL) is calculated by GeographicLib based on this offset
	 *   and the origin's altitude. The KML is then written with `<altitudeMode>absolute</altitudeMode>`.
	 */
	class KmlGenerator
	{
	public:
		/**
		 * @brief Generates a KML file from a pre-built simulation world.
		 *
		 * @param world The simulation world containing all objects and paths.
		 * @param outputKmlPath The path for the output KML file.
		 * @return True on success, false on failure.
		 */
		static bool generateKml(const core::World& world, const std::string& outputKmlPath);
	};
}
