//
// Created by David Young on 10/3/24.
//

#ifndef XML_PARSER_H
#define XML_PARSER_H

#include <string>

namespace core {
	class World;
}

namespace serial
{
	void parseSimulation(const std::string& filename, core::World* world);
}

#endif //XML_PARSER_H
