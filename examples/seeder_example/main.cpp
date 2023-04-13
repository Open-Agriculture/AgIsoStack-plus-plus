//================================================================================================
/// @file main.cpp
///
/// @brief Defines `main` for the seeder example
/// @details This example is meant to use all the major protocols in a more "complete" application.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#include "seeder.hpp"

#include <csignal>

std::atomic_bool running = { true };

void signal_handler(int)
{
	running = false;
}

int main()
{
	int retVal = 0;
	Seeder seederExample;
	std::signal(SIGINT, signal_handler);

	if (seederExample.initialize())
	{
		while (running)
		{
			seederExample.update();
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
		seederExample.terminate();
	}
	else
	{
		retVal = -1; // Something wasn't right, such as CAN interface was missing.
	}
	return retVal;
}
