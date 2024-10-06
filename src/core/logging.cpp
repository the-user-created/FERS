// logging.cpp
// Created by David Young on 9/20/24.
//

#include "logging.h"

#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <bits/chrono.h>

namespace logging
{
	/**
	 * @brief Global logger object.
	 */
	Logger logger;

	/**
	 * @brief Converts a log level to its string representation.
	 * @param level The log level.
	 * @return The string representation of the log level.
	 */
	std::string Logger::getLevelString(const Level level)
	{
		switch (level)
		{
		case Level::TRACE: return "TRACE";
		case Level::DEBUG: return "DEBUG";
		case Level::INFO: return "INFO";
		case Level::WARNING: return "WARNING";
		case Level::ERROR: return "ERROR";
		case Level::FATAL: return "FATAL";
		default: return "UNKNOWN";
		}
	}

	/**
	 * @brief Gets the current timestamp as a string.
	 * @return The current timestamp.
	 */
	std::string Logger::getCurrentTimestamp()
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
	void Logger::log(const Level level, const std::string& message, const std::source_location& location)
	{
		if (level >= _log_level)
		{
			std::scoped_lock lock(_log_mutex);

			// Extract only the filename from the full path
			const std::string filename = std::filesystem::path(location.file_name()).filename().string();

			const std::string file_line = filename + ":" + std::to_string(location.line());

			std::ostringstream oss;
			// Format each component with fixed width
			oss << "[" << getCurrentTimestamp() << "] "
				<< "[" << std::setw(7) << std::left << getLevelString(level) << "] "
				<< "[" << std::setw(30) << std::left << file_line << "] "
				<< message << std::endl;

			// Log to console
			std::cerr << oss.str();

			// Log to file if opened
			if (_log_file.is_open()) { _log_file << oss.str(); }
		}
	}

	/**
	 * @brief Sets the log file path to log messages to a file. This will overwrite the existing log file.
	 * @param filePath The path to the log file.
	 */
	void Logger::logToFile(const std::string& filePath)
	{
		std::scoped_lock lock(_log_mutex);
		if (_log_file.is_open()) { _log_file.close(); }
		_log_file.open(filePath, std::ios::out | std::ios::trunc);
		if (!_log_file) { throw std::runtime_error("Unable to open log file: " + filePath); }
	}

	/**
	 * @brief Stops logging to the file.
	 */
	void Logger::stopLoggingToFile()
	{
		std::scoped_lock lock(_log_mutex);
		if (_log_file.is_open()) { _log_file.close(); }
	}
}
