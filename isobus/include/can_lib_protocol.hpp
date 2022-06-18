#pragma once

#include "can_message.hpp"

#include <vector>

namespace isobus
{

class CANLibProtocol
{
public:
    CANLibProtocol();
    virtual ~CANLibProtocol();

    bool get_is_initialized() const;

    bool get_protocol(std::uint32_t index, CANLibProtocol *returnedProtocol);

    virtual void initialize();

    virtual void process_message(CANMessage *const message) = 0;

    virtual void update() = 0;

protected:
    static std::vector<CANLibProtocol> protocolList;

    bool initialized;
};

}
