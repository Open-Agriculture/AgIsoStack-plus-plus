//================================================================================================
/// @file can_arguments_parser.hpp
///
/// @brief A class that extracts the --interface and --driver parameters
/// @author Miklos Marton
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#pragma once

#include <string>

struct CanParameters
{
	std::string interface;
	std::string driver;
};

class CanParametersParser
{
public:
	CanParametersParser(int argc, char *argv[])
	{
		parse(argc, argv);
	}

	const CanParameters &parameters() const
	{
		return args;
	}

private:
	void parse(int argc, char *argv[])
	{
		for (int i = 1; i < argc; ++i)
		{
			const std::string argument = argv[i];

			if ((argument == "--interface") && ((i + 1) < argc))
			{
				args.interface = argv[++i];
			}
			else if ((argument == "--driver") && ((i + 1) < argc))
			{
				args.driver = argv[++i];
			}
		}
	}

	CanParameters args;
};
