//rspulserender.cpp
//Performs the second phase of the simulation - rendering the result
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//7 June 2006

#include <stdexcept>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <cstdio> //The C stdio functions are used for binary export
#include <boost/scoped_array.hpp>

#include "rsradar.h"
#include "rspulserender.h"
#include "rsresponse.h"
#include "rsdebug.h"
#include "rsparameters.h"
#include "rsnoise.h"
#include "fersbin.h"

#define TIXML_USE_STL
#include "tinyxml/tinyxml.h"

using namespace rs;

namespace {

  /// Write the FersBin file header
  void WriteFersBinFileHeader(FILE *fp)
  {
    FersBin::FileHeader fh;
    fh.magic = 0x73726566;
    fh.version = 1;
    fh.float_size = sizeof(double);
    if (fwrite(&fh, sizeof(fh), 1, fp) != 1)
      throw std::runtime_error("[ERROR] Could not write file header to fersbin file");
  }

  /// Write the FersBin response header
  void WriteFersBinResponseHeader(FILE *fp, rsFloat start, rsFloat rate, int size)
  {
    FersBin::PulseResponseHeader prh;
    prh.magic = 0xFE00;
    prh.count = size;
    prh.rate = static_cast<double>(rate);
    prh.start = static_cast<double>(start);
    if (fwrite(&prh, sizeof(prh), 1, fp) != 1)
      throw std::runtime_error("[ERROR] Could not write response header to fersbin file");
  }

  /// Write the FersCSV file header
  void WriteFersCSVFileHeader(FILE* out, const std::string& recv)
  {
    fprintf(out, "# FERS CSV Simulation Results\n");
    fprintf(out, "# Recieved at %s\n", recv.c_str());
  }

  /// Write the FersBin response header
  void WriteFersCSVResponseHeader(FILE* out, rsFloat start, rsFloat rate)
  {
    fprintf(out, "\n\n# Start of return pulse \n");
    fprintf(out, "%f, %f\n", start, rate);
  }

  ///Open the binary file for response export
  FILE *OpenBinaryFile(const std::string &recv_name)
  {
    FILE *out_bin = 0;
    if (rs::rsParameters::export_binary()) {
      //Build the filename for the binary file
      std::ostringstream b_oss;
      b_oss.setf(std::ios::scientific);    
      b_oss << recv_name << ".fersbin";
      //Open the binary file for writing
      out_bin = fopen(b_oss.str().c_str(), "wb");
      if (!out_bin)
	throw std::runtime_error("[ERROR] Could not open file "+b_oss.str()+" for writing");
      // Write the file header into the file
      WriteFersBinFileHeader(out_bin);
    }
    return out_bin;
  }

  /// Open the CSV file for response export
  FILE *OpenCSVFile(const std::string &recv_name)
  {
    FILE *out_csv = 0;
    if (rs::rsParameters::export_csvbinary()) {
      //Build the filename for the CSV file
      std::ostringstream oss;
      oss.setf(std::ios::scientific);
      oss << recv_name << ".ferscsv";
      //Open the file for writing
      out_csv = fopen(oss.str().c_str(), "w");
      if (!out_csv)
	throw std::runtime_error("[ERROR] Could not open file "+oss.str()+" for writing");
      // Write the file header into the file
      WriteFersCSVFileHeader(out_csv, recv_name);
    }
    return out_csv;
  }

  /// Add noise to the window, to simulate receiver noise
  void AddNoiseToWindow(rs::rsComplex *data, unsigned int size, rsFloat temperature)
  {
    if (temperature == 0)
      return;  
    //Calculate the noise power
    rsFloat power = rsNoise::NoiseTemperatureToPower(temperature, rsParameters::rate()/2);
    WGNGenerator generator(sqrt(power)/2.0);
    //Add the noise to the signal
    for (unsigned int i = 0; i < size; i++)
      data[i] += rsComplex(generator.GetSample(), generator.GetSample());
  }

  /// Add a response array to the receive window
  void AddArrayToWindow(rsFloat wstart, rs::rsComplex *window, unsigned int wsize, rsFloat rate, rsFloat rstart, rs::rsComplex *resp, unsigned int rsize)
  {
    int start_sample = static_cast<int>(std::floor(rate*(rstart-wstart)));
    rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Adding response at %f at sample %d\n", rstart, start_sample);
    //Get the offset into the response
    unsigned int roffset = 0;
    if (start_sample < 0) {
      roffset = -start_sample;
      start_sample = 0;
    }
    //Loop through the response, adding it to the window
    for (unsigned int i = roffset; (i < rsize) && ((i+start_sample) < wsize); i++)
      window[i+start_sample] += resp[i];    
  }

  /// Export to the FersBin file format
  void ExportResponseFersBin(const std::vector<rs::ResponseBase*>& responses, const rs::Receiver* recv, const std::string& recv_name)
  {
    //Bail if there are no responses to export
    if (responses.empty())
      return;

    FILE* out_bin = OpenBinaryFile(recv_name);   
    FILE* out_csv = OpenCSVFile(recv_name);

    // Now loop through the responses and write them to the file


    int window_count = recv->GetWindowCount();
    for (int i = 0; i < window_count; i++) {
      rsFloat length = recv->GetWindowLength();
      rsFloat start = recv->GetWindowStart(i);
      rsFloat end = start+length;
      rsFloat rate = rsParameters::rate();
      unsigned int size = static_cast<unsigned int>(std::ceil(length*rate));
      // Allocate memory for the entire window
      boost::scoped_array<rsComplex> window(new rsComplex[size]);
      //Clear the window in memory
      memset(window.get(), 0, sizeof(rsComplex)*size);
      //Add Noise to the window
      AddNoiseToWindow(window.get(), size, recv->GetNoiseTemperature());
      //Loop through the responses, adding those in this window
      std::vector<rs::ResponseBase *>::const_iterator iter;
      for (iter = responses.begin(); iter != responses.end(); iter++) {	
	rs::ResponseBase* resp = *iter;
	if ((resp->GetStart() < end) && ((resp->GetStart() + resp->GetLength()) >= start)) {	  
	  //Render the pulse into memory
	  unsigned int psize;
	  rsFloat prate;
	  boost::shared_array<rs::rsComplex> array = resp->RenderBinary(prate, psize);
	  //If the rates differ from the default rate, we can't render for now
	  if (prate != rate)
	    throw std::runtime_error("[ERROR] Cannot render results with differing rates");
	  //Now add the array to the window
	  AddArrayToWindow(start, window.get(), size, rate, resp->GetStart(), array.get(), psize);
	}
      }
     
      //Export the binary format
      if (rs::rsParameters::export_binary()) {
	//Write the output header into the file
	WriteFersBinResponseHeader(out_bin, start, rate, size);
	//Now write the binary data for the pulse
	if (fwrite(window.get(), sizeof(rs::rsComplex), size, out_bin) != size)
	  throw std::runtime_error("[ERROR] Could not complete write to binary file");
      }
      //Export the CSV format
      if (rs::rsParameters::export_csvbinary()) {
	//Write the output header into the file
	WriteFersCSVResponseHeader(out_csv, start, rate);
	// Now write the binary data for the pulse
	for (unsigned int i = 0; i < size; i++)
	  fprintf(out_csv, "%20.12e, %20.12e\n", window[i].real(), window[i].imag());
      }
    } // for (i = 1:windows)
    // Close the binary and csv files
    if (out_bin) fclose(out_bin);
    if (out_csv) fclose(out_csv);
  }

}

/// Export the responses received by a receiver to an XML file
void rs::ExportReceiverXML(const std::vector<rs::ResponseBase*> &responses, const std::string filename)
{
  //Create the document
  TiXmlDocument doc;
  TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
  doc.LinkEndChild(decl);
  //Create a root node for the document
  TiXmlElement *root = new TiXmlElement("receiver");
  doc.LinkEndChild(root);

  //dump each response in turn
  std::vector<rs::ResponseBase*>::const_iterator ri;
  for (ri = responses.begin(); ri != responses.end(); ri++)
    (*ri)->RenderXML(root);

  //write the output to the specified file
  doc.SaveFile(filename+".fersxml");  
}

/// Export the responses in CSV format
void rs::ExportReceiverCSV(const std::vector<rs::ResponseBase*> &responses, const std::string filename)
{
  std::map<std::string, std::ofstream *> streams; //map of per-transmitter open files
  std::vector<rs::ResponseBase*>::const_iterator iter;
  for (iter = responses.begin(); iter != responses.end(); iter++)
    {
      std::ofstream *of;
      // See if a file is already open for that transmitter
      std::map<std::string, std::ofstream *>::iterator ofi = streams.find((*iter)->GetTransmitterName());
      // If the file for that transmitter does not exist, add it
      if (ofi == streams.end())	{
        std::ostringstream oss;  
        oss << filename << "_" << (*iter)->GetTransmitterName() << ".csv";
	  //Open a new ofstream with that name
	  of = new std::ofstream(oss.str().c_str());
	  of->setf(std::ios::scientific); //Set the stream in scientific notation mode
	  //Check if the open succeeded
	  if (!(*of))
	    throw std::runtime_error("[ERROR] Could not open file "+oss.str()+" for writing");
	  //Add the file to the map
	  streams[(*iter)->GetTransmitterName()] = of;
	}
      else { //The file is already open
	of = (*ofi).second;
      }
      // Render the response to the file
      (*iter)->RenderCSV(*of);
    }
  //Close all the files that we opened
  std::map<std::string, std::ofstream *>::iterator ofi;
  for (ofi = streams.begin(); ofi != streams.end(); ofi++)
    delete (*ofi).second;
}

/// Export the receiver pulses to the specified binary file, using the specified quantization
void rs::ExportReceiverBinary(const std::vector<rs::ResponseBase *> &responses, Receiver* recv, const std::string recv_name, const std::string filename)
{
  ExportResponseFersBin(responses, recv, recv_name);
}

