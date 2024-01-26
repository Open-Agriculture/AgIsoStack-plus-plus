#ifndef CONTROL_FUNCTION_HELPERS_HPP
#define CONTROL_FUNCTION_HELPERS_HPP

#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"

namespace test_helpers
{
	std::shared_ptr<isobus::InternalControlFunction> claim_internal_control_function(std::uint8_t address, std::uint8_t canPort);

	std::shared_ptr<isobus::PartneredControlFunction> force_claim_partnered_control_function(std::uint8_t address, std::uint8_t canPort);

	std::shared_ptr<isobus::ControlFunction> create_mock_control_function(std::uint8_t address);

	std::shared_ptr<isobus::InternalControlFunction> create_mock_internal_control_function(std::uint8_t address);

} // namespace test_helpers

#endif // CONTROL_FUNCTION_HELPERS_HPP
