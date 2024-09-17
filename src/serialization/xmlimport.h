// xmlimport.h
// Import a simulator world and simulation parameters from an XML file
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 26 April 2006

#ifndef XML_IMPORT_H
#define XML_IMPORT_H

#include <string>

#include "core/world.h"

namespace xml
{
	void loadXmlFile(const std::string& filename, rs::World* world);
}

#endif
