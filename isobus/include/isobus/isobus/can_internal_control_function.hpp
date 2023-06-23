//================================================================================================
/// @file can_internal_control_function.hpp
///
/// @brief A representation of an ISOBUS ECU that we can send from. Use this class
/// when defining your own control functions that will claim an address within your program.
/// @author Adrian Del Grosso
/// @author Daan Steenbergen
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_INTERNAL_CONTROL_FUNCTION_HPP
#define CAN_INTERNAL_CONTROL_FUNCTION_HPP

#include "isobus/isobus/can_address_claim_state_machine.hpp"
#include "isobus/isobus/can_badge.hpp"
#include "isobus/isobus/can_control_function.hpp"

#include <vector>

namespace isobus
{
	class CANNetworkManager;

	//================================================================================================
	/// @class InternalControlFunction
	///
	/// @brief Describes an internal ECU's NAME and address data. Used to send CAN messages.
	/// @details This class is used to define your own ECU's NAME, and is used to transmit messages.
	/// Each instance of this class will claim a unique address on the bus, and can be used to
	/// send messages.
	//================================================================================================
	class InternalControlFunction : public ControlFunction
	{
	public:
		/// @brief The factory function to construct an internal control function
		/// @param[in] desiredName The NAME for this control function to claim as
		/// @param[in] preferredAddress The preferred NAME for this control function
		/// @param[in] CANPort The CAN channel index for this control function to use
		static std::shared_ptr<InternalControlFunction> create(NAME desiredName, std::uint8_t preferredAddress, std::uint8_t CANPort);

		/// @brief Used by the network manager to tell the ICF that the address claim state machine needs to process
		/// a J1939 command to move address.
		void process_commanded_address(std::uint8_t commandedAddress, CANLibBadge<CANNetworkManager>);

		/// @brief Updates the internal control function together with it's associated address claim state machine
		/// @returns Wether the control function has changed address by the end of the update
		bool update_address_claiming(CANLibBadge<CANNetworkManager>);

	protected:
		/// @brief The protected constructor for the internal control function, which is called by the (inherited) factory function
		/// @param[in] desiredName The NAME for this control function to claim as
		/// @param[in] preferredAddress The preferred NAME for this control function
		/// @param[in] CANPort The CAN channel index for this control function to use
		InternalControlFunction(NAME desiredName, std::uint8_t preferredAddress, std::uint8_t CANPort);

	private:
		AddressClaimStateMachine stateMachine; ///< The address claimer for this ICF
	};

} // namespace isobus

#endif // CAN_INTERNAL_CONTROL_FUNCTION_HPP
