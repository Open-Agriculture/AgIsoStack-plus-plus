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

#include <list>

namespace isobus
{
    
class InternalControlFunction : public ControlFunction
{
public:
    InternalControlFunction(NAME desiredName, std::uint8_t preferredAddress, std::uint8_t CANPort);
    ~InternalControlFunction();

    static InternalControlFunction *get_internal_control_function(std::uint32_t index);
    static std::uint32_t get_number_internal_control_functions();

    static void update_address_claiming();
    void update();

private:
    static std::list<InternalControlFunction*> internalControlFunctionList;
    AddressClaimStateMachine stateMachine;
};

} // namespace isobus

#endif // CAN_INTERNAL_CONTROL_FUNCTION_HPP
