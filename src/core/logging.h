// logging.h
// Created by David Young on 9/20/24.
//

#pragma once

#define LOG(level, ...) log(level, std::source_location::current(), __VA_ARGS__)

#include <expected>
#include <format>
#include <fstream>
#include <mutex>
#include <source_location>
#include <string>
#include <utility>

namespace logging
{
	/**
	 * @brief Enum class representing the log levels.
	 */
	enum class Level
	{
		TRACE, ///< Trace level for detailed debugging information.
		DEBUG, ///< Debug level for general debugging information.
		INFO, ///< Info level for informational messages.
		WARNING, ///< Warning level for potentially harmful situations.
		ERROR, ///< Error level for error events.
		FATAL ///< Fatal level for severe error events.
	};

	/**
	 * @brief Logger class for handling logging operations.
	 */
	class Logger
	{
	public:
		/**
		 * @brief Default constructor.
		 */
		Logger() = default;

		/**
		 * @brief Destructor that Destructor that ensures the log file is closed using RAII.
		 */
		~Logger() noexcept = default;

		/**
		 * @brief Sets the logging level.
		 * @param level The logging level to set.
		 */
		void setLevel(const Level level) noexcept { _log_level = level; }

		/**
		 * @brief Logs a message with a specific log level and source location.
		 * @param level The log level.
		 * @param message The message to log.
		 * @param location The source location of the log call.
		 */
		void log(Level level, const std::string& message,
		         const std::source_location& location = std::source_location::current()) noexcept;

		/**
		 * @brief Logs a formatted message with a specific log level and source location.
		 * @tparam Args Variadic template for format arguments.
		 * @param level The log level.
		 * @param location The source location of the log call.
		 * @param formatStr The format string.
		 * @param args The format arguments.
		 */
		template <typename... Args>
		void log(const Level level, const std::source_location& location, const std::string& formatStr,
		         Args&&... args) noexcept
		{
			if (level >= _log_level)
			{
				const std::string message = std::vformat(formatStr, std::make_format_args(args...));
				log(level, message, location);
			}
		}

		/**
		 * @brief Sets the log file path to log messages to a file.
		 * @param filePath The path to the log file.
		 * @return std::expected indicating success or error message on failure.
		 */
		std::expected<void, std::string> logToFile(const std::string& filePath) noexcept;

	private:
		Level _log_level = Level::INFO; ///< Current log level.
		std::optional<std::ofstream> _log_file; ///< Output file stream for logging to a file.
		std::mutex _log_mutex; ///< Mutex for thread-safe logging.

		/**
		 * @brief Gets the current timestamp as a string.
		 * @return The current timestamp.
		 */
		static std::string getCurrentTimestamp() noexcept;
	};

	/**
	 * @brief Externally available logger object.
	 */
	extern Logger logger;

	inline std::string getLevelString(const Level level) noexcept
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
	 * @brief Logs a formatted message with a specific log level and source location.
	 * @tparam Args Variadic template for format arguments.
	 * @param level The log level.
	 * @param location The source location of the log call.
	 * @param formatStr The format string.
	 * @param args The format arguments.
	 */
	template <typename... Args>
	void log(Level level, const std::source_location& location, const std::string& formatStr, Args&&... args) noexcept
	{
		logger.log(level, location, formatStr, std::forward<Args>(args)...);
	}
}
