#pragma once

#include "can_control_function.hpp"
#include "can_address_claim_state_machine.hpp"
#include "can_NAME_filter.hpp"
#include "can_lib_callbacks.hpp"
#include "can_lib_badge.hpp"

#include <vector>

namespace isobus
{
class CANNetworkManager;
class PartneredControlFunction : public ControlFunction
{
public:
    PartneredControlFunction(std::uint8_t CANPort, const std::vector<NAMEFilter> NAMEFilters);

    void add_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback);
    void remove_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback);

    std::uint32_t get_number_parameter_group_number_callbacks() const;

    static PartneredControlFunction *get_partnered_control_function(std::uint32_t index);
    static std::uint32_t get_number_partnered_control_functions();

private:
    friend class CANNetworkManager;

    ParameterGroupNumberCallbackData get_parameter_group_number_callback(std::uint32_t index) const;

    static std::vector<PartneredControlFunction> partneredControlFunctionList;
    const std::vector<NAMEFilter> NAMEFilterList;
    std::vector<ParameterGroupNumberCallbackData> parameterGroupNumberCallbacks;
};

} // namespace isobus
