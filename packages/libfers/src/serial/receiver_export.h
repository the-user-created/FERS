// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2006-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
* @file receiver_export.h
* @brief Export receiver data to various formats.
*/

#pragma once

#include <memory>
#include <span>
#include <string>

namespace pool
{
	class ThreadPool;
}

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
	* @param responses A span of unique pointers to Response objects representing the data to be exported.
	* @param filename The name of the XML file to export the data into.
	* @throws std::runtime_error If the file cannot be saved.
	*/
	void exportReceiverXml(std::span<const std::unique_ptr<Response>> responses, const std::string& filename);

	/**
	* @brief Exports receiver responses to a binary format.
	*
	* @param responses A span of unique pointers to Response objects representing the data to be exported.
	* @param recv A pointer to the radar::Receiver object containing receiver data.
	* @param recvName The name of the receiver, which will be used as the filename for the binary export.
	* @param pool A reference to the ThreadPool object for parallel processing.
	* @throws std::runtime_error If the file cannot be saved or data cannot be written to it.
	*/
	void exportReceiverBinary(std::span<const std::unique_ptr<Response>> responses, radar::Receiver* recv,
	                          const std::string& recvName, pool::ThreadPool& pool);

	/**
	* @brief Exports receiver CW data to a binary HDF5 file.
	*
	* @param recv A pointer to the radar::Receiver object containing CW IQ data.
	* @param recvName The name of the receiver, which will be used as the filename for the binary export.
	* @throws std::runtime_error If the file cannot be saved or data cannot be written to it.
	*/
	void exportReceiverCwBinary(const radar::Receiver* recv, const std::string& recvName);

	/**
	* @brief Exports receiver responses to CSV files.
	*
	* @param responses A span of unique pointers to Response objects representing the data to be exported.
	* @param filename The base name for the CSV file. Transmitter-specific names will be appended.
	* @throws std::runtime_error If a file cannot be opened for writing.
	*/
	void exportReceiverCsv(std::span<const std::unique_ptr<Response>> responses, const std::string& filename);
}
