//================================================================================================
/// @file isobus_task_controller_server.hpp
///
/// @brief An abstract task controller server class. You can consume this file and implement
/// the pure virtual functions to create your own task controller or data logger server.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef ISOBUS_TASK_CONTROLLER_SERVER_HPP
#define ISOBUS_TASK_CONTROLLER_SERVER_HPP

#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/isobus_language_command_interface.hpp"

#include <deque>

namespace isobus
{
	class TaskControllerServer
	{
	public:
		/// @brief Enumerates the different error codes that can be returned when activating
		/// a device descriptor object pool.
		enum class ObjectPoolActivationError : std::uint8_t
		{
			NoErrors = 0x00,
			ThereAreErrorsInTheDDOP = 0x01,
			TaskControllerRanOutOfMemoryDuringActivation = 0x02,
			AnyOtherError = 0x04,
			DifferentDDOPExistsWithSameStructureLabel = 0x08
		};

		/// @brief Enumerates the different error codes that can be returned when deleting
		/// a device descriptor object pool.
		enum class ObjectPoolDeletionErrors : std::uint8_t
		{
			ObjectPoolIsReferencedByTaskData = 0,
			ServerCannotCheckForObjectPoolReferences = 1,
			ErrorDetailsNotAvailable = 0xFF
		};

		/// @brief Enumerates the different error codes that can be returned when processing a DDOP
		enum class ObjectPoolErrorCodes : std::uint8_t
		{
			NoErrors = 0x00,
			MethodOrAttributeNotSupported = 0x01,
			UnknownObjectReference = 0x02,
			AnyOtherError = 0x04,
			DDOPWasDeletedFromVolatileMemory = 0x08
		};

		/// @brief Enumerates the different process data commands that can be sent to the client.
		enum class ProcessDataCommands : std::uint8_t
		{
			TechnicalCapabilities = 0x00, ///< Used for determining the technical capabilities of a TC, DL, or client.
			DeviceDescriptor = 0x01, ///< Transfer and management of device descriptors
			RequestValue = 0x02, ///< Used when the value of the data entity specified by the data dictionary identifier is requested.
			Value = 0x03, ///< This command is used both to answer a request value command and to set the value of a process data entity.
			MeasurementTimeInterval = 0x04, ///< The process data value is the time interval for sending the data element specified by the data dictionary identifier.
			MeasurementDistanceInterval = 0x05, ///< The process data value is the distance interval for sending the data element specified by the data dictionary identifier.
			MeasurementMinimumWithinThreshold = 0x06, ///< The client has to send the value	of this data element to the TC or DL when the value is higher than the threshold value
			MeasurementMaximumWithinThreshold = 0x07, ///< The client has to send the value of this data element to the TC or DL when the value is lower than the threshold value.
			MeasurementChangeThreshold = 0x08, ///< The client has to send the value of this data element to the TC or DL when the value change is higher than or equal to the change threshold since last transmission.
			PeerControlAssignment = 0x09, ///< This message is used to establish a connection between a setpoint value source and a setpoint value user
			SetValueAndAcknowledge = 0x0A, ///< This command is used to set the value of a process data entity and request a reception acknowledgement from the recipient
			Reserved = 0x0B, ///< Reserved.
			Reserved2 = 0x0C, ///< Reserved.
			Acknowledge = 0x0D, ///< Message is a Process Data Acknowledge (PDACK).
			Status = 0x0E, ///< Message is a Task Controller Status message
			ClientTask = 0x0F ///< Sent by the client as a status message every 2s
		};

		/// @brief Enumerates the different options that can be reported by the server.
		/// Each option is a bit in a bitfield, with 1 meaning the option is supported and 0 meaning it is not.
		/// For example, if the server supports documentation and peer control assignment, but not the other options,
		/// the bitfield would be 0b00001001.
		enum ServerOptions : std::uint8_t
		{
			SupportsDocumentation = 0x01,
			SupportsTCGEOWithoutPositionBasedControl = 0x02,
			SupportsTCGEOWithPositionBasedControl = 0x04,
			SupportsPeerControlAssignment = 0x08,
			SupportsImplementSectionControl = 0x10,
			ReservedOption1 = 0x20,
			ReservedOption2 = 0x40,
			ReservedOption3 = 0x80
		};

		/// @brief Constructor for a TC server.
		/// @param internalControlFunction The control function to use to communicate with the clients.
		/// @param numberBoomsSupported The number of booms to report as supported by the TC.
		/// @param numberSectionsSupported The number of sections to report as supported by the TC.
		/// @param numberChannelsSupportedForPositionBasedControl The number of channels to report as supported by the TC.
		/// @param optionsBitfield The options bitfield to report as supported by the TC. See ServerOptions enum for the bit definitions.
		TaskControllerServer(std::shared_ptr<InternalControlFunction> internalControlFunction,
		                     std::uint8_t numberBoomsSupported,
		                     std::uint8_t numberSectionsSupported,
		                     std::uint8_t numberChannelsSupportedForPositionBasedControl,
		                     std::uint8_t optionsBitfield);

		/// @brief Deleted copy constructor
		TaskControllerServer(TaskControllerServer &) = delete;

		/// @brief Deleted assignment operator
		TaskControllerServer &operator=(const TaskControllerServer &) = delete;

		// **** Functions to be implemented by the consumer of the library ****
		virtual bool activate_object_pool(std::shared_ptr<ControlFunction> clientControlFunction, ObjectPoolActivationError &activationError, ObjectPoolErrorCodes &objectPoolError, std::uint16_t &parentObjectIDOfFaultyObject, std::uint16_t &faultyObjectID) = 0;

		virtual bool change_designator(std::shared_ptr<ControlFunction> clientControlFunction, std::uint16_t objectIDToAlter, const std::vector<std::uint8_t> &designator) = 0;

		virtual bool deactivate_object_pool(std::shared_ptr<ControlFunction> clientControlFunction) = 0;

		virtual bool delete_device_descriptor_object_pool(std::shared_ptr<ControlFunction> clientControlFunction, ObjectPoolDeletionErrors &returnedErrorCode) = 0;

		virtual bool get_is_stored_device_descriptor_object_pool_by_structure_label(std::shared_ptr<ControlFunction> clientControlFunction, const std::vector<std::uint8_t> &structureLabel, const std::vector<std::uint8_t> &extendedStructureLabel) = 0;

		virtual bool get_is_stored_device_descriptor_object_pool_by_localization_label(std::shared_ptr<ControlFunction> clientControlFunction, const std::array<std::uint8_t, 7> &localizationLabel) = 0;

		virtual bool get_is_enough_memory_available(std::uint32_t numberBytesRequired) = 0;

		virtual std::uint32_t get_number_of_complete_object_pools_stored_for_client(std::shared_ptr<ControlFunction> clientControlFunction) = 0;

		virtual void identify_task_controller(std::uint8_t taskControllerNumber) = 0;

		virtual void on_client_timeout(std::shared_ptr<ControlFunction> clientControlFunction) = 0;

		virtual void on_process_data_acknowledge(std::shared_ptr<ControlFunction> clientControlFunction, std::uint16_t dataDescriptionIndex, std::uint16_t elementNumber, std::uint8_t errorCodesFromClient, ProcessDataCommands processDataCommand) = 0;

		virtual bool on_value_command(std::shared_ptr<ControlFunction> clientControlFunction, std::uint16_t dataDescriptionIndex, std::uint16_t elementNumber, std::int32_t processDataValue, std::uint8_t &errorCodes) = 0;

		virtual bool store_device_descriptor_object_pool(std::shared_ptr<ControlFunction> clientControlFunction, const std::vector<std::uint8_t> &objectPoolData, bool appendToPool) = 0;

		// **** Functions used to communicate with the client ****

		/// @brief Sends a request to a client for an element's value of a particular DDI.
		/// @param apDestination The control function to send the message to
		/// @param aDataDescriptionIndex The Data Description Index being requested
		/// @param aElementNumber The element number being requested
		/// @returns true if the message was sent, otherwise false
		bool send_request_value(std::shared_ptr<ControlFunction> clientControlFunction, std::uint16_t dataDescriptionIndex, std::uint16_t elementNumber) const;

		/// @brief Sends a time interval measurement command.
		/// The process data value for this command is the time interval for sending the data element
		/// specified by the data dictionary identifier.The client has to send the value of this data
		/// element to the TC or DL cyclic with this time interval.
		/// @param clientControlFunction The control function to send the message to
		/// @param dataDescriptionIndex The data description index of the data element to send the command for
		/// @param elementNumber The element number of the data element to send the command for
		/// @param timeInterval The time interval for sending the data element specified by the data dictionary identifier.
		/// @returns true if the message was sent, otherwise false
		bool send_time_interval_measurement_command(std::shared_ptr<ControlFunction> clientControlFunction, std::uint16_t dataDescriptionIndex, std::uint16_t elementNumber, std::uint32_t timeInterval) const;

		bool send_distance_interval_measurement_command(std::shared_ptr<ControlFunction> clientControlFunction, std::uint16_t dataDescriptionIndex, std::uint16_t elementNumber, std::uint32_t distanceInterval) const;

		bool send_minimum_threshold_measurement_command(std::shared_ptr<ControlFunction> clientControlFunction, std::uint16_t dataDescriptionIndex, std::uint16_t elementNumber, std::uint32_t minimum) const;

		bool send_maximum_threshold_measurement_command(std::shared_ptr<ControlFunction> clientControlFunction, std::uint16_t dataDescriptionIndex, std::uint16_t elementNumber, std::uint32_t maximum) const;

		bool send_change_threshold_measurement_command(std::shared_ptr<ControlFunction> clientControlFunction, std::uint16_t dataDescriptionIndex, std::uint16_t elementNumber, std::uint32_t threshold) const;

		bool send_set_value_and_acknowledge(std::shared_ptr<ControlFunction> clientControlFunction, std::uint16_t dataDescriptionIndex, std::uint16_t elementNumber, std::uint32_t processDataValue) const;

		/// @brief Use this to set the reported task state in the status message.
		/// Basically, this should be set to true when the user starts a job, and false when the user stops a job.
		/// @note Don't be like some terminals which set this to true all the time, that's very annoying for the client.
		/// @param isTaskActive Whether a task is currently active or not.
		void set_task_totals_active(bool isTaskActive);

		/// @brief Returns whether a task is currently active or not.
		/// @returns Whether a task is currently active or not.
		bool get_task_totals_active() const;

		/// @brief Returns the language command interface used to communicate with the client which language/units are in use.
		/// The language command is very important for the TC to function correctly, so it is recommended that you call this
		/// function and configure the language command interface before calling initialize().
		/// @returns The language command interface used to communicate with the client which language/units are in use.
		LanguageCommandInterface &get_language_command_interface();

		// **** Functions used to initialize and run the server ****

		/// @brief Initializes the task controller server.
		void initialize();

		/// @brief Returns whether or not the task controller server has been initialized.
		bool get_initialized() const;

		/// @brief This must be called cyclically for the interface to operate correctly.
		/// You can run this in a separate thread or in the main loop of your program, but
		/// it must be called at least around 2-3x as fast as your fastest triggered message.
		/// 50ms may be a good value to start with.
		void update();

	private:
		/// @brief Enumerates the different versions of the task controller standard.
		enum class TaskControllerVersion : std::uint8_t
		{
			DraftInternationalStandard = 0, ///< The version of the DIS (draft International Standard).
			FinalDraftInternationalStandardFirstEdition = 1, ///< The version of the FDIS.1 (final draft International Standard, first edition).
			FirstPublishedEdition = 2, ///< The version of the FDIS.2 and the first edition published ss an International Standard.
			SecondEditionDraft = 3, ///< The version of the second edition published as a draft International Standard(E2.DIS).
			SecondPublishedEdition = 4, ///< The version of the second edition published as the final draft International Standard(E2.FDIS) and as the International Standard(E2.IS)
			Unknown = 0xFF
		};

		/// @brief Enumerates the different status bits that can be sent in the status message.
		enum class ServerStatusBit : std::uint8_t
		{
			TaskTotalsActive = 0x01,
			BusySavingDataToNVM = 0x02,
			BusyReadingDataFromNVM = 0x04,
			BusyExecutingACommand = 0x08,
			OutOfMemory = 0x80,
		};

		/// @brief Stores messages received from task controller clients for processing later.
		/// @param message The message received from the client.
		/// @param parentPointer A context variable that can be used to find this class's instance.
		static void store_rx_message(const CANMessage &message, void *parentPointer);

		/// @brief Processes messages received from task controller clients.
		void process_rx_messages();

		bool send_generic_process_data_default_payload(std::uint8_t multiplexer, std::shared_ptr<ControlFunction> destination) const;

		/// @brief Sends a measurement command to the client.
		/// @param clientControlFunction The control function to send the message to
		/// @param commandValue The command value to send
		/// @param dataDescriptionIndex The data description index of the data element to send the command for
		/// @param elementNumber The element number of the data element to send the command for
		/// @param processDataValue The process data value to send
		/// @returns true if the message was sent, otherwise false
		bool send_measurement_command(std::shared_ptr<ControlFunction> clientControlFunction, std::uint8_t commandValue, std::uint16_t dataDescriptionIndex, std::uint16_t elementNumber, std::uint32_t processDataValue) const;

		/// @brief Sends a status message broadcast
		/// @returns true if the message was sent, otherwise false
		bool send_status_message() const;

		static constexpr std::uint32_t STATUS_MESSAGE_RATE_MS = 2000; ///< The rate at which status messages are sent to the clients in milliseconds.

		LanguageCommandInterface languageCommandInterface; ///< The language command interface used to communicate with the client which language/units are in use.
		std::shared_ptr<InternalControlFunction> serverControlFunction; ///< The control function used to communicate with the clients.
		std::deque<CANMessage> rxMessageQueue; ///< A queue of messages received from the clients which will be processed when update is called.
		std::mutex taskControllerMutex; ///< A mutex used to protect the task controller server from concurrent access.
		std::uint32_t lastStatusMessageTimestamp_ms = 0; ///< The timestamp of the last status message sent on the bus
		const std::uint8_t numberBoomsSupportedToReport; ///< The number of booms that will be reported as supported by the TC.
		const std::uint8_t numberSectionsSupportedToReport; ///< The number of sections that will be reported as supported by the TC.
		const std::uint8_t numberChannelsSupportedForPositionBasedControlToReport; ///< The number of channels that will be reported as supported by the TC.
		const std::uint8_t optionsBitfieldToReport; ///< The options bitfield that will be reported as supported by the TC.
		std::uint8_t currentStatusByte = 0; ///< The current status byte to send in the status message.
		std::uint8_t currentCommandByte = 0; ///< The current command byte to send in the status message.
		std::uint8_t currentCommandSourceAddress = NULL_CAN_ADDRESS; ///< The current command source address to send in the status message.
		bool initialized = false; ///< Whether or not the task controller server has been initialized.
	};
} // namespace isobus

#endif // ISOBUS_TASK_CONTROLLER_SERVER_HPP
