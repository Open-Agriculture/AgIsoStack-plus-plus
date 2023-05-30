//================================================================================================
/// @file isobus_maintain_power_interface.hpp
///
/// @brief Defines an interface for sending and receiving the maintain power message (PGN 65095).
/// @details This interface is for managing the maintain power message, which is a
/// message sent by any CF connected to the implement bus requesting that the Tractor
/// ECU (TECU) not switch off the power for 2s after it has received the wheel-based speed and
/// distance message indicating that the ignition has been switched off.
/// The message also includes the connected implement(s) operating state.
/// You can choose if the TECU maintains actuator power independently of ECU power as well.
/// You might want to maintain actuator power to ensure your section valves close when keyed off.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef ISOBUS_MAINTAIN_POWER_INTERFACE_HPP
#define ISOBUS_MAINTAIN_POWER_INTERFACE_HPP

#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_message.hpp"
#include "isobus/utility/processing_flags.hpp"

#include <memory>

namespace isobus
{
	/// @brief Manages sending and receiving the maintain power message (PGN 65095)
	class MaintainPowerInterface
	{
	public:
		/// @brief Stores information sent/received in a maintain power message.
		class MaintainPowerData
		{
		public:
			/// @brief 	Signal that indicates that an implement is connected to a tractor or power unit and is in work state.
			/// @details SPN 7447
			enum class ImplementInWorkState : std::uint8_t
			{
				ImplementNotInWorkState = 0,
				ImplementInWorkState = 1,
				ErrorIndication = 2,
				NotAvailable = 3
			};

			/// @brief Signal that indicates that an implement is connected to a tractor or power unit and is ready for work.
			/// @details SPN 1871
			enum class ImplementReadyToWorkState
			{
				ImplementNotReadyForFieldWork = 0,
				ImplementReadyForFieldWork = 1,
				ErrorIndication = 2,
				NotAvailable = 3
			};

			/// @brief Indicates the state of an implement where it may be disconnected from a tractor or power unit.
			/// @details SPN 1870
			enum class ImplementParkState
			{
				ImplementMayNotBeDisconnected = 0,
				ImplementMayBeDisconnected = 1,
				ErrorIndication = 2,
				NotAvailable = 3
			};

			/// @brief Indicates the transport state of an implement connected to a tractor or power unit.
			/// @details SPN 1869
			enum class ImplementTransportState
			{
				ImplementMayNotBeTransported = 0,
				ImplementMayBeTransported = 1,
				ErrorIndication = 2,
				NotAvailable = 3
			};

			/// @brief Constructor for a MaintainPowerData object, which stores information sent/received in a maintain power message.
			explicit MaintainPowerData(std::shared_ptr<ControlFunction> sendingControlFunction);

			/// @brief Sets the reported implement in-work state
			/// @param[in] inWorkState The reported implement in-work state to set
			/// @returns True if the set value was different from the stored value, otherwise false.
			bool set_implement_in_work_state(ImplementInWorkState inWorkState);

			/// @brief Returns the reported implement in-work state
			/// @return The reported implement in-work state
			ImplementInWorkState get_implement_in_work_state() const;

			/// @brief Sets the reported implement ready to work state
			/// @param[in] readyToWorkState The reported implement ready to work state
			/// @returns True if the set value was different from the stored value, otherwise false.
			bool set_implement_ready_to_work_state(ImplementReadyToWorkState readyToWorkState);

			/// @brief Returns the reported implement ready to work state
			/// @return The reported implement ready to work state
			ImplementReadyToWorkState get_implement_ready_to_work_state() const;

			/// @brief Sets the reported implement park state
			/// @param[in] parkState The reported implement park state to set
			/// @returns True if the set value was different from the stored value, otherwise false.
			bool set_implement_park_state(ImplementParkState parkState);

			/// @brief Returns the reported implement park state
			/// @returns The reported implement park state
			ImplementParkState get_implement_park_state() const;

			/// @brief Sets the reported implement transport state
			/// @param[in] transportState The reported implement transport state to set
			/// @returns True if the set value was different from the stored value, otherwise false.
			bool set_implement_transport_state(ImplementTransportState transportState);

			/// @brief Returns the reported implement transport state
			/// @returns The reported implement transport state
			ImplementTransportState get_implement_transport_state() const;

		private:
			MaintainPowerData() = delete;
			std::shared_ptr<ControlFunction> sendingControlFunction = nullptr; ///< The control function that is sending the message.
			std::uint32_t maintainActuatorPowerTime_ms = 0; ///< The amount of time to ask the TECU to maintain actuator/section power. Will be rounded up to the next 2s mark when sent.
			std::uint32_t maintainECUPowerTime_ms = 0; ///< The amount of time to ask the TECU to maintain ECU power. Will be rounded up to the next 2s mark when sent.
			std::uint32_t timestamp_ms = 0; ///< A timestamp for when the message was released in milliseconds
			ImplementInWorkState currentImplementInWorkState = ImplementInWorkState::NotAvailable; ///< The reported implement in-work state
			ImplementReadyToWorkState currentImplementReadyToWorkState = ImplementReadyToWorkState::NotAvailable; ///< The reported implement ready to work state
			ImplementParkState currentImplementParkState = ImplementParkState::NotAvailable; ///< The reported implement park state
			ImplementTransportState currentImplementTransportState = ImplementTransportState::NotAvailable; ///< The reported transport state of the implement
		};

		/// @brief Constructor for a MaintainPowerInterface
		/// @param[in] sourceControlFunction The control function to send the message from, or nullptr to listen only
		explicit MaintainPowerInterface(std::shared_ptr<InternalControlFunction> sourceControlFunction);

		/// @brief Destructor for a MaintainPowerInterface
		~MaintainPowerInterface();

		/// @brief Use this to configure the transmission of the maintain power message.
		MaintainPowerData maintainPowerTransmitData;

	protected:
		/// @brief Enumerates a set of flags to manage transmitting messages owned by this interface
		enum class TransmitFlags : std::uint32_t
		{
			SendMaintainPower = 0, ///< A flag to manage sending the maintain power message

			NumberOfFlags ///< The number of flags in this enumeration
		};

		/// @brief Sets up the class and registers it to receive callbacks from the network manager for processing
		/// messages. The class will not receive messages if this is not called.
		void initialize();

		/// @brief Returns if the interface has been initialized
		/// @returns true if initialize has been called for this interface, otherwise false
		bool get_initialized() const;

		/// @brief Transmits the maintain power message
		/// @returns True if the message was sent, otherwise false
		bool send_maintain_power() const;

		/// @brief Processes one flag (which sends the associated message)
		/// @param[in] flag The flag to process
		/// @param[in] parentPointer A pointer to the interface instance
		static void process_flags(std::uint32_t flag, void *parentPointer);

		/// @brief Processes a CAN message
		/// @param[in] message The CAN message being received
		/// @param[in] parentPointer A context variable to find the relevant instance of this class
		static void process_rx_message(const CANMessage &message, void *parentPointer);

	private:
		ProcessingFlags txFlags; ///< Tx flag for sending the maintain power message. Handles retries automatically.
		std::uint32_t keyNotOffTimestamp = 0; ///< A timestamp to track when the key was detected as ON, used to detect transitions to "Not On".
		std::uint32_t keyOffTimestamp = 0; ///< A timestamp to track when the key is off, used to calculate how many messages to send and when to send them.
		bool initialized = false;
	};
} // namespace isobus
#endif // ISOBUS_MAINTAIN_POWER_INTERFACE_HPP
