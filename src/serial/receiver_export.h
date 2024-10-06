// received_export.h
// Export receiver data to various formats
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

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

	void exportReceiverXml(std::span<const std::unique_ptr<Response>> responses, const std::string& filename);

	void exportReceiverBinary(std::span<const std::unique_ptr<Response>> responses, const radar::Receiver* recv, const std::string& recvName);

	void exportReceiverCsv(std::span<const std::unique_ptr<Response>> responses, const std::string& filename);
}
