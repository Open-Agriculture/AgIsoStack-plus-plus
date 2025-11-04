#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

#include "../../common/console_logger.cpp"

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " <iop file path>\n";
		return 1;
	}

	const char *filename = argv[1];
	std::ifstream file(filename, std::ios::binary);
	if (!file)
	{
		std::cerr << "Unable to open: " << filename << "\n";
		return 1;
	}

	std::vector<std::uint8_t> buffer((std::istreambuf_iterator<char>(file)),
	                                 std::istreambuf_iterator<char>());

	if (buffer.empty())
	{
		std::cerr << "File is empty or not readable.\n";
		return 1;
	}

	isobus::CANStackLogger::set_can_stack_logger_sink(&logger);

	isobus::VirtualTerminalServerManagedWorkingSet vt;
	bool result = vt.parse_iop_into_objects(buffer.data(),
	                                        static_cast<std::uint32_t>(buffer.size()));

	std::cout << "IOP parse result: " << std::boolalpha << result << "\n";
	return 0;
}
