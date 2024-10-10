/**
 * @file logging.h
 * @brief Header file for the logging system.
 *
 * This file defines a logging system with multiple log levels, supporting thread-safe logging
 * to a file or standard output, and allowing formatted log messages with source location details.
 * The logging system is customizable with log levels and file output for effective debugging
 * and tracking of events in an application.
 *
 * @author David Young
 * @date 2024-09-20
 */

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
	 * @class Level
	 * @brief Enum class representing the log levels.
	 *
	 * This enumeration defines different logging levels that specify the severity or importance of the log messages.
	 * These levels are used to filter messages based on their relevance to the current logging configuration.
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
	 * @class Logger
	 * @brief Logger class for handling logging operations.
	 *
	 * This class manages logging functionality, including setting log levels, formatting log messages,
	 * and optionally writing them to a file.
	 * It supports thread-safe operations with the use of a mutex,
	 * and logs messages with optional source location details for better traceability.
	 *
	 * Example usage:
	 * @code
	 * Logger logger;
	 * logger.setLevel(Level::DEBUG);
	 * logger.log(Level::INFO, "Application started");
	 * @endcode
	 */
	class Logger
	{
	public:
		/**
		 * @brief Default constructor.
		 *
		 * Initializes the logger with default settings. The default log level is set to INFO,
		 * and no file output is configured initially.
		 */
		Logger() = default;

		/**
		 * @brief Destructor that ensures the log file is closed using RAII.
		 *
		 * If the logger is writing to a file, the file is closed when the logger object is destroyed.
		 */
		~Logger() noexcept = default;

		/**
		 * @brief Sets the logging level.
		 *
		 * Updates the logging level to filter log messages based on their severity.
		 * Only messages with severity equal to or higher than the set level are logged.
		 *
		 * @param level The logging level to set.
		 */
		void setLevel(const Level level) noexcept { _log_level = level; }

		/**
		 * @brief Logs a message with a specific log level and source location.
		 *
		 * This function logs a message, including details about where the log was called (file, line, and function).
		 * The message is logged only if the log level is greater than or equal to the current log level.
		 *
		 * @param level The log level.
		 * @param message The message to log.
		 * @param location The source location of the log call.
		 */
		void log(Level level, const std::string& message,
		         const std::source_location& location = std::source_location::current()) noexcept;

		/**
		 * @brief Logs a formatted message with a specific log level and source location.
		 *
		 * This templated function logs a formatted message, using variadic arguments for formatting.
		 * It constructs the message using the format string and arguments, then passes it to the log function.
		 *
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
		 *
		 * Configures the logger to write log messages to a specified file.
		 * If the file cannot be opened, an error message is returned.
		 *
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
		 *
		 * Generates a timestamp string to be included in the log message, representing the time
		 * the log event occurred.
		 *
		 * @return The current timestamp.
		 */
		static std::string getCurrentTimestamp() noexcept;
	};

	/**
	 * @brief Externally available logger object.
	 *
	 * A global instance of the Logger class, allowing logging to be done without explicitly
	 * creating a Logger object in various parts of the application.
	 */
	extern Logger logger;

	/**
	 * @brief Converts a log level enum value to its string representation.
	 *
	 * This inline function maps the Level enum values to corresponding string labels for
	 * use in log messages.
	 *
	 * @param level The log level.
	 * @return A string representing the log level.
	 */
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
	 *
	 * This function provides a simple interface for logging a formatted message, allowing
	 * the user to specify the log level, format string, and arguments.
	 * The message is formatted and passed to the logger for logging.
	 *
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
