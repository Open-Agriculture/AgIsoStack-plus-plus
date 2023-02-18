//================================================================================================
/// @file isobus_task_controller_client.hpp
///
/// @brief A class to manage a client connection to a ISOBUS field computer's task controller
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#ifndef ISOBUS_TASK_CONTROLLER_CLIENT_HPP
#define ISOBUS_TASK_CONTROLLER_CLIENT_HPP

#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/isobus_language_command_interface.hpp"
#include "isobus/utility/processing_flags.hpp"

namespace isobus
{
	class TaskControllerClient
	{
	public:
		/// @brief Enumerates the different internal state machine states
		enum class StateMachineState
		{
			Disconnected, ///< Not communicating with the TC
			WaitForStartUpDelay, ///< Client is waiting for the mandatory 6s startup delay
			WaitForServerStatusMessage, ///< Client is waiting to identify the TC via reception of a valid status message
			SendWorkingSetMaster, ///< Client initating communication with TC by sending the working set master message
			SendStatusMessage,
			RequestVersion,
			WaitForRequestVersionResponse,
			WaitForRequestVersionFromServer,
			SendRequestVersionResponse,
			RequestLanguage,
			WaitForLanguageResponse,
			Connected
		};

		/// @brief Enumerates the different task controller versions
		enum class Version : std::uint8_t
		{
			DraftInternationalStandard = 0, ///< The version of the DIS (draft International Standard).
			FinalDraftInternationalStandardFirstEdition = 1, ///< The version of the FDIS.1 (final draft International Standard, first edition).
			FirstPublishedEdition = 2, ///< The version of the FDIS.2 and the first edition published ss an International Standard.
			SecondEditionDraft = 3, ///< The version of the second edition published as a draft International Standard(E2.DIS).
			SecondPublishedEdition = 4, ///< The version of the second edition published as the final draft International Standard(E2.FDIS) and as the International Standard(E2.IS)
			Unknown = 0xFF
		};

		enum class ServerOptions : std::uint8_t
		{
			SupportsDocumentation = 0x01,
			SupportsTCGEOWithoutPositionBasedControl = 0x02,
			SupportsTCGEOWithPositionBasedControl = 0x04,
			SupportsPeerControlAssignment = 0x08,
			SupportsImplementSectionControlFunctionality = 0x10,
			ReservedOption1 = 0x20,
			ReservedOption2 = 0x40,
			ReservedOption3 = 0x80
		};

		/// @brief The constructor for a TaskControllerClient
		/// @param[in] partner The TC server control function
		/// @param[in] clientSource The internal control function to communicate from
		TaskControllerClient(std::shared_ptr<PartneredControlFunction> partner, std::shared_ptr<InternalControlFunction> clientSource);

		/// @brief Destructor for the client
		~TaskControllerClient();

		// Setup Functions
		/// @brief This function starts the state machine. Call this once you have created your DDOP, set up the client capabilities, and are ready to connect.
		/// @param[in] spawnThread The client will start a thread to manage itself if this parameter is true. Otherwise you must update it cyclically
		/// by calling the `update` function.
		void initialize(bool spawnThread);

		/// @brief A convenient way to set all client options at once instead of calling the individual setters
		/// @details This function sets up the parameters that the client will report to the TC server.
		/// These parameters should be tailored to your specific application.
		/// @param[in] numberBoomsSupported Configures the max number of booms the client supports
		/// @param[in] numberSectionsSupported Configures the max number of sections supported by the client for section control
		/// @param[in] numberChannelsSupportedForPositionBasedControl Configures the max number of channels supported by the client for position based control
		/// @param[in] supportsDocumentation Denotes if your app supports documentation
		/// @param[in] supportsTCGEOWithoutPositionBasedControl Denotes if your app supports TC-GEO without position based control
		/// @param[in] supportsTCGEOWithPositionBasedControl Denotes if your app supports TC-GEO with position based control
		/// @param[in] supportsPeerControlAssignment Denotes if your app supports peer control assignment
		/// @param[in] supportsImplementSectionControl Denotes if your app supports implement section control
		void configure(std::uint8_t numberBoomsSupported,
		               std::uint8_t numberSectionsSupported,
		               std::uint8_t numberChannelsSupportedForPositionBasedControl,
		               bool supportsDocumentation,
		               bool supportsTCGEOWithoutPositionBasedControl,
		               bool supportsTCGEOWithPositionBasedControl,
		               bool supportsPeerControlAssignment,
		               bool supportsImplementSectionControl);

		// Calling this will stop the worker thread if it exists
		/// @brief Terminates the client and joins the worker thread if applicable
		void terminate();

		/// @brief Returns the previously configured number of booms supported by the client
		/// @returns The previously configured number of booms supported by the client
		std::uint8_t get_number_booms_supported() const;

		/// @brief Returns the previously configured number of section supported by the client
		/// @returns The previously configured number of booms supported by the client
		std::uint8_t get_number_sections_supported() const;

		/// @brief Returns the previously configured number of channels supported for position based control
		/// @returns The previously configured number of channels supported for position based control
		std::uint8_t get_number_channels_supported_for_position_based_control() const;

		/// @brief Returns if the client has been configured to report that it supports documentation to the TC
		/// @returns `true` if the client has been configured to report that it supports documentation, otherwise `false`
		bool get_supports_documentation() const;

		/// @brief Returns if the client has been configured to report that it supports TC-GEO without position based control to the TC
		/// @returns `true` if the client has been configured to report that it supports TC-GEO without position based control, otherwise `false`
		bool get_supports_tcgeo_without_position_based_control() const;

		/// @brief Returns if the client has been configured to report that it supports TC-GEO with position based control to the TC
		/// @returns `true` if the client has been configured to report that it supports TC-GEO with position based control, otherwise `false`
		bool get_supports_tcgeo_with_position_based_control() const;

		/// @brief Returns if the client has been configured to report that it supports peer control assignment to the TC
		/// @returns `true` if the client has been configured to report that it supports peer control assignment, otherwise `false`
		bool get_supports_peer_control_assignment() const;

		/// @brief Returns if the client has been configured to report that it supports implement section control to the TC
		/// @returns `true` if the client has been configured to report that it supports implement section control, otherwise `false`
		bool get_supports_implement_section_control() const;

		/// @brief Returns if the client has been initialized
		/// @note This does not mean that the client is connected to the TC server
		/// @returns true if the client has been initialized
		bool get_is_initialized() const;

		/// @brief Check whether the client is connected to the TC server
		/// @returns true if cconnected, false otherwise
		bool get_is_connected() const;

		/// @brief Returns the current state machine state
		/// @returns The current internal state machine state
		StateMachineState get_state() const;

		/// @brief Returns the number of booms that the connected TC supports for section control
		/// @returns Number of booms that the connected TC supports for section control
		std::uint8_t get_connected_tc_number_booms_supported() const;

		/// @brief Returns the number of sections that the connected TC supports for section control
		/// @returns Number of sections that the connected TC supports for section control
		std::uint8_t get_connected_tc_number_sections_supported() const;

		/// @brief Returns the number of channels that the connected TC supports for position control
		/// @returns Number of channels that the connected TC supports for position control
		std::uint8_t get_connected_tc_number_channels_supported() const;

		/// @brief Returns the maximum boot time in seconds reported by the connected TC
		/// @returns Maximum boot time (seconds) reported by the connected TC, or 0xFF if that info is not available.
		std::uint8_t get_connected_tc_max_boot_time() const;

		/// @brief Returns if the connected TC supports a certain option
		/// @param[in] option The option to check against
		/// @returns `true` if the option was reported as "supported" by the TC, otherwise `false`
		bool get_connected_tc_option_supported(ServerOptions option) const;

		/// @brief Returns the version of the connected task controller
		/// @returns The version reported by the connected task controller
		Version get_connected_tc_version() const;

		/// @brief The cyclic update function for this interface.
		/// @note This function may be called by the TC worker thread if you called
		/// initialize with a parameter of `true`, otherwise you must call it
		/// yourself at some interval.
		void update();

		/// @brief Used to determine the language and unit systems in use by the TC server
		LanguageCommandInterface languageCommandInterface;

	protected:
		/// @brief Enumerates the different Process Data commands from ISO11783-10 Table B.1
		enum class ProcessDataCommands : std::uint8_t
		{
			TechnicalCapabilities = 0x00, ///< Used for determining the technical capabilities of a TC, DL, or client.
			DeviceDescriptor = 0x01, ///< Subcommand for the transfer and management of device descriptors
			RequestValue = 0x02, ///< The value of the data entity specified by the data dictionary identifier is requested.
			Value = 0x03, ///< This command is used both to answer a request value command and to set the value of a process data entity.
			MeasurementTimeInterval = 0x04, ///< The process data value is the time interval for sending the data element specified by the data dictionary identifier.
			MeasurementDistanceInterval = 0x05, ///< The process data value is the distance interval for sending the data element specified by the data dictionary identifier.
			MeasurementMinimumWithinThreshold = 0x06, ///< The client has to send the value	of this data element to the TC or DL when the value is higher than the threshold value
			MeasurementMaximumWithinThreshold = 0x07, ///< The client has to send the value of this data element to the TC or DL when the value is lower than the threshold value.
			MeasurementChangeThreshold = 0x08, ///< The client has to send the value of this data element to the TC or DL when the value change is higher than or equal to the change threshold since last transmission.
			PeerControlAssignment = 0x09, ///< This message is used to establish a connection between a	setpoint value source and a setpoint value user
			SetValueAndAcknowledge = 0x0A, ///< This command is used to set the value of a process data entity and request a reception acknowledgement from the recipient
			Reserved1 = 0x0B, ///< Reserved.
			Reserved2 = 0x0C, ///< Reserved.
			ProcessDataAcknowledge = 0x0D, ///< Message is a Process Data Acknowledge (PDACK).
			StatusMessage = 0x0E, ///< Message is a Task Controller Status message
			ClientTask = 0x0F ///< Sent by the client
		};

		/// @brief Enumerates the subcommands within the technical data message group
		enum class TechnicalDataMessageCommands : std::uint8_t
		{
			ParameterRequestVersion = 0x00, ///< The Request Version message allows the TC, DL, and the client to determine the ISO 11783-10 version of the implementation
			ParameterVersion = 0x01, ///< The Version message is sent in response to the request version message and contains the ISO 11783-10 version information of the TC, DL, or client implementation
			IdentifyTaskController = 0x02 ///< Upon receipt of this message, the TC shall display, for a period of 3 s, the TC Number.
		};

		/// @brief Enumerates the subcommands within the device descriptor command message group
		enum class DeviceDescriptorCommands : std::uint8_t
		{
			RequestStructureLabel = 0x00, ///< Allows the client to determine the availability of the requested	device descriptor structure
			StructureLabel = 0x01, ///< The Structure Label message is sent by the TC or DL to inform the client about the availability of the requested version of the device descriptor structure
			RequestLocalizationLabel = 0x02, ///< Allows the client to determine the availability of the requested device descriptor localization
			LocalizationLabel = 0x03, ///< Sent by the TC or DL to inform the client about the availability of the requested localization version of the device descriptor
			RequestObjectPoolTransfer = 0x04, ///< The Request Object-pool Transfer message allows the client to determine whether it is allowed to transfer(part of) the device descriptor object pool to the TC
			RequestObjectPoolTransferResponse = 0x05, ///< Sent in response to Request Object-pool Transfer message
			ObjectPoolTransfer = 0x06, ///< Enables the client to transfer (part of) the device descriptor object pool to the TC
			ObjectPoolTransferResponse = 0x07, ///< Response to an object pool transfer
			ObjectPoolActivateDeactivate = 0x08, ///< sent by a client to complete its connection procedure to a TC or DL or to disconnect from a TC or DL.
			ObjectPoolActivateDeactivateResponse = 0x09, ///< sent by a client to complete its connection procedure to a TC or DL or to disconnect from a TC or DL.
			ObjectPoolDelete = 0x0A, ///< This is a message to delete the device descriptor object pool for the client that sends this message.
			ObjectPoolDeleteResponse = 0x0B, ///< TC response to a Object-pool Delete message
			ChangeDesignator = 0x0C, ///< This message is used to update the designator of an object.
			ChangeDesignatorResponse = 0x0D ///< Sent in response to Change Designator message
		};

		/// @brief Processes a CAN message destined for any TC client
		/// @param[in] message The CAN message being received
		/// @param[in] parentPointer A context variable to find the relevant TC client class
		static void process_rx_message(CANMessage *message, void *parentPointer);

		/// @brief Sends the response to a request for version from the TC
		/// @returns `true` if the message was sent, otherwise `false`
		bool send_request_version_response() const;

		/// @brief Sends the status message to the TC
		/// @returns `true` if the message was sent, otherwise false
		bool send_status() const;

		/// @brief Sends the version request message to the TC
		/// @returns `true` if the message was sent, otherwise `false`
		bool send_version_request() const;

		/// @brief Sends the working set master message
		/// @returns `true` if the message was sent, otherwise false
		bool send_working_set_master() const;

		/// @brief Changes the internal state machine state and updates the associated timestamp
		/// @param[in] newState The new state for the state machine
		void set_state(StateMachineState newState);

		static constexpr std::uint32_t SIX_SECOND_TIMEOUT_MS = 6000; ///< The startup delay time defined in the standard
		static constexpr std::uint16_t TWO_SECOND_TIMEOUT_MS = 2000; ///< Used for sending the status message to the TC

	private:
		std::shared_ptr<PartneredControlFunction> partnerControlFunction; ///< The partner control function this client will send to
		std::shared_ptr<InternalControlFunction> myControlFunction; ///< The internal control function the client uses to send from
		StateMachineState currentState = StateMachineState::Disconnected; ///< Tracks the internal state machine's current state
		std::uint32_t stateMachineTimestamp_ms = 0; ///< Timestamp that tracks when the state machine last changed states (in milliseconds)
		std::uint32_t controlFunctionValidTimestamp_ms = 0; ///< A timestamp to track when (in milliseconds) our internal control function becomes valid
		std::uint32_t statusMessageTimestamp_ms = 0; ///< Timestamp corresponding to the last time we sent a status message to the TC
		std::uint8_t numberOfWorkingSetMembers = 1; ///< The number of working set members that will be reported in the working set master message
		std::uint8_t tcStatusBitfield = 0; ///< The last received TC/DL status from the status message
		std::uint8_t sourceAddressOfCommandBeingExecuted = 0; ///< Source address of client for which the current command is being executed
		std::uint8_t commandBeingExecuted = 0; ///< The current command the TC is executing as reported in the status message
		std::uint8_t serverVersion = 0; ///< The detected version of the TC Server
		std::uint8_t maxServerBootTime_s = 0; ///< Maximum number of seconds from a power cycle to transmission of first “Task Controller Status message” or 0xFF
		std::uint8_t serverOptionsByte1 = 0; ///< The options specified in ISO 11783-10 that this TC, DL, or client meets (The definition of this byte is introduced in ISO11783-10 version 3)
		std::uint8_t serverOptionsByte2 = 0; ///< Reserved for ISO assignment, should be zero or 0xFF.
		std::uint8_t serverNumberOfBoomsForSectionControl = 0; ///< When reported by the TC, this is the maximum number of section control booms that are supported
		std::uint8_t serverNumberOfSectionsForSectionControl = 0; ///< When reported by the TC, this is the maximum number of sections that are supported (or 0xFF for version 2 and earlier).
		std::uint8_t serverNumberOfChannelsForPositionBasedControl = 0; ///< When reported by the TC, this is the maximum number of individual control channels that is supported
		std::uint8_t numberBoomsSupported = 0; ///< Stores the number of booms this client supports for section control
		std::uint8_t numberSectionsSupported = 0; ///< Stores the number of sections this client supports for section control
		std::uint8_t numberChannelsSupportedForPositionBasedControl = 0; ///< Stores the number of channels this client supports for position based control
		bool initialized = false; ///< Tracks the initialization state of the interface instance
		bool shouldTerminate = false; ///< This variable tells the worker thread to exit
		bool enableStatusMessage = false; ///< Enables sending the status message to the TC cyclically
		bool supportsDocumentation = false; ///< Determines if the client reports documentation support to the TC
		bool supportsTCGEOWithoutPositionBasedControl = false; ///< Determines if the client reports TC-GEO without position control capability to the TC
		bool supportsTCGEOWithPositionBasedControl = false; ///< Determines if the client reports TC-GEO with position control capability to the TC
		bool supportsPeerControlAssignment = false; ///< Determines if the client reports peer control assignment capability to the TC
		bool supportsImplementSectionControl = false; ///< Determines if the client reports implement section control capability to the TC
	};
} // namespace isobus

#endif // ISOBUS_TASK_CONTROLLER_CLIENT_HPP
