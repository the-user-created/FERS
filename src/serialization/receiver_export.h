// received_export.h
// Export receiver data to various formats
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef RECEIVER_EXPORT_H
#define RECEIVER_EXPORT_H

#include <string>
#include <vector>

namespace rs
{
	// Forward declarations
	class Receiver;
	class Response;
}

namespace receiver_export
{
	void exportReceiverXml(const std::vector<rs::Response*>& responses, const std::string& filename);

	void exportReceiverBinary(const std::vector<rs::Response*>& responses, const rs::Receiver* recv,
	                          const std::string& recvName);

	void exportReceiverCsv(const std::vector<rs::Response*>& responses, const std::string& filename);
}

#endif //RECEIVER_EXPORT_H
