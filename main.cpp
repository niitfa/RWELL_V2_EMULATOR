#include <iostream>
#include <stdint.h>
#include <functional>
#include <thread>
#include "rwell_emulator.h"

int main(int argc, char **argv)
{
	RWELLEmulator* rwell = new RWELLEmulator("0.0.0.0", 22250);
	while (true) rwell->loop();
	return 0;
}
