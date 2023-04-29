//================================================================================================
/// @file can_control_function.hpp
///
/// @brief Defines a base class to represent a generic ISOBUS control function.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_CONTROL_FUNCTION_HPP
#define CAN_CONTROL_FUNCTION_HPP

#include "isobus/isobus/can_NAME.hpp"

#include <mutex>

namespace isobus
{
	//================================================================================================
	/// @class ControlFunction
	///
	/// @brief A class that describes an ISO11783 control function, which includes a NAME and address.
	//================================================================================================
	class ControlFunction
	{
	public:
		/// @brief The type of the control function
		enum class Type
		{
			Internal, ///< The control function is part of our stack and can address claim
			External, ///< The control function is some other device on the bus
			Partnered ///< An external control function that you explicitly want to talk to
		};

		/// @brief The base class constructor for a control function
		/// @param[in] NAMEValue The NAME of the control function
		/// @param[in] addressValue The current address of the control function
		/// @param[in] CANPort The CAN channel index that the control function communicates on
		ControlFunction(NAME NAMEValue, std::uint8_t addressValue, std::uint8_t CANPort);

		/// @brief The base class destructor for a control function
		virtual ~ControlFunction();

		/// @brief Returns the current address of the control function
		/// @returns The current address of the control function
		std::uint8_t get_address() const;

		/// @brief Describes if the control function has a valid address (not NULL or global)
		/// @returns true if the address is < 0xFE
		bool get_address_valid() const;

		/// @brief Returns the CAN channel index the control function communicates on
		/// @returns The control function's CAN channel index
		std::uint8_t get_can_port() const;

		/// @brief Returns the NAME of the control function as described by its address claim message
		/// @returns The control function's NAME
		NAME get_NAME() const;

		/// @brief Returns the `Type` of the control function
		/// @returns The control function type
		Type get_type() const;

	protected:
		friend class CANNetworkManager;
		static std::mutex controlFunctionProcessingMutex; ///< Protects the control function tables
		NAME controlFunctionNAME; ///< The NAME of the control function
		Type controlFunctionType = Type::External; ///< The Type of the control function
		std::uint8_t address; ///< The address of the control function
		std::uint8_t canPortIndex; ///< The CAN channel index of the control function
		bool claimedAddressSinceLastAddressClaimRequest = false; ///< Used to mark CFs as stale if they don't claim within a certain time
	};

} // namespace isobus

#endif // CAN_CONTROL_FUNCTION_HPP
