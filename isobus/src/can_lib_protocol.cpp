#include "can_lib_protocol.hpp"

namespace isobus
{
	std::vector<CANLibProtocol> CANLibProtocol::protocolList;

	CANLibProtocol::CANLibProtocol() :
	  initialized(false)
	{
	}

	CANLibProtocol::~CANLibProtocol()
	{
	}

	bool CANLibProtocol::get_is_initialized() const
	{
		return initialized;
	}

	bool CANLibProtocol::get_protocol(std::uint32_t index, CANLibProtocol *returnedProtocol)
	{
		returnedProtocol = nullptr;

		if (index < protocolList.size())
		{
			returnedProtocol = &protocolList[index];
		}
		return (nullptr != returnedProtocol);
	}

	void CANLibProtocol::initialize()
	{
		initialized = true;
	}

} // namespace isobus
