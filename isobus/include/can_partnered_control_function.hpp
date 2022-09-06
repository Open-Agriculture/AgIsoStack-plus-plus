//================================================================================================
/// @file can_partnered_control_function.hpp
///
/// @brief A class that describes a control function on the bus that the stack should communicate
/// with. Use these to describe ECUs you want to send messages to.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_PARTNERED_CONTROL_FUNCTION_HPP
#define CAN_PARTNERED_CONTROL_FUNCTION_HPP

#include "can_control_function.hpp"
#include "can_address_claim_state_machine.hpp"
#include "can_NAME_filter.hpp"
#include "can_callbacks.hpp"
#include "can_badge.hpp"

#include <vector>

namespace isobus
{
class CANNetworkManager;
class PartneredControlFunction : public ControlFunction
{
public:
    PartneredControlFunction(std::uint8_t CANPort, const std::vector<NAMEFilter> NAMEFilters);
    ~PartneredControlFunction();

    void add_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parent);
    void remove_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parent);

    std::uint32_t get_number_parameter_group_number_callbacks() const;

    std::uint32_t get_number_name_filters() const;
    std::uint32_t get_number_name_filters_with_parameter_type(NAME::NAMEParameters parameter);
    bool get_name_filter_parameter(std::uint32_t index, NAME::NAMEParameters &parameter, std::uint32_t &filterValue) const;

    bool check_matches_name(NAME NAMEToCheck) const;

    static PartneredControlFunction *get_partnered_control_function(std::uint32_t index);
    static std::uint32_t get_number_partnered_control_functions();

private:
    friend class CANNetworkManager;

    ParameterGroupNumberCallbackData get_parameter_group_number_callback(std::uint32_t index) const;

    static std::vector<PartneredControlFunction*> partneredControlFunctionList;
    const std::vector<NAMEFilter> NAMEFilterList;
    std::vector<ParameterGroupNumberCallbackData> parameterGroupNumberCallbacks;
};

} // namespace isobus

#endif // CAN_PARTNERED_CONTROL_FUNCTION
