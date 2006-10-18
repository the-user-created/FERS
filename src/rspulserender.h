//rspulserender.h
//Definitions for pulse rendering functions
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//7 June 2006

#ifndef __RS_PULSE_RENDER
#define __RS_PULSE_RENDER

#include <config.h>
#include <vector>
#include <string>

namespace rs {
  //Forward declaration of ResponseBase (see rsresponse.h)
  class ResponseBase;
  //Forward declaration of Receiver (see rsradar.h)
  class Receiver;
  /// Export the responses received by a receiver to an XML file
  void ExportReceiverXML(const std::vector<rs::ResponseBase*> &responses, const std::string filename);
  /// Export the receiver pulses to the specified binary file, using the specified quantization
  void ExportReceiverBinary(const std::vector<rs::ResponseBase *> &responses, rs::Receiver* recv, const std::string recv_name, const std::string filename);
  /// Export the receiver responses to the specified CSV value files
  void ExportReceiverCSV(const std::vector<rs::ResponseBase*> &responses, const std::string filename);
}


#endif
