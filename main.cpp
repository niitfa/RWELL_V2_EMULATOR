#include <iostream>
#include <stdint.h>
#include <functional>
#include <thread>
#include "rwell_emulator.h"

int main(int argc, char **argv)
{
	RWELLEmulator* rwell = new RWELLEmulator("127.0.0.1", 22250);
	while (true) rwell->loop();
	return 0;
}
