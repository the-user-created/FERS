// received_export.h
// Export receiver data to various formats
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef RECEIVER_EXPORT_H
#define RECEIVER_EXPORT_H

#include <memory>
#include <vector>

namespace radar
{
	class Receiver;
}

namespace serial
{
	class Response;

	void exportReceiverXml(const std::vector<std::unique_ptr<Response>>& responses, const std::string& filename);

	void exportReceiverBinary(const std::vector<std::unique_ptr<Response>>& responses, const radar::Receiver* recv,
	                          const std::string& recvName);

	void exportReceiverCsv(const std::vector<std::unique_ptr<Response>>& responses, const std::string& filename);
}

#endif //RECEIVER_EXPORT_H
