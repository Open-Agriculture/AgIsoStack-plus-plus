#pragma once

#include "can_control_function.hpp"
#include "can_message.hpp"
#include "can_lib_badge.hpp"

#include <vector>

namespace isobus
{
    class CANNetworkManager;
	class CANLibProtocol
	{
	public:
		CANLibProtocol();
		virtual ~CANLibProtocol();

		bool get_is_initialized() const;

		static bool get_protocol(std::uint32_t index, CANLibProtocol *&returnedProtocol);

        static std::uint32_t get_number_protocols();

		virtual void initialize(CANLibBadge<CANNetworkManager>);

		virtual void process_message(CANMessage *const message) = 0;

        virtual bool protocol_transmit_message(std::uint32_t parameterGroupNumber,
		                               std::uint8_t *data,
		                               std::uint32_t messageLength,
		                               ControlFunction *source,
		                               ControlFunction *destination) = 0;

		virtual void update(CANLibBadge<CANNetworkManager>) = 0;

	protected:
		static std::vector<CANLibProtocol*> protocolList;

		bool initialized;
	};

}
