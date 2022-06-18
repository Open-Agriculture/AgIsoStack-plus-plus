#pragma once

#include "can_control_function.hpp"
#include "can_address_claim_state_machine.hpp"
#include "can_NAME_filter.hpp"
#include "can_lib_callback.hpp"

#include <list>
#include <vector>

namespace isobus
{
    
class PartneredControlFunction : public ControlFunction
{
public:
    PartneredControlFunction(std::uint8_t CANPort, const std::vector<NAMEFilter> NAMEFilters);

    static PartneredControlFunction *get_partnered_control_function(std::uint32_t index);
    static std::uint32_t get_number_partnered_control_functions();

private:
    static std::list<PartneredControlFunction> partneredControlFunctionList;
    const std::vector<NAMEFilter> NAMEFilterList;
    std::list<CANLibCallback> parameterGroupNumberCallbacks;
};

} // namespace isobus
