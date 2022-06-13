#pragma once

#include <cstdint>
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

protected:
    virtual void update() = 0;

    static std::vector<CANLibProtocol> protocolList;

    bool initialized;
};

}
