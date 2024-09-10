// Test clock modeling classes

#include "config.h"
#include "rsnoise.h"
#include "rstiming.h"

using namespace rs;

unsigned int processors = 4;

//Create the prototype timing class
PrototypeTiming* makeProtoTiming()
{
	//Two parameter clock model
	PrototypeTiming* proto = new PrototypeTiming("test");
	proto->addAlpha(0, 0.05); //White PM
	proto->addAlpha(2, 0.95); //White FM
	proto->setFrequency(1e9);
	//proto->AddAlpha(3, 0); //Flicker FM
	return proto;
}

// Test the pulsetiming class
int testClockModelTiming(const PrototypeTiming* proto)
{
	constexpr int pulses = 3;
	ClockModelTiming* timing = new ClockModelTiming("test_pulse");
	timing->initializeModel(proto);

	for (int i = 0; i < pulses; i++)
	{
		const RS_FLOAT j = timing->getPulseTimeError();
		std::cout << j << std::endl;
	}
	for (int k = 0; k < pulses; k++)
	{
		constexpr int pulse_length = 1000;
		for (int i = 0; i < pulse_length; i++)
		{
			const RS_FLOAT j = timing->nextNoiseSample();
			std::cout << j << std::endl;
		}
	}
	delete timing;
	return 0;
}


int main()
{
	rs_noise::initializeNoise();
	const PrototypeTiming* timing = makeProtoTiming();

	//TestPulseTiming(timing);
	testClockModelTiming(timing);

	rs_noise::cleanUpNoise();
	delete timing;
}
