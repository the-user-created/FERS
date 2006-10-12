//fersbin.h - Structures defining the fersbin file format
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//12 September 2006

#ifndef __FERSBIN_H
#define __FERSBIN_H

#include <boost/cstdint.hpp> //Use the sized integers from boost

namespace FersBin {


  /// File header - place one at exactly the beginning of the file
  struct FileHeader {
    //FileHeader is exactly 8 bytes long
    boost::uint32_t magic;
    boost::uint16_t version; //The version of the file format
    boost::uint16_t float_size; //Size (in bytes) of the floats used by the other structures    
  } __attribute__((packed));
  /// Response header - place one at the beginning of each response
  struct PulseResponseHeader {
    boost::uint32_t magic; //Magic number to mark block start
    boost::uint32_t count; //Number of complex samples
    double rate; //Sample rate of the data in this chunk
    double start; //Start time of this block
  } __attribute__((packed));
}

#endif
