//xmlimport.h
//Import a simulator world and simulation parameters from an XML file
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started: 26 April 2006

#ifndef XML_IMPORT_H
#define XML_IMPORT_H

#include <config.h>
#include <string>

#include "rsworld.h"

namespace xml
{
	//Load an XML file containing a simulation description
	//putting the structure of the world into the world object in the process
	void LoadXMLFile(const std::string& filename, rs::World* world);

	class XMLException
	{
	};
}

#endif
