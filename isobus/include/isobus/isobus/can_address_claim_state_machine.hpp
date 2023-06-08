//================================================================================================
/// @file can_address_claim_state_machine.hpp
///
/// @brief Defines a class for managing the address claiming process
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_ADDRESS_CLAIM_STATE_MACHINE_HPP
#define CAN_ADDRESS_CLAIM_STATE_MACHINE_HPP

#include "isobus/isobus/can_NAME.hpp"
#include "isobus/isobus/can_constants.hpp"

namespace isobus
{
	class CANMessage; ///< Forward declare CANMessage

	//================================================================================================
	/// @class AddressClaimStateMachine
	///
	/// @brief State machine for managing the J1939/ISO11783 address claim process
	///
	/// @details This class manages address claiming for internal control functions
	/// and keeps track of things like requests for address claim.
	//================================================================================================
	class AddressClaimStateMachine
	{
	public:
		/// @brief Defines the state machine states for address claiming
		enum class State
		{
			None, ///< Address claiming is uninitialized
			WaitForClaim, ///< State machine is waiting for the random delay time
			SendRequestForClaim, ///< State machine is sending the request for address claim
			WaitForRequestContentionPeriod, ///< State machine is waiting for the address claim contention period
			SendPreferredAddressClaim, ///< State machine is claiming the preferred address
			ContendForPreferredAddress, ///< State machine is contending the preferred address
			SendArbitraryAddressClaim, ///< State machine is claiming an address
			SendReclaimAddressOnRequest, ///< An ECU requested address claim, inform the bus of our current address
			UnableToClaim, ///< State machine could not claim an address
			AddressClaimingComplete ///< Address claiming is complete and we have an address
		};

		/// @brief The constructor of the state machine class
		/// @param[in] preferredAddressValue The address you prefer to claim
		/// @param[in] ControlFunctionNAME The NAME you want to claim
		/// @param[in] portIndex The CAN channel index to claim on
		AddressClaimStateMachine(std::uint8_t preferredAddressValue, NAME ControlFunctionNAME, std::uint8_t portIndex);

		/// @brief The destructor for the address claim state machine
		~AddressClaimStateMachine();

		/// @brief Returns the current state of the state machine
		/// @returns The current state of the state machine
		State get_current_state() const;

		/// @brief Attempts to process a commanded address.
		/// @details If the state machine has claimed successfully before,
		/// this will attempt to move a NAME from the claimed address to the new, specified address.
		/// @param[in] commandedAddress The address to attempt to claim
		void process_commanded_address(std::uint8_t commandedAddress);

		/// @brief Enables or disables the address claimer
		/// @param[in] value true if you want the class to claim, false if you want to be a sniffer only
		void set_is_enabled(bool value);

		/// @brief Returns if the address claimer is enabled
		/// @returns true if the class will address claim, false if in sniffing mode
		bool get_enabled() const;

		/// @brief Returns the address claimed by the state machine or 0xFE if none claimed
		/// @returns The address claimed by the state machine or 0xFE if no address has been claimed
		std::uint8_t get_claimed_address() const;

		/// @brief Updates the state machine, should be called periodically
		void update();

	private:
		/// @brief Processes a CAN message
		/// @param[in] message The CAN message being received
		/// @param[in] parentPointer A context variable to find the relevant address claimer
		static void process_rx_message(const CANMessage &message, void *parentPointer);

		/// @brief Sets the current state machine state
		void set_current_state(State value);

		/// @brief Sends the PGN request for the address claim PGN
		bool send_request_to_claim() const;

		/// @brief Sends the address claim message
		/// @param[in] address The address to claim
		bool send_address_claim(std::uint8_t address);

		NAME m_isoname; ///< The ISO NAME to claim as
		State m_currentState = State::None; ///< The address claim state machine state
		std::uint32_t m_timestamp_ms = 0; ///< A generic timestamp in milliseconds used to find timeouts
		std::uint8_t m_portIndex; ///< The CAN channel index to claim on
		std::uint8_t m_preferredAddress; ///< The address we'd prefer to claim as (we may not get it)
		std::uint8_t m_randomClaimDelay_ms; ///< The random delay as required by the ISO11783 standard
		std::uint8_t m_claimedAddress = NULL_CAN_ADDRESS; ///< The actual address we ended up claiming
		bool m_enabled = true; ///<  Enable/disable state for this state machine
	};

} // namespace isobus

#endif // CAN_ADDRESS_CLAIM_STATE_MACHINE_HPP
