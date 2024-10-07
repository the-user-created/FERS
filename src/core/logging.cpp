// logging.cpp
// Created by David Young on 9/20/24.
//

#include "logging.h"

#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <bits/chrono.h>

namespace logging
{
	/**
	 * @brief Global logger object.
	 */
	Logger logger;

	/**
	 * @brief Gets the current timestamp as a string.
	 * @return The current timestamp.
	 */
	std::string Logger::getCurrentTimestamp() noexcept
	{
		const auto now = std::chrono::system_clock::now();
		const std::time_t time = std::chrono::system_clock::to_time_t(now);
		std::tm tm{};
		localtime_r(&time, &tm);

		std::ostringstream oss;
		oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
		return oss.str();
	}

	/**
	 * @brief Logs a message with a specific log level and source location.
	 * @param level The log level.
	 * @param message The message to log.
	 * @param location The source location of the log call.
	 */
	void Logger::log(const Level level, const std::string& message, const std::source_location& location) noexcept
	{
		if (level >= _log_level)
		{
			std::scoped_lock lock(_log_mutex);

			// Extract only the filename from the full path
			const std::string filename = std::filesystem::path(location.file_name()).filename().string();
			const std::string file_line = filename + ":" + std::to_string(location.line());

			std::ostringstream oss;
			oss << "[" << getCurrentTimestamp() << "] "
				<< "[" << std::setw(7) << std::left << getLevelString(level) << "] "
				<< "[" << std::setw(30) << std::left << file_line << "] "
				<< message << std::endl;

			// Log to console
			std::cerr << oss.str();

			// Log to file if opened
			if (_log_file && _log_file->is_open()) { (*_log_file) << oss.str(); }
		}
	}

	/**
	 * @brief Attempts to set the log file path to log messages to a file.
	 * @param filePath The path to the log file.
	 * @return std::expected indicating success or error message on failure.
	 */
	std::expected<void, std::string> Logger::logToFile(const std::string& filePath) noexcept
	{
		std::scoped_lock lock(_log_mutex);

		std::ofstream file(filePath, std::ios::out | std::ios::trunc);
		if (!file) { return std::unexpected("Unable to open log file: " + filePath); }

		_log_file = std::move(file);
		return {};
	}
}
