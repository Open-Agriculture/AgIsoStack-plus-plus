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
		/// @brief Constructor for an internal control function
		/// @param[in] desiredName The NAME for this control function to claim as
		/// @param[in] preferredAddress The preferred NAME for this control function
		/// @param[in] CANPort The CAN channel index for this control function to use
		InternalControlFunction(NAME desiredName, std::uint8_t preferredAddress, std::uint8_t CANPort);

		/// @brief Destructor for an internal control function
		~InternalControlFunction();

		/// @brief Returns a an internal control function from the list of all internal control functions
		/// @param[in] index The index in the list internalControlFunctionList from which to get an ICF
		/// @returns The requested internal control function or `nullptr` if the index is out of range
		static InternalControlFunction *get_internal_control_function(std::uint32_t index);

		/// @brief Returns the number of internal control functions that exist
		static std::uint32_t get_number_internal_control_functions();

		/// @brief Lets network manager know a control function changed address recently
		/// @details These tell the network manager when the address table needs to be explicitly
		/// updated for an internal control function claiming a new address.
		/// Other CF types are handled in Rx message processing.
		static bool get_any_internal_control_function_changed_address(CANLibBadge<CANNetworkManager>);

		/// @brief Used to determine if the internal control function changed address since the last network manager update
		/// @returns true if the ICF changed address since the last network manager update
		bool get_changed_address_since_last_update(CANLibBadge<CANNetworkManager>) const;

		/// @brief Used by the network manager to tell the ICF that the address claim state machine needs to process
		/// a J1939 command to move address.
		void process_commanded_address(std::uint8_t commandedAddress, CANLibBadge<CANNetworkManager>);

		/// @brief Updates all address claim state machines
		static void update_address_claiming(CANLibBadge<CANNetworkManager>);

	private:
		/// @brief Updates the internal control function, should be called periodically by the network manager
		void update();

		static std::vector<InternalControlFunction *> internalControlFunctionList; ///< A list of all internal control functions that exist
		static bool anyChangedAddress; ///< Lets the network manager know if any ICF changed address since the last update
		AddressClaimStateMachine stateMachine; ///< The address claimer for this ICF
		bool objectChangedAddressSinceLastUpdate = false; ///< Tracks if this object has changed address since the last update
	};

} // namespace isobus

#endif // CAN_INTERNAL_CONTROL_FUNCTION_HPP
