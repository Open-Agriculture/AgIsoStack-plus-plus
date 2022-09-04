//================================================================================================
/// @file can_internal_control_function.hpp
///
/// @brief A representation of an ISOBUS ECU that we can send from. Use this class
/// when defining your own control functions that will claim an address within your program.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_INTERNAL_CONTROL_FUNCTION_HPP
#define CAN_INTERNAL_CONTROL_FUNCTION_HPP

#include "can_control_function.hpp"
#include "can_address_claim_state_machine.hpp"
#include "can_badge.hpp"

#include <list>

namespace isobus
{
class CANNetworkManager; 
class InternalControlFunction : public ControlFunction
{
public:
    InternalControlFunction(NAME desiredName, std::uint8_t preferredAddress, std::uint8_t CANPort);
    ~InternalControlFunction();

    static InternalControlFunction *get_internal_control_function(std::uint32_t index);
    static std::uint32_t get_number_internal_control_functions();

    // These tell the network manager when the address table needs to be explicitly
    // updated for an internal control function claiming a new address.
    // Other CF types are handled in Rx message processing.
    static bool get_any_internal_control_function_changed_address(CANLibBadge<CANNetworkManager>);
    bool get_changed_address_since_last_update(CANLibBadge<CANNetworkManager>) const;

    static void update_address_claiming(CANLibBadge<CANNetworkManager>);

private:
    void update();

    static std::list<InternalControlFunction*> internalControlFunctionList;
    static bool anyChangedAddress;
    AddressClaimStateMachine stateMachine;
    bool objectChangedAddressSinceLastUpdate;
};

} // namespace isobus

#endif // CAN_INTERNAL_CONTROL_FUNCTION_HPP
