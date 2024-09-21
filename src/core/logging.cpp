// logging.cpp
// Created by David Young on 9/20/24.
//

#include "logging.h"

#include <filesystem>
#include <iostream>

namespace logging
{
	Logger logger; // Initialize global logger

	Logger::Logger() : _log_level(Level::DEBUG) {}

	Logger::~Logger() { if (_log_file.is_open()) { _log_file.close(); } }

	void Logger::setLevel(const Level level) { _log_level = level; }

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

	void Logger::log(const Level level, const std::string& message, const std::source_location location)
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
				<< "[" << std::setw(7) << std::left << getLevelString(level) << "] " // Log level (fixed width of 8)
				<< "[" << std::setw(30) << std::left << file_line << "] " // Filename:Line (fixed width)
				<< message << std::endl;

			// Log to console
			std::cerr << oss.str();

			// Log to file if opened
			if (_log_file.is_open()) { _log_file << oss.str(); }
		}
	}

	void Logger::logToFile(const std::string& filePath)
	{
		std::scoped_lock lock(_log_mutex);
		if (_log_file.is_open()) { _log_file.close(); }
		_log_file.open(filePath, std::ios::out | std::ios::app);
		if (!_log_file) { throw std::runtime_error("Unable to open log file: " + filePath); }
	}

	void Logger::stopLoggingToFile()
	{
		std::scoped_lock lock(_log_mutex);
		if (_log_file.is_open()) { _log_file.close(); }
	}
}
