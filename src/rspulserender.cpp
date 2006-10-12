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

#include "rspulserender.h"
#include "rsresponse.h"
#include "rsdebug.h"
#include "rsparameters.h"
#include "fersbin.h"

#define TIXML_USE_STL
#include "tinyxml/tinyxml.h"

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
  void WriteFersBinResponseHeader(FILE *fp, rs::ResponseBase *resp, int size, rsFloat rate)
  {
    FersBin::PulseResponseHeader prh;
    prh.magic = 0xFE00;
    prh.count = size;
    prh.rate = static_cast<double>(rate);
    prh.start = static_cast<double>(resp->GetStart());
    if (fwrite(&prh, sizeof(prh), 1, fp) != 1)
      throw std::runtime_error("[ERROR] Could not write response header to fersbin file");
  }

  /// Write the FersCSV file header
  void WriteFersCSVFileHeader(FILE* out, const std::string& recv, const std::string &trans)
  {
    fprintf(out, "# FERS CSV Simulation Results\n");
    fprintf(out, "# Recieved at %s\n", recv.c_str());
    fprintf(out, "# Transmitted by %s\n", trans.c_str());  
  }

  /// Write the FersBin response header
  void WriteFersCSVResponseHeader(FILE* out, rs::ResponseBase *resp, int size, rsFloat rate)
  {
    fprintf(out, "\n\n# Start of return pulse \n");
    fprintf(out, "%f, %f\n", resp->GetStart(), rate);
  }

  /// Export to the FersBin file format
  void ExportResponseFersBin(const std::vector<rs::ResponseBase*>& responses, const std::string& recv_name, const std::string& filename)
  {
    //Bail if there are no responses to export
    if (responses.empty())
      return;

    FILE *out_bin;
    if (rs::rsParameters::export_binary()) {
      //Build the filename for the binary file
      std::ostringstream b_oss;
      b_oss.setf(std::ios::scientific);    
      b_oss << recv_name << "_" << (*responses.begin())->GetTransmitterName() << ".fersbin";
      //Open the binary file for writing
      out_bin = fopen(b_oss.str().c_str(), "wb");
      if (!out_bin)
	throw std::runtime_error("[ERROR] Could not open file "+b_oss.str()+" for writing");
      // Write the file header into the file
      WriteFersBinFileHeader(out_bin);
    }
    FILE* out_csv;
    if (rs::rsParameters::export_csvbinary()) {
      //Build the filename for the CSV file
      std::ostringstream oss;
      oss.setf(std::ios::scientific);
      oss << recv_name << "_" << (*responses.begin())->GetTransmitterName() << ".ferscsv";
      //Open the file for writing
      out_csv = fopen(oss.str().c_str(), "w");
      if (!out_csv)
	throw std::runtime_error("[ERROR] Could not open file "+oss.str()+" for writing");
      // Write the file header into the file
      WriteFersCSVFileHeader(out_csv, recv_name, (*responses.begin())->GetTransmitterName());
    }

    // Now loop through the responses and write them to the file
    std::vector<rs::ResponseBase *>::const_iterator iter;
    for (iter = responses.begin(); iter != responses.end(); iter++) {
      rs::ResponseBase* resp = *iter;
      //Render the pulse into memory
      unsigned int size;
      rsFloat rate;
      boost::shared_array<rs::rsComplex> array = resp->RenderBinary(rate, size);
      //Export the binary format
      if (rs::rsParameters::export_binary()) {
	//Write the output header into the file
	WriteFersBinResponseHeader(out_bin, resp, size, rate);
	//Now write the binary data for the pulse
	if (fwrite(array.get(), sizeof(rs::rsComplex), size, out_bin) != size)
	  throw std::runtime_error("[ERROR] Could not complete write to binary file");
      }
      //Export the CSV format
      if (rs::rsParameters::export_csvbinary()) {
	//Write the output header into the file
	WriteFersCSVResponseHeader(out_csv, resp, size, rate);
	// Now write the binary data for the pulse
	for (unsigned int i = 0; i < size; i++)
	  fprintf(out_csv, "%f, %f\n", array[i].real(), array[i].imag());
      }
    }
    fclose(out_bin);
    fclose(out_csv);
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
void rs::ExportReceiverBinary(const std::vector<rs::ResponseBase *> &responses, const std::string recv_name, const std::string filename)
{
  ExportResponseFersBin(responses, recv_name, filename);
}

