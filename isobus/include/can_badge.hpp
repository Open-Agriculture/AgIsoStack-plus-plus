//================================================================================================
/// @file cab_badge.hpp
///
/// @brief A way to only allow certain object types to access certain functions that is enforced
/// at compile time. A neat trick from Serenity OS :^) 
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_BADGE_HPP
#define CAN_BADGE_HPP

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

#endif // CAN_BADGE_HPP
