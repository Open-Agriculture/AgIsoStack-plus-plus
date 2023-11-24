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

#include "isobus/isobus/isobus_language_command_interface.hpp"
#include "isobus/isobus/can_constants.hpp"

namespace isobus
{
	class TaskControllerServer
	{
	public:
		enum class ObjectPoolActivationError : std::uint8_t
		{
			NoErrors = 0x00,
			ThereAreErrorsInTheDDOP = 0x01,
			TaskControllerRanOutOfMemoryDuringActivation = 0x02,
			AnyOtherError = 0x04,
			DifferentDDOPExistsWithSameStructureLabel = 0x08
		};

		enum class ObjectPoolDeletionErrors : std::uint8_t
		{
			ObjectPoolIsReferencedByTaskData = 0,
			ServerCannotCheckForObjectPoolReferences = 1,
			ErrorDetailsNotAvailable = 0xFF
		};

		enum class ObjectPoolErrorCodes : std::uint8_t
		{
			NoErrors = 0x00,
			MethodOrAttributeNotSupported = 0x01,
			UnknownObjectReference = 0x02,
			AnyOtherError = 0x04,
			DDOPWasDeletedFromVolatileMemory = 0x08
		};

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

		TaskControllerServer(std::shared_ptr<InternalControlFunction> internalControlFunction,
		                     std::uint8_t numberBoomsSupported,
		                     std::uint8_t numberSectionsSupported,
		                     std::uint8_t numberChannelsSupportedForPositionBasedControl,
		                     std::uint8_t optionsBitfield);

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

		void initialize();

		bool get_initialized() const;

		LanguageCommandInterface &get_language_command_interface();

		/// @brief This must be called cyclically for the interface to operate correctly.
		/// You can run this in a separate thread or in the main loop of your program, but
		/// it must be called at least around 2-3x as fast as your fastest triggered message.
		/// 50ms may be a good value to start with.
		void update();

	private:
		enum class TaskControllerVersion : std::uint8_t
		{
			DraftInternationalStandard = 0, ///< The version of the DIS (draft International Standard).
			FinalDraftInternationalStandardFirstEdition = 1, ///< The version of the FDIS.1 (final draft International Standard, first edition).
			FirstPublishedEdition = 2, ///< The version of the FDIS.2 and the first edition published ss an International Standard.
			SecondEditionDraft = 3, ///< The version of the second edition published as a draft International Standard(E2.DIS).
			SecondPublishedEdition = 4, ///< The version of the second edition published as the final draft International Standard(E2.FDIS) and as the International Standard(E2.IS)
			Unknown = 0xFF
		};

		/// @brief Processes messages received from task controller clients.
		/// @param message The message received from the client.
		/// @param parentPointer A context variable that can be used to find this class's instance.
		static void process_rx_message(const CANMessage &message, void *parentPointer);

		bool send_generic_process_data_default_payload(std::uint8_t multiplexer, std::shared_ptr<ControlFunction> destination) const;

		bool send_status_message() const;

		static constexpr std::uint32_t STATUS_MESSAGE_RATE_MS = 2000; ///< The rate at which status messages are sent to the clients in milliseconds.

		LanguageCommandInterface languageCommandInterface; ///< The language command interface used to communicate with the client which language/units are in use.
		std::shared_ptr<InternalControlFunction> serverControlFunction; ///< The control function used to communicate with the clients.
		std::mutex taskControllerMutex; ///< A mutex used to protect the task controller server from concurrent access.
		std::uint32_t lastStatusMessageTimestamp_ms = 0; ///< The timestamp of the last status message sent on the bus
		const std::uint8_t numberBoomsSupportedToReport;
		const std::uint8_t numberSectionsSupportedToReport;
		const std::uint8_t numberChannelsSupportedForPositionBasedControlToReport;
		const std::uint8_t optionsBitfieldToReport;
		std::uint8_t currentStatusByte = 0;
		std::uint8_t currentCommandByte = 0;
		std::uint8_t currentCommandSourceAddress = NULL_CAN_ADDRESS;
		bool initialized = false;
	};
} // namespace isobus

#endif // ISOBUS_TASK_CONTROLLER_SERVER_HPP
