//================================================================================================
/// @file can_internal_control_function.hpp
///
/// @brief A representation of an ISOBUS ECU that we can send from. Use this class
/// when defining your own control functions that will claim an address within your program.
/// @author Adrian Del Grosso
/// @author Daan Steenbergen
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================

#ifndef CAN_INTERNAL_CONTROL_FUNCTION_HPP
#define CAN_INTERNAL_CONTROL_FUNCTION_HPP

#include "isobus/isobus/can_control_function.hpp"
#include "isobus/isobus/can_message.hpp"
#include "isobus/utility/event_dispatcher.hpp"

#include <vector>

namespace isobus
{
	class ParameterGroupNumberRequestProtocol;

	/// @brief Describes an internal ECU's NAME and address data. Used to send CAN messages.
	/// @details This class is used to define your own ECU's NAME, and is used to transmit messages.
	/// Each instance of this class will claim a unique address on the bus, and can be used to
	/// send messages.
	class InternalControlFunction : public ControlFunction
	{
	public:
		/// @brief Defines the states the internal control function can be in
		enum class State
		{
			None, ///< Initial state
			WaitForClaim, ///< Waiting for the random delay time to expire
			SendRequestForClaim, ///< Sending the request for address claim to the bus
			WaitForRequestContentionPeriod, ///< Waiting for the address claim contention period to expire
			SendPreferredAddressClaim, ///< Claiming the preferred address as our own
			ContendForPreferredAddress, ///< Contending the preferred address with another ECU
			SendArbitraryAddressClaim, ///< Claiming an arbitrary (not our preferred) address as our own
			SendReclaimAddressOnRequest, ///< An ECU requested address claim, inform the bus of our current address
			UnableToClaim, ///< Unable to claim an address
			AddressClaimingComplete ///< Address claiming is complete and we have an address
		};

		/// @brief The constructor of an internal control function.
		/// In most cases use `CANNetworkManager::create_internal_control_function()` instead,
		/// only use this constructor if you have advanced needs.
		/// @param[in] desiredName The NAME for this control function to claim as
		/// @param[in] preferredAddress The preferred NAME for this control function
		/// @param[in] CANPort The CAN channel index for this control function to use
		InternalControlFunction(NAME desiredName, std::uint8_t preferredAddress, std::uint8_t CANPort);

		/// @brief Returns the current state of the internal control function
		/// @returns The current state
		State get_current_state() const;

		/// @brief Processes a CAN message for address claiming purposes
		/// @param[in] message The CAN message being received
		void process_rx_message_for_address_claiming(const CANMessage &message);

		/// @brief Updates the internal control function address claiming, will be called periodically by
		/// the network manager if the ICF is registered there.
		/// @returns true if the address of internal control function has changed, otherwise false
		bool update_address_claiming();

		/// @brief Returns the preferred address of the internal control function
		/// @returns The preferred address
		std::uint8_t get_preferred_address() const;

		/// @brief Returns the event dispatcher for when an address is claimed. Use this to register a callback
		/// for when an address is claimed.
		/// @returns The event dispatcher for when an address is claimed
		EventDispatcher<std::uint8_t> &get_address_claimed_event_dispatcher();

		/// @brief Gets the PGN request protocol for this ICF
		/// @returns The PGN request protocol for this ICF
		std::weak_ptr<ParameterGroupNumberRequestProtocol> get_pgn_request_protocol() const;

		/// @brief Validates that a CAN message has not caused an address violation for this ICF.
		/// If a violation is found, a re-claim will be executed for as is required by ISO 11783-5,
		/// and will attempt to activate a DTC that is defined in ISO 11783-5.
		/// This function is for advanced use cases only. Normally, the network manager will call this
		/// for every message received.
		/// @note Address violation occurs when two CFs are using the same source address.
		/// @param[in] message The message to process
		/// @returns true if the message caused an address violation, otherwise false
		bool process_rx_message_for_address_violation(const CANMessage &message);

	protected:
		friend class CANNetworkManager; ///< Allow the network manager to access the pgn request protocol
		std::shared_ptr<ParameterGroupNumberRequestProtocol> pgnRequestProtocol; ///< The PGN request protocol for this ICF

	private:
		/// @brief Sends the PGN request for the address claim PGN
		/// @returns true if the message was sent, otherwise false
		bool send_request_to_claim() const;

		/// @brief Sends the address claim message to the bus
		/// @param[in] address The address to claim
		/// @returns true if the message was sent, otherwise false
		bool send_address_claim(std::uint8_t address);

		/// @brief Sends the "cannot claim source address" message
		/// @note If a CF attempting to claim an SA is unsuccessful it shall send the cannot claim source address message
		/// @returns true if the message was sent, otherwise false
		bool send_cannot_claim_source_address();

		/// @brief Attempts to process a commanded address.
		/// @details If the state machine has claimed successfully before,
		/// this will attempt to move a NAME from the claimed address to the new, specified address.
		/// @param[in] commandedAddress The address to attempt to claim
		void process_commanded_address(std::uint8_t commandedAddress);

		/// @brief Setter for the state
		/// @param[in] value The new state
		void set_current_state(State value);

		static constexpr std::uint32_t ADDRESS_CONTENTION_TIME_MS = 250; ///< The time in milliseconds to wait for address contention

		State state = State::None; ///< The current state of the internal control function
		std::uint32_t stateChangeTimestamp_ms = 0; ///< A timestamp in milliseconds used for timing the address claiming process
		std::uint8_t preferredAddress; ///< The address we'd prefer to claim as (we may not get it)
		std::uint8_t randomClaimDelay_ms; ///< The random delay before claiming an address as required by the ISO11783 standard
		EventDispatcher<std::uint8_t> addressClaimedDispatcher; ///< The event dispatcher for when an address is claimed
	};

} // namespace isobus

#endif // CAN_INTERNAL_CONTROL_FUNCTION_HPP
