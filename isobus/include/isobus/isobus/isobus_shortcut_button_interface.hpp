//================================================================================================
/// @file isobus_shortcut_button_interface.hpp
///
/// @brief Defines an interface for communicating as or from an ISOBUS shortcut button (ISB).
/// Defined in AEF Guideline 004 - ISB and at https://www.isobus.net/isobus/pGNAndSPN/10936
/// (ISO 11783-7)
///
/// @details This interfaces manages the PGN used by isobus shortcut buttons (ISB).
/// You can choose to either receive this message, send it, or both. An ISB is essentially
/// a command to all implements to enter a safe state. See the description located at
/// https://www.isobus.net/isobus/pGNAndSPN/10936, ISO 11783-7, or
/// https://www.aef-online.org/fileadmin/user_upload/Content/pdfs/AEF_One_Pager.pdf
/// for more details.
///
/// @attention If you consume this message, you MUST implement an associated alarm in your
/// VT/UT object pool, along with an icon or other indication on your home screen that your
/// working set master supports ISB, as required for AEF conformance.
///
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#ifndef ISOBUS_SHORTCUT_BUTTON_INTERFACE_HPP
#define ISOBUS_SHORTCUT_BUTTON_INTERFACE_HPP

#include "isobus/isobus/can_NAME.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_message.hpp"
#include "isobus/utility/event_dispatcher.hpp"
#include "isobus/utility/processing_flags.hpp"

#include <array>
#include <functional>
#include <list>
#include <memory>

namespace isobus
{
	/// @brief An interface for communicating as or interpreting the messages of ISOBUS Shortcut Buttons
	/// @note This interface must be cyclically updated from your application since it's an application layer
	/// message. Be sure to call "update" from time to time. Suggested rate is at least every 500ms, but ideally every 100ms or faster.
	/// @details This interface parses the "All implements stop operations switch state" message
	/// that is sent by ISOBUS shortcut buttons, and also allows you to optionally transmit the same message
	/// as an ISOBUS shortcut button.
	///
	/// This message may be sent by any control function connected to the implement bus on forestry or
	/// agriculture implements providing to connected systems the current state of the all implement stop operations switch.
	/// At least one of these switches shall be in each operator location of the connected system.
	///
	/// All implements shall start a process to stop all operations when this broadcast message is received from any CF
	/// with a value of "Stop implement operations" (SPN 5140). Before an implement turns off all implement operations,
	/// it shall assume a failsafe condition. If an implement is operating in an automation mode,
	/// it may enter a failsafe condition before requesting the tractor ECU to exit the automation mode,
	/// e.g. PTO, Auxiliary valve, and/or tractor movement.
	///
	/// The working set master for the implement shall then inform the operator that the implement has stopped
	/// all operations due to the activation of the Stop All Implement Operations switch.
	/// Implement working set masters shall include, on their home screen, an indication,
	/// e.g. icon or a function name, if it supports Stop All Implement Operations.
	/// The Working Set shall monitor the number of transitions for each ISB server upon receiving first
	/// the message from a given ISB server. A Working Set shall consider an increase in the transitions
	/// without detecting a corresponding transition of the Stop all implement operations state as an error and react accordingly.
	class ShortcutButtonInterface
	{
	public:
		/// @brief Enumerates the states that can be sent in the main ISB message (pgn 64770, 0xFD02)
		enum class StopAllImplementOperationsState : std::uint8_t
		{
			StopImplementOperations = 0, ///< Stop implement operations
			PermitAllImplementsToOperationOn = 1, ///< Permit all implements to operation ON
			Error = 2, ///< Error indication
			NotAvailable = 3 ///< Not available
		};

		/// @brief Constructor for a ShortcutButtonInterface
		/// @param[in] internalControlFunction The InternalControlFunction that the interface will use to send messages (not nullptr)
		/// @param[in] serverEnabled Enables the interface's transmission of the "Stop all implement operations" message.
		ShortcutButtonInterface(std::shared_ptr<InternalControlFunction> internalControlFunction, bool serverEnabled = false);

		/// @brief Destructor for a ShortcutButtonInterface
		virtual ~ShortcutButtonInterface();

		/// @brief Used to initialize the interface. Registers for PGNs with the network manager.
		void initialize();

		/// @brief Returns if the interface has been initialized
		/// @returns true if the interface has been initialized
		bool get_is_initialized() const;

		/// @brief Gets the event dispatcher for when the assigned bus' ISB state changes.
		/// The assigned bus is determined by which internal control function you pass into the constructor.
		/// @returns The event dispatcher which can be used to register callbacks/listeners to
		EventDispatcher<StopAllImplementOperationsState> &get_stop_all_implement_operations_state_event_dispatcher();

		/// @brief Sets the state that the interface will broadcast on the bus.
		/// @note This is only used when the interface was created as a server.
		/// @param[in] newState The state to broadcast on the bus if the interface is a server
		void set_stop_all_implement_operations_state(StopAllImplementOperationsState newState);

		/// @brief Returns the current ISB state for the bus, which is a combination of the internal commanded
		/// state and the states reported by all other CFs.
		/// @returns The current ISB state for the bus associated with the interface's internal control function
		StopAllImplementOperationsState get_state() const;

		/// @brief This must be called cyclically to update the interface.
		/// This processes transmits and timeouts.
		void update();

	private:
		/// @brief Stores data about a sender of the stop all implement operations switch state
		class ISBServerData
		{
		public:
			/// @brief Constructor for ISBServerData, sets default values
			ISBServerData() = default;

			NAME ISONAME; ///< The ISONAME of the sender, used as a lookup key
			StopAllImplementOperationsState commandedState; ///< The last state we received from this ISB
			std::uint32_t messageReceivedTimestamp_ms = 0; ///< Tracks the last time we received a message from this ISB so we can time them out if needed
			std::uint8_t stopAllImplementOperationsTransitionNumber = 0; ///< Number of transitions from Permit (01) to Stop (00) since power up of the stop all implement operations parameter
		};

		/// @brief Enumerates a set of flags that the interface uses to know if it should transmit a message
		enum class TransmitFlags : std::uint32_t
		{
			SendStopAllImplementOperationsSwitchState = 0, ///< A flag to send the main ISB message

			NumberOfFlags ///< The number of flags defined in this enumeration
		};

		/// @brief Parses incoming CAN messages for the interface
		/// @param message The CAN message to parse
		/// @param parentPointer A generic context variable, usually the `this` pointer for this interface instance
		static void process_rx_message(const CANMessage &message, void *parentPointer);

		/// @brief Processes the internal Tx flags
		/// @param[in] flag The flag to process
		/// @param[in] parent A context variable to find the relevant interface class
		static void process_flags(std::uint32_t flag, void *parent);

		/// @brief A generic way for a protocol to process a received message
		/// @param[in] message A received CAN message
		void process_message(const CANMessage &message);

		/// @brief Sends the Stop all implement operations switch state message
		/// @returns true if the message was sent, otherwise false
		bool send_stop_all_implement_operations_switch_state() const;

		static constexpr std::uint32_t TRANSMISSION_RATE_MS = 1000; ///< The cyclic transmission time for PGN 0xFD02
		static constexpr std::uint32_t TRANSMISSION_TIMEOUT_MS = 3000; ///< Amount of time between messages until we consider an ISB stale (arbitrary, but similar to VT timeout)

		std::list<ISBServerData> isobusShorcutButtonList; ///< A list of all senders of the ISB messages used to track transition counts
		std::shared_ptr<InternalControlFunction> sourceControlFunction = nullptr; ///< The internal control function that the interface is assigned to and will use to transmit
		EventDispatcher<StopAllImplementOperationsState> ISBEventDispatcher; ///< Manages callbacks about ISB states
		ProcessingFlags txFlags; ///< A set of flags to manage retries while sending messages
		std::uint32_t allImplementsStopOperationsSwitchStateTimestamp_ms = 0; ///< A timestamp to track the need for cyclic transmission of PGN 0xFD02
		std::uint8_t stopAllImplementOperationsTransitionNumber = 0; ///< A counter used to track our transitions from "stop" to "permit" when acting as a server
		StopAllImplementOperationsState commandedState = StopAllImplementOperationsState::NotAvailable; ///< The state set by the user to transmit if we're acting as a server
		bool actAsISBServer = false; ///< A setting that enables sending the ISB messages rather than just receiving them
		bool initialized = false; ///< Stores if the interface has been initialized
	};
} // namespace isobus
#endif // ISOBUS_SHORTCUT_BUTTON_INTERFACE_HPP
