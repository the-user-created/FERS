/**
* @file receiver_export.h
* @brief Export receiver data to various formats.
*
* This header defines functions for exporting receiver data to different formats such as XML, Binary, and CSV.
* It supports serialization of response data into structured formats for further processing or storage.
*
* @authors David Young, Marc Brooker
* @date 2024-09-13
*/

#pragma once

#include <memory>
#include <span>
#include <string>

namespace radar
{
	class Receiver;
}

namespace serial
{
	class Response;

	/**
	* @brief Exports receiver responses to an XML file.
	*
	* Serializes the given responses into XML format and saves them to the specified file.
	*
	* @param responses A span of unique pointers to Response objects representing the data to be exported.
	* @param filename The name of the XML file to export the data into.
	* @throws std::runtime_error If the file cannot be saved.
	*/
	void exportReceiverXml(std::span<const std::unique_ptr<Response>> responses, const std::string& filename);

	/**
	* @brief Exports receiver responses to a binary format.
	*
	* Serializes the given responses along with receiver data into a binary format using HDF5
	* and saves them to the specified file.
	*
	* @param responses A span of unique pointers to Response objects representing the data to be exported.
	* @param recv A pointer to the radar::Receiver object containing receiver data.
	* @param recvName The name of the receiver, which will be used as the filename for the binary export.
	* @throws std::runtime_error If the file cannot be saved or data cannot be written to it.
	*/
	void exportReceiverBinary(std::span<const std::unique_ptr<Response>> responses, const radar::Receiver* recv, const std::string& recvName);

	/**
	* @brief Exports receiver responses to CSV files.
	*
	* Serializes the given responses into CSV format and saves them to multiple CSV files. Each response
	* is exported to a separate CSV file corresponding to the transmitter name.
	*
	* @param responses A span of unique pointers to Response objects representing the data to be exported.
	* @param filename The base name for the CSV file. Transmitter-specific names will be appended.
	* @throws std::runtime_error If a file cannot be opened for writing.
	*/
	void exportReceiverCsv(std::span<const std::unique_ptr<Response>> responses, const std::string& filename);
}
