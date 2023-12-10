#ifndef CONTROL_FUNCTION_HELPERS_HPP
#define CONTROL_FUNCTION_HELPERS_HPP

#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"

namespace test_helpers
{
	std::shared_ptr<isobus::InternalControlFunction> claim_internal_control_function(std::uint8_t address, std::uint8_t canPort);

	std::shared_ptr<isobus::PartneredControlFunction> force_claim_partnered_control_function(std::uint8_t address, std::uint8_t canPort);

	std::uint32_t create_extended_can_id(std::uint8_t priority,
	                                     std::uint32_t parameterGroupNumber,
	                                     std::shared_ptr<isobus::ControlFunction> destination,
	                                     std::shared_ptr<isobus::ControlFunction> source);

}; // namespace test_helpers

#endif // CONTROL_FUNCTION_HELPERS_HPP
