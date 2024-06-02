//================================================================================================
/// @file processing_flags.hpp
///
/// @brief A class that manages 1 bit flags. Handy as a retry machanism for sending CAN messages.
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================
#ifndef PROCESSING_FLAGS_HPP
#define PROCESSING_FLAGS_HPP

#include <cstdint>

namespace isobus
{
	class ProcessingFlags
	{
	public:
		typedef void (*ProcessFlagsCallback)(std::uint32_t flag, void *parentPointer);

		ProcessingFlags(std::uint32_t numberOfFlags, ProcessFlagsCallback processingCallback, void *parentPointer);
		~ProcessingFlags();

		void set_flag(std::uint32_t flag);
		void process_all_flags();

	private:
		ProcessFlagsCallback callback;
		const std::uint32_t maxFlag;
		std::uint8_t *flagBitfield;
		void *parent;
	};
} // namespace isobus

#endif // PROCESSING_FLAGS_HPP
