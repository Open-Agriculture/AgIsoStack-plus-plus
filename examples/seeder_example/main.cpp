#include "seeder.hpp"

#include <csignal>

std::atomic_bool running = { true };

void signal_handler(int)
{
	running = false;
}

int main()
{
	std::signal(SIGINT, signal_handler);

	Seeder seederExample;

	if (seederExample.initialize())
	{
		while (running)
		{
			seederExample.update();
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
		seederExample.terminate();
	}
}
