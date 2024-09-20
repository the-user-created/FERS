// logging.h
// Created by David Young on 9/20/24.
//

#pragma once

#include <format>
#include <fstream>
#include <mutex>
#include <source_location>
#include <string>

#define LOG(level, ...) logging::log(level, std::source_location::current(), __VA_ARGS__)

namespace logging
{
	enum class Level
	{
		VV,
		VERBOSE,
		INFO,
		CRITICAL,
		WARNING,
		ERROR
	};

	class Logger
	{
	public:
		Logger();

		~Logger();

		void setLevel(Level level);

		void log(Level level, const std::string& message,
		         std::source_location location = std::source_location::current());

		template <typename... Args>
		void log(const Level level, const std::source_location location, const std::string& formatStr, Args&&... args)
		{
			if (level >= _log_level)
			{
				const std::string message = std::vformat(formatStr, std::make_format_args(args...));
				log(level, message, location);
			}
		}

		void logToFile(const std::string& filePath);

		void stopLoggingToFile();

	private:
		Level _log_level;
		std::ofstream _log_file;
		std::mutex _log_mutex;

		static std::string getLevelString(Level level);

		static std::string getCurrentTimestamp();
	};

	extern Logger logger; // Externally available logger object

	template <typename... Args>
	void log(Level level, const std::source_location location, const std::string& formatStr, Args&&... args)
	{
		logger.log(level, location, formatStr, std::forward<Args>(args)...);
	}
}
