// xmlimport.h
// Import a simulator world and simulation parameters from an XML file
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 26 April 2006

#ifndef XML_IMPORT_H
#define XML_IMPORT_H

#include <string>

namespace core {
	class World;
}

namespace serial
{
	void loadXmlFile(const std::string& filename, core::World* world);
}

#endif
