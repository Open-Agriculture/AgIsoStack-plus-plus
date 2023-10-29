//================================================================================================
/// @file isobus_virtual_terminal_server.hpp
///
/// @brief An abstract VT server.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef ISOBUS_VIRTUAL_TERMINAL_SERVER_HPP
#define ISOBUS_VIRTUAL_TERMINAL_SERVER_HPP

#include "isobus/isobus/can_callbacks.hpp"
#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/isobus_language_command_interface.hpp"
#include "isobus/isobus/isobus_virtual_terminal_base.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"
#include "isobus/utility/event_dispatcher.hpp"

namespace isobus
{
	class VirtualTerminalServer : public VirtualTerminalBase
	{
	public:
		VirtualTerminalServer(std::shared_ptr<InternalControlFunction> controlFunctionToUse);

		~VirtualTerminalServer();

		void initialize();
		bool get_initialized() const;
		std::shared_ptr<VirtualTerminalServerManagedWorkingSet> get_active_working_set() const;

		/// @brief The Button Activation message allows the VT to transmit operator selection of a Button object to the Working
		/// Set Master
		/// @param[in] activationCode 0 for released, 1 for "pressed", 2 for "still held", or 3 for "aborted"
		/// @param[in] objectId Object ID of Button object
		/// @param[in] parentObjectId Object ID of parent Data Mask or in the case where the Button is in a visible Window Mask object, the Object ID of the Window Mask object
		/// @param[in] keyNumber Button key code (see ISO11783-6)
		/// @param[in] destination The VT client to send the message to
		/// @returns true if the message was sent, otherwise false
		bool send_button_activation_message(KeyActivationCode activationCode, std::uint16_t objectId, std::uint16_t parentObjectId, std::uint8_t keyNumber, std::shared_ptr<ControlFunction> destination) const;

		/// @brief Sends the VT Change Numeric Value message
		/// @details The VT sends this message any time the operator enters a numeric value for an input object or variable,
		/// regardless of whether or not the value changed.This message is not sent if the input was aborted(in this case a VT ESC message would be sent instead).For input objects that have a numeric variable reference,
		/// the Object ID of the numeric variable object is used in this message.
		/// @param[in] objectId The object ID of the affected object
		/// @param[in] value The value that the referenced object was changed to
		/// @param[in] destination The control function to send the message to
		/// @returns True if the message was sent, otherwise false
		bool send_change_numeric_value_message(std::uint16_t objectId, std::uint32_t value, std::shared_ptr<ControlFunction> destination) const;

		/// @brief Sends the VT Select Input Object message
		/// @details This message is sent by the VT any time an input field, Button, or Key object is selected (gets focus),
		/// deselected (loses focus), opened for edit or closed after edit by the operator or an ESC command.
		/// @param[in] objectId The object ID of the affected object
		/// @param[in] isObjectSelected True of the object has focus, false if the object has been deselected
		/// @param[in] isObjectOpenForInput True if the object is open for data input (only is object is selected!), otherwise false
		/// @param[in] destination The control function to send the message to
		/// @returns True if the message was sent, otherwise false
		bool send_select_input_object_message(std::uint16_t objectId, bool isObjectSelected, bool isObjectOpenForInput, std::shared_ptr<ControlFunction> destination) const;

		/// @brief The Button Activation message allows the VT to transmit operator selection of a key object to the Working
		/// Set Master
		/// @param[in] activationCode 0 for released, 1 for "pressed", 2 for "still held", or 3 for "aborted"
		/// @param[in] objectId Object ID of Button object
		/// @param[in] parentObjectId Object ID of parent Data Mask or in the case where the Button is in a visible Window Mask object, the Object ID of the Window Mask object
		/// @param[in] keyNumber Button key code (see ISO11783-6)
		/// @param[in] destination The VT client to send the message to
		/// @returns true if the message was sent, otherwise false
		bool send_soft_key_activation_message(KeyActivationCode activationCode, std::uint16_t objectId, std::uint16_t parentObjectId, std::uint8_t keyNumber, std::shared_ptr<ControlFunction> destination) const;

		// ----------- Mandatory Functions you must implement -----------------------
		virtual bool get_is_enough_memory(std::uint32_t requestedMemory) const = 0;
		virtual VTVersion get_version() const = 0;
		virtual std::uint8_t get_number_of_navigation_soft_keys() const = 0;
		virtual std::uint8_t get_soft_key_descriptor_x_pixel_width() const = 0;
		virtual std::uint8_t get_soft_key_descriptor_y_pixel_width() const = 0;
		virtual std::uint8_t get_number_of_possible_virtual_soft_keys_in_soft_key_mask() const = 0;
		virtual std::uint8_t get_number_of_physical_soft_keys() const = 0;
		virtual std::uint16_t get_data_mask_area_size_x_pixels() const = 0;
		virtual std::uint16_t get_data_mask_area_size_y_pixels() const = 0;
		virtual void suspend_working_set(std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSetWithError) = 0;
		virtual SupportedWideCharsErrorCode get_supported_wide_chars(std::uint8_t codePlane,
		                                                             std::uint16_t firstWideCharInInquiryRange,
		                                                             std::uint16_t lastWideCharInInquiryRange,
		                                                             std::uint8_t &numberOfRanges,
		                                                             std::vector<std::uint8_t> &wideCharRangeArray) = 0;

		virtual std::vector<std::uint8_t> get_versions(NAME clientNAME) = 0;
		virtual std::vector<std::uint8_t> get_supported_objects() const = 0;

		/// @brief This function is called when the client wants the server to load a previously stored object pool.
		/// If there exists in the VT's non-volatile memory an object pool matching the provided version label,
		/// return it. If one does not exist, return an empty vector.
		/// @param[in] versionLabel The object pool version to load for the given client NAME
		/// @param[in] clientNAME The client requesting the object pool
		/// @returns The requested object pool associated with the version label.
		virtual std::vector<std::uint8_t> load_version(const std::vector<std::uint8_t> &versionLabel, NAME clientNAME) = 0;

		/// @brief This function is called when the client wants the server to save an object pool
		/// to the VT's non-volatile memory.
		/// If the object pool is saved successfully, return true, otherwise return false.
		/// @note This may be called multiple times with the same version, but different data. When this
		/// happens, the expectation is that you will append each objectPool together into one large file.
		/// @param[in] objectPool The object pool data to save
		/// @param[in] versionLabel The object pool version to save for the given client NAME
		/// @param[in] clientNAME The client requesting the object pool
		/// @returns The requested object pool associated with the version label.
		virtual bool save_version(const std::vector<std::uint8_t> &objectPool, const std::vector<std::uint8_t> &versionLabel, NAME clientNAME) = 0;

		//------------ Optional functions you can override --------------------
		virtual VirtualTerminalBase::GraphicMode get_graphic_mode() const;
		virtual std::uint8_t get_powerup_time() const;
		virtual std::uint8_t get_supported_small_fonts_bitfield() const;
		virtual std::uint8_t get_supported_large_fonts_bitfield() const;

		//-------------- Callbacks/Event driven interface ---------------------
		EventDispatcher<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, std::uint16_t, std::uint16_t> &get_on_change_active_mask_event_dispatcher();
		EventDispatcher<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, std::uint16_t, bool> &get_on_hide_show_object_event_dispatcher();
		EventDispatcher<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, std::uint16_t, bool> &get_on_enable_disable_object_event_dispatcher();
		EventDispatcher<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, std::uint16_t, std::uint32_t> &get_on_change_numeric_value_event_dispatcher();
		EventDispatcher<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, std::uint16_t, std::uint16_t, std::int8_t, std::int8_t> &get_on_change_child_location_event_dispatcher();

		//----------------- Other Server Settings -----------------------------
		LanguageCommandInterface &get_language_command_interface();

	protected:
		enum class ChangeActiveMaskErrorBit : std::uint8_t
		{
			InvalidWorkingSetObjectID = 0,
			InvalidMaskObjectID = 1,
			AnyOtherError = 4
		};

		enum class ChangeChildLocationErrorBit : std::uint8_t
		{
			ParentObjectDoesntExistOrIsNotAParentOfSpecifiedObject = 0,
			TargetObjectDoesNotExistOrIsNotApplicable = 1,
			AnyOtherError = 4
		};

		enum class ChangeNumericValueErrorBit : std::uint8_t
		{
			InvalidObjectID = 0,
			InvalidValue = 1,
			ValueInUse = 2, // such as: open for input
			Undefined = 3,
			AnyOtherError = 4
		};

		enum class EnableDisableObjectErrorBit : std::uint8_t
		{
			Undefined = 0,
			InvalidObjectID = 1,
			InvalidEnableDisableCommandValue = 2,
			CouldNotCompleteTheInputObjectIsCurrentlyBeingModified = 3,
			AnyOtherError = 4
		};

		enum class HideShowObjectErrorBit : std::uint8_t
		{
			ReferencesToMissingChildObjects = 0,
			InvalidObjectID = 1,
			CommandError = 2,
			Undefined = 3,
			AnyOtherError = 4
		};

		/// @brief Checks to see if the message should be listened to based on
		/// what the message is, and if the client has sent the proper working set master message
		bool check_if_source_is_managed(const CANMessage &message);

		/// @brief Processes a CAN message from any VT client
		/// @param[in] message The CAN message being received
		/// @param[in] parentPointer A context variable to find the relevant VT server class
		static void process_rx_message(const CANMessage &message, void *parent);

		/// @brief Sends a message using the acknowledgement PGN
		/// @param[in] type The type of acknowledgement to send (Ack, vs Nack, etc)
		/// @param[in] parameterGroupNumber The PGN to acknowledge
		/// @param[in] source The source control function to send from
		/// @param[in] destination The destination control function to send the acknowledgement to
		/// @returns true if the message was sent, false otherwise
		bool send_acknowledgement(AcknowledgementType type, std::uint32_t parameterGroupNumber, std::shared_ptr<InternalControlFunction> source, std::shared_ptr<ControlFunction> destination);

		/// @brief Sends a response to a change active mask command
		/// @param[in] newMaskObjectID The object ID for the new active mask
		/// @param[in] errorBitfield An error bitfield
		/// @param[in] destination The control function to send the message to
		/// @returns true if the message was sent, otherwise false
		bool send_change_active_mask_response(std::uint16_t newMaskObjectID, std::uint8_t errorBitfield, std::shared_ptr<ControlFunction> destination);

		/// @brief Sends a response to a change child location command
		/// @param[in] parentObjectID The object ID for the parent of the object to move
		/// @param[in] objectID The object ID for the object to move
		/// @param[in] errorBitfield An error bitfield
		/// @param[in] destination The control function to send the message to
		/// @returns true if the message was sent, otherwise false
		bool send_change_child_location_response(std::uint16_t parentObjectID, std::uint16_t objectID, std::uint8_t errorBitfield, std::shared_ptr<ControlFunction> destination);

		/// @brief Sends a response to a change numeric value command
		/// @param[in] objectID The object ID for the object whose numeric value was meant to be changed
		/// @param[in] errorBitfield An error bitfield
		/// @param[in] value The value that was set by the client
		/// @param[in] destination The control function to send the message to
		/// @returns true if the message was sent, otherwise false
		bool send_change_numeric_value_response(std::uint16_t objectID, std::uint8_t errorBitfield, std::uint32_t value, std::shared_ptr<ControlFunction> destination);

		/// @brief Sends a response to the enable/disable object command
		/// @param[in] objectID The object ID for the object
		/// @param[in] errorBitfield An error bitfield
		/// @param[in] value The enable/disable state that was set by the client
		/// @param[in] destination The control function to send the message to
		/// @returns true if the message was sent, otherwise false
		bool send_enable_disable_object_response(std::uint16_t objectID, std::uint8_t errorBitfield, bool value, std::shared_ptr<ControlFunction> destination);

		/// @brief This message is sent by the VT to a Working Set Master to acknowledge the End of Object Pool message.
		/// @details When the VT replies with an error of any type
		/// the VT should delete the object pool from volatile memory storage and inform the operator
		/// by an alarm type method of the suspension of the Working Set and indicate the reason for
		/// the deletion. On reception of this message, the responsible ECU(s) should enter a failsafe
		/// operation mode providing a safe shutdown procedure of the whole device.
		/// @param[in] success Indicates if the pool was error free
		/// @param[in] parentIDOfFaultingObject The parent object ID for the faulty object, or NULL_OBJECT_ID
		/// @param[in] faultingObjectID The faulty object's ID or the NULL_OBJECT_ID
		/// @param[in] errorCodes A bitfield of error codes that describe the issues with the pool
		/// @returns true if the message was sent, otherwise false
		bool send_end_of_object_pool_response(bool success,
		                                      std::uint16_t parentIDOfFaultingObject,
		                                      std::uint16_t faultingObjectID,
		                                      std::uint8_t errorCodes,
		                                      std::shared_ptr<ControlFunction> destination);

		/// @brief Sends a response to the hide/show object command
		/// @param[in] objectID The object ID for the object
		/// @param[in] errorBitfield An error bitfield
		/// @param[in] value The hide/show state that was set by the client
		/// @param[in] destination The control function to send the message to
		/// @returns true if the message was sent, otherwise false
		bool send_hide_show_object_response(std::uint16_t objectID, std::uint8_t errorBitfield, bool value, std::shared_ptr<ControlFunction> destination);

		bool send_status_message();

		bool send_supported_objects(std::shared_ptr<ControlFunction> destination) const;

		/// @brief Cyclic update function
		void update();

		EventDispatcher<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, std::uint16_t, std::uint16_t> onChangeActiveMaskEventDispatcher;
		EventDispatcher<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, std::uint16_t, bool> onHideShowObjectEventDispatcher;
		EventDispatcher<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, std::uint16_t, bool> onEnableDisableObjectEventDispatcher;
		EventDispatcher<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, std::uint16_t, std::uint32_t> onChangeNumericValueEventDispatcher;
		EventDispatcher<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, std::uint16_t, std::uint16_t, std::int8_t, std::int8_t> onChangeChildLocationEventDispatcher;
		LanguageCommandInterface languageCommandInterface;
		std::shared_ptr<InternalControlFunction> serverInternalControlFunction;
		std::vector<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>> managedWorkingSetList;
		std::shared_ptr<VirtualTerminalServerManagedWorkingSet> activeWorkingSet;
		std::uint32_t statusMessageTimestamp_ms = 0;
		std::uint16_t activeWorkingSetDataMaskObjectID = NULL_OBJECT_ID;
		std::uint16_t activeWorkingSetSoftkeyMaskObjectID = NULL_OBJECT_ID;
		std::uint8_t activeWorkingSetMasterAddress = NULL_CAN_ADDRESS;
		std::uint8_t busyCodesBitfield = 0;
		std::uint8_t currentCommandFunctionCode = 0;
		bool initialized = false;
	};
} // namespace isobus
#endif //ISOBUS_VIRTUAL_TERMINAL_SERVER_HPP
