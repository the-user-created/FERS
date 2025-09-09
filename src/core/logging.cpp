/**
 * @file logging.cpp
 * @brief Implementation of the logging system.
 *
 * @author David Young
 * @date 2024-09-20
 */

#include "logging.h"

#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <chrono>

namespace logging
{
	Logger logger;

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

	void Logger::log(const Level level, const std::string& message, const std::source_location& location) noexcept
	{
		if (level >= _log_level)
		{
			std::scoped_lock lock(_log_mutex);

			const std::string filename = std::filesystem::path(location.file_name()).filename().string();
			const std::string file_line = filename + ":" + std::to_string(location.line());

			std::ostringstream oss;
			oss << "[" << getCurrentTimestamp() << "] "
				<< "[" << std::setw(7) << std::left << getLevelString(level) << "] "
				<< "[" << std::setw(30) << std::left << file_line << "] "
				<< message << std::endl;

			std::cerr << oss.str();

			if (_log_file && _log_file->is_open()) { *_log_file << oss.str(); }
		}
	}

	std::expected<void, std::string> Logger::logToFile(const std::string& filePath) noexcept
	{
		std::scoped_lock lock(_log_mutex);

		std::ofstream file(filePath, std::ios::out | std::ios::trunc);
		if (!file) { return std::unexpected("Unable to open log file: " + filePath); }

		_log_file = std::move(file);
		return {};
	}
}
