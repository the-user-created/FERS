#pragma once

#include <string>

namespace serial
{
	/**
	 * @class KmlGenerator
	 * @brief Generates KML files from FERS simulation scenarios.
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
