//================================================================================================
/// @file processing_flags.cpp
///
/// @brief A class that manages 1 bit flags. Handy as a retry machanism for sending CAN messages.
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================

#include "isobus/utility/processing_flags.hpp"

#include <cstring>

namespace isobus
{
	ProcessingFlags::ProcessingFlags(std::uint32_t numberOfFlags, ProcessFlagsCallback processingCallback, void *parentPointer) :
	  callback(processingCallback),
	  maxFlag(numberOfFlags),
	  flagBitfield(nullptr),
	  parent(parentPointer)
	{
		const std::uint32_t numberBytes = (numberOfFlags / 8) + 1;
		flagBitfield = new std::uint8_t[numberBytes];
		memset(flagBitfield, 0, numberBytes);
	}

	ProcessingFlags ::~ProcessingFlags()
	{
		delete[] flagBitfield;
		flagBitfield = nullptr;
	}

	void ProcessingFlags::set_flag(std::uint32_t flag)
	{
		if (flag <= maxFlag)
		{
			flagBitfield[(flag / 8)] |= (1 << (flag % 8));
		}
	}

	void ProcessingFlags::process_all_flags()
	{
		const std::uint32_t numberBytes = (maxFlag / 8) + 1;

		for (std::uint32_t i = 0; i < numberBytes; i++)
		{
			if (flagBitfield[i])
			{
				for (std::uint8_t j = 0; j < 8; j++)
				{
					std::uint8_t currentFlag = (flagBitfield[i] & (1 << j));

					if (currentFlag)
					{
						flagBitfield[i] &= ~(currentFlag);
						callback((8 * i) + j, parent);
					}
				}
			}
		}
	}
} // namespace isobus
