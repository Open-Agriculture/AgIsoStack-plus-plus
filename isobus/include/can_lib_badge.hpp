#pragma once

namespace isobus
{
    
template<typename T>
class CANLibBadge
{
private:
    friend T;
    CANLibBadge() {};
};

} // namespace isobus
