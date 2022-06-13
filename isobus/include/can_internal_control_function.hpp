#pragma once

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
