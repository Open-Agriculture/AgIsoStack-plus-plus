//================================================================================================
/// @file can_control_function_reference.hpp
///
/// @brief Holds a weak reference to a control function, but in a way that it is more robust.
/// @author Daan Steenbergen
///
/// @copyright 2023 Open-Agriculture Contributors
//================================================================================================

#ifndef CAN_CONTROL_FUNCTION_REFERENCE_HPP
#define CAN_CONTROL_FUNCTION_REFERENCE_HPP

#include "isobus/isobus/can_control_function.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"

namespace isobus
{
	//================================================================================================
	/// @class ControlFunctionReference
	///
	/// @brief A class that holds a weak reference to a control function, but in a way that it is more robust.
	//================================================================================================
	class ControlFunctionReference : public std::weak_ptr<ControlFunction>
	{
	public:
		static const isobus::ControlFunctionReference ANY_CONTROL_FUNCTION; ///< A reference to all control functions on the bus

		/// @brief Constructs a control function reference from a ControlFunction
		/// @param[in] controlFunction The control function to reference, or nullptr if all control functions are referenced
		ControlFunctionReference(std::shared_ptr<ControlFunction> controlFunction);

		/// @brief Constructs a control function reference from a PartneredControlFunction
		/// @param[in] controlFunction The control function to reference, or nullptr if all control functions are referenced
		ControlFunctionReference(std::shared_ptr<PartneredControlFunction> controlFunction);

		/// @brief Check if the control function is not actively managed by the stack anymore
		/// @return true if the control function is unmaintained, otherwise false
		bool is_stale() const;

		/// @brief Check if the control function has a valid address
		/// @returns true if the control function still exists and has a valid address, otherwise false
		bool has_valid_address() const;

		/// @brief Get the address of the control function
		/// @param[out] address The address of the control function
		/// @returns true if the control function still exists and has a valid address, otherwise false
		bool get_address(std::uint8_t &address) const;

		/// @brief Check if the reference is to all control functions on the bus
		/// @returns true if the reference is to all control functions on the bus, otherwise false
		bool is_broadcast() const;

	private:
		bool is_global; ///< True if the reference is to all control functions on the bus
	};

	//================================================================================================
	/// @class PartneredControlFunctionReference
	///
	/// @brief A class that holds a weak reference to a control function, but in a way that it is more robust.
	//================================================================================================
	class PartneredControlFunctionReference : public std::weak_ptr<PartneredControlFunction>
	  , public ControlFunctionReference
	{
	public:
		static const isobus::PartneredControlFunctionReference ANY_CONTROL_FUNCTION; ///< A reference to all control functions on the bus

		/// @brief Constructs a control function reference from a shared pointer
		/// @param[in] controlFunction The control function to reference, or nullptr if all control functions are referenced
		explicit PartneredControlFunctionReference(std::shared_ptr<PartneredControlFunction> controlFunction);
	};
}
#endif // CAN_CONTROL_FUNCTION_REFERENCE_HPP
