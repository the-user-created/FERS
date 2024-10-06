//
// Created by David Young on 10/3/24.
//

#pragma once

#include <string>

namespace core {
	class World;
}

namespace serial
{
	void parseSimulation(const std::string& filename, core::World* world);
}
