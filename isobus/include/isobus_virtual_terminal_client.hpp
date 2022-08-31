//================================================================================================
/// @file isobus_virtual_terminal_client.hpp
///
/// @brief A class to manage a client connection to a ISOBUS virtual terminal display
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#ifndef ISOBUS_VIRTUAL_TERMINAL_CLIENT_HPP
#define ISOBUS_VIRTUAL_TERMINAL_CLIENT_HPP

#include "can_partnered_control_function.hpp"
#include "can_internal_control_function.hpp"

#include <memory>
#include <vector>

namespace isobus
{

	class VirtualTerminalClient
	{
	public:
		enum class HideShowObjectCommand : std::uint8_t
		{
			HideObject = 0,
			ShowObject = 1
		};

		enum class EnableDisableObjectCommand : std::uint8_t
		{
			DisableObject = 0,
			EnableObject = 1
		};

		enum class SelectInputObjectOptions : std::uint8_t
		{
			ActivateObjectForDataInput = 0x00,
			SetFocusToObject = 0xFF
		};

		enum class VTVersion
		{
			Version2OrOlder,
			Version3,
			Version4,
			Version5,
			Version6,
			ReservedOrUnknown,
		};

		enum class LineDirection
		{
			TopLeftToBottomRightOfEnclosingVirtualRectangle = 0,
			BottomLeftToTopRightOfEnclosingVirtualRectangle = 1
		};

		enum class FontSize
		{
			Size6x8 = 0,
			Size8x8 = 1,
			Size8x12 = 2,
			Size12x16 = 3,
			Size16x16 = 4,
			Size16x24 = 5,
			Size24x24 = 6,
			Size24x32 = 7,
			Size32x32 = 8,
			Size48x64 = 9,
			Size64x64 = 10,
			Size64x96 = 11,
			Size96x128 = 12,
			Size128x128 = 13,
			Size128x192 = 14
		};

		enum class FontStyleBits
		{
			Bold = 0,
			CrossedOut = 1,
			Underlined = 2,
			Italic = 3,
			Inverted = 4,
			Flashing = 5,
			FlashingHidden = 6,
			ProportionalFontRendering = 7
		};

		enum class FontType
		{
			ISO8859_1 = 0, // ISO Latin 1
			ISO8859_15 = 1, // ISO Latin 9
			ISO8859_2 = 2, // ISO Latin 2
			Reserved_1 = 3,
			ISO8859_4 = 4, // ISO Latin 4
			ISO8859_5 = 5, // Cyrillic
			Reserved_2 = 6,
			ISO8859_7 = 7, // Greek
			ReservedEnd = 239,
			ProprietaryBegin = 240,
			ProprietaryEnd = 255
		};

		enum class FillType
		{
			NoFill = 0,
			FillWithLineColor = 1,
			FillWithSpecifiedColorInFillColorAttribute = 2,
			FillWithPatternGivenByFillPatternAttribute = 3
		};

		enum class MaskType
		{
			DataMask = 1,
			AlarmMask = 2
		};

		enum class AlarmMaskPriority
		{
			High = 0,
			Medium = 1,
			Low = 2
		};

		enum class MaskLockState
		{
			UnlockMask = 0,
			LockMask = 1
		};

		enum class KeyActivationCode
		{
			ButtonUnlatchedOrReleased = 0,
			ButtonPressedOrLatched = 1,
			ButtonStillHeld = 2,
			ButtonPressAborted = 3
		};

		enum class StateMachineState
		{
			Disconnected,
			WaitForPartnerVTStatusMessage,
			ReadyForObjectPool,
			SendGetMemory,
			WaitForGetMemoryResponse,
			SendGetNumberSoftkeys,
			WaitForGetNumberSoftKeysResponse,
			SendGetTextFontData,
			WaitForGetTextFontDataResponse,
			SendGetHardware,
			WaitForGetHardwareResponse,
			UploadObjectPool,
			SendEndOfObjectPool,
			WaitForEndOfObjectPoolResponse,
			Connected
		};

		static const std::uint16_t NULL_OBJECT_ID = 0xFFFF;

		explicit VirtualTerminalClient(std::shared_ptr<PartneredControlFunction> partner, std::shared_ptr<InternalControlFunction> clientSource);

		// Basic Interaction
		void RegisterVTEventCallback();

		// Command Messages
		bool send_hide_show_object(std::uint16_t objectID, HideShowObjectCommand command);
		bool send_enable_disable_object(std::uint16_t objectID, EnableDisableObjectCommand command);
		bool send_select_input_object(std::uint16_t objectID, SelectInputObjectOptions option);
		bool send_ESC();
		bool send_control_audio_signal(std::uint8_t activations, std::uint16_t frequency_hz, std::uint16_t duration_ms, std::uint16_t offTimeDuration_ms);
		bool send_set_audio_volume(std::uint8_t volume_percent);
		bool send_change_child_location(std::uint16_t objectID, std::uint16_t parentObjectID, std::uint8_t relativeXPositionChange, std::uint8_t relativeYPositionChange);
		bool send_change_child_position(std::uint16_t objectID, std::uint16_t parentObjectID, std::uint16_t xPosition, std::uint16_t yPosition);
		bool send_change_size_command(std::uint16_t objectID, std::uint16_t newWidth, std::uint16_t newHeight);
		bool send_change_background_colour(std::uint16_t objectID, std::uint8_t color);
		bool send_change_numeric_value(std::uint16_t objectID, std::uint32_t value);
		bool send_change_string_value(std::uint16_t objectID, uint16_t stringLength, const char *value);
		bool send_change_string_value(std::uint16_t objectID, const std::string &value);
		bool send_change_endpoint(std::uint16_t objectID, std::uint16_t width_px, std::uint16_t height_px, LineDirection direction);
		bool send_change_font_attributes(std::uint16_t objectID, std::uint8_t color, FontSize size, std::uint8_t type, std::uint8_t styleBitfield);
		bool send_change_line_attributes(std::uint16_t objectID, std::uint8_t color, std::uint8_t width, std::uint16_t lineArtBitmask);
		bool send_change_fill_attributes(std::uint16_t objectID, FillType fillType, std::uint8_t color, std::uint16_t fillPatternObjectID);
		bool send_change_active_mask(std::uint16_t workingSetObjectID, std::uint16_t newActiveMaskObjectID);
		bool send_change_softkey_mask(MaskType type, std::uint16_t dataOrAlarmMaskObjectID, std::uint16_t newSoftKeyMaskObjectID);
		bool send_change_attribute(std::uint16_t objectID, std::uint8_t attributeID, std::uint32_t value);
		bool send_change_priority(std::uint16_t alarmMaskObjectID, AlarmMaskPriority priority);
		bool send_change_list_item(std::uint16_t objectID, std::uint8_t listIndex, std::uint16_t newObjectID);
		bool send_lock_unlock_mask(MaskLockState state, std::uint16_t objectID, std::uint16_t timeout_ms);
		bool send_execute_macro(std::uint16_t objectID);
		bool send_change_object_label(std::uint16_t objectID, std::uint16_t labelStringObjectID, std::uint8_t fontType, std::uint16_t graphicalDesignatorObjectID);
		bool send_change_polygon_point(std::uint16_t objectID, std::uint8_t pointIndex, std::uint16_t newXValue, std::uint16_t newYValue);
		bool send_change_polygon_scale(std::uint16_t objectID, std::uint16_t widthAttribute, std::uint16_t heightAttribute);
		bool send_select_color_map_or_palette(std::uint16_t objectID);
		bool send_execute_extended_macro(std::uint16_t objectID);
		bool send_select_active_working_set(std::uint64_t NAMEofWorkingSetMasterForDesiredWorkingSet);

		// Graphics Context Commands
		bool send_set_graphics_cursor(std::int16_t xPosition, std::int16_t yPosition);
		bool send_move_graphics_cursor(std::int16_t xOffset, std::int16_t yOffset);
		bool send_set_foreground_colour(std::uint8_t color);
		bool send_set_background_colour(std::uint8_t color);
		bool send_set_line_attributes_object_id(std::uint16_t objectID);
		bool send_set_fill_attributes_object_id(std::uint16_t objectID);
		bool send_set_font_attributes_object_id(std::uint16_t objectID);
		bool send_erase_rectangle(std::uint16_t width, std::uint16_t height);
		bool send_draw_point(std::int16_t xOffset, std::int16_t yOffset);
		bool send_draw_line(std::int16_t xOffset, std::int16_t yOffset);
		bool send_draw_rectangle(std::uint16_t width, std::uint16_t height);
		bool send_draw_closed_ellipse(std::uint16_t width, std::uint16_t height);
		bool send_draw_polygon(std::uint8_t numberOfPoints, std::int16_t *listOfXOffsetsRelativeToCursor, std::int16_t *listOfYOffsetsRelativeToCursor);
		bool send_draw_text(bool transparent, std::uint8_t textLength, const char *value);
		bool send_pan_viewport(std::int16_t xAttribute, std::int16_t yAttribute);
		bool send_zoom_viewport(float zoom);
		bool send_pan_and_zoom_viewport(std::int16_t xAttribute, std::int16_t yAttribute, float zoom);
		bool send_change_viewport_size(std::uint16_t width, std::uint16_t height);
		bool send_draw_vt_object(std::uint16_t objectID);
		bool send_copy_canvas_to_picture_graphic(std::uint16_t objectID);
		bool send_copy_viewport_to_picture_graphic(std::uint16_t objectID);

		// VT Querying
		bool send_get_attribute_value(std::uint16_t objectID, std::uint8_t attributeID);

		VTVersion get_connected_vt_version() const;

		// Object Pool

		// These are the functions for specifying your pool to upload.
		// You have a few options:
		// 1. Upload in one blob of contigious memory 
		// This is good for small pools or pools where you have all the data in memory.
		// 2. Get a callback at some inteval to provide data in chunks
		// This is probably better for huge pools if you are RAM constrained, or if your
		// pool is stored on some external device that you need to get data from in pages.
		// If using callbacks, The object pool and pointer MUST NOT be deleted or leave scope until upload is done.
		void set_object_pool(const std::uint8_t *pool, std::uint32_t size);
		void set_object_pool(const std::vector<std::uint8_t> *pool);
		void register_object_pool_data_chunk_callback(DataChunkCallback value);

		// Periodic Update Function (thread should call this)
		void update();

	private:
		enum class Function
		{
			SoftKeyActivationMessage = 0x00,
			ButtonActivationMessage = 0x01,
			PointingEventMessage = 0x02,
			VTSelectInputObjectMessage = 0x03,
			VTESCMessage = 0x04,
			VTChangeNumericValueMessage = 0x05,
			VTChangeActiveMaskMessage = 0x06,
			VTChangeSoftKeyMaskMessage = 0x07,
			VTChangeStringValueMessage = 0x08,
			VTOnUserLayoutHideShowMessage = 0x09,
			VTControlAudioSignalTerminationMessage = 0x0A,
			ObjectPoolTransferMessage = 0x11,
			EndOfObjectPoolMessage = 0x12,
			AuxiliaryAssignmentTypeOneCommand = 0x20,
			AuxiliaryInputTypeOneStatus = 0x21,
			PreferredAssignmentCommand = 0x22,
			AuxiliaryInputTypeTwoMaintenanceMessage = 0x23,
			AuxiliaryAssignmentTypeTwoCommand = 0x24,
			AuxiliaryInputStatusTypeTwoEnableCommand = 0x25,
			AuxiliaryInputTypeTwoStatusMessage = 0x26,
			AuxiliaryCapabilitiesRequest = 0x27,
			SelectActiveWorkingSet = 0x90,
			ESCCommand = 0x92,
			HideShowObjectCommand = 0xA0,
			EnableDisableObjectCommand = 0xA1,
			SelectInputObjectCommand = 0xA2,
			ControlAudioSignalCommand = 0xA3,
			SetAudioVolumeCommand = 0xA4,
			ChangeChildLocationCommand = 0xA5,
			ChangeSizeCommand = 0xA6,
			ChangeBackgroundColourCommand = 0xA7,
			ChangeNumericValueCommand = 0xA8,
			ChangeEndPointCommand = 0xA9,
			ChangeFontAttributesCommand = 0xAA,
			ChangeLineAttributesCommand = 0xAB,
			ChangeFillAttributesCommand = 0xAC,
			ChangeActiveMaskCommand = 0xAD,
			ChangeSoftKeyMaskCommand = 0xAE,
			ChangeAttributeCommand = 0xAF,
			ChangePriorityCommand = 0xB0,
			ChangeListItemCommand = 0xB1,
			DeleteObjectPoolCommand = 0xB2,
			ChangeStringValueCommand = 0xB3,
			ChangeChildPositionCommand = 0xB4,
			ChangeObjectLabelCommand = 0xB5,
			ChangePolygonPointCommand = 0xB6,
			ChangePolygonScaleCommand = 0xB7,
			GraphicsContextCommand = 0xB8,
			GetAttributeValueMessage = 0xB9,
			SelectColourMapCommand = 0xBA,
			IdentifyVTMessage = 0xBB,
			ExecuteExtendedMacroCommand = 0xBC,
			LockUnlockMaskCommand = 0xBD,
			ExecuteMacroCommand = 0xBE,
			GetMemoryMessage = 0xC0,
			GetSupportedWidecharsMessage = 0xC1,
			GetNumberOfSoftKeysMessage = 0xC2,
			GetTextFontDataMessage = 0xC3,
			GetWindowMaskDataMessage = 0xC4,
			GetSupportedObjectsMessage = 0xC5,
			GetHardwareMessage = 0xC7,
			StoreVersionCommand = 0xD0,
			LoadVersionCommand = 0xD1,
			DeleteVersionCommand = 0xD2,
			ExtendedGetVersionsMessage = 0xD3,
			ExtendedStoreVersionCommand = 0xD4,
			ExtendedLoadVersionCommand = 0xD5,
			ExtendedDeleteVersionCommand = 0xD6,
			GetVersionsMessage = 0xDF,
			GetVersionsResponse = 0xE0,
			UnsupportedVTFunctionMessage = 0xFD,
			VTStatusMessage = 0xFE,
			WorkingSetMaintenanceMessage = 0xFF
		};

		enum class EventID : std::uint8_t
		{
			Reserved = 0,
			OnActivate = 1,
			OnDeactivate = 2,
			OnShow = 3,
			OnHide = 4,
			OnEnable = 5,
			OnDisable = 6,
			OnChangeActiveMask = 7,
			OnChangeSoftKeyMask = 8,
			OnChangeAttribute = 9,
			OnChangeBackgroundColor = 10,
			OnChangeFontAttributes = 11,
			OnChangeLineAttributes = 12,
			OnChangeFillAttributes = 13,
			OnChangeChildLocation = 14,
			OnChangeSize = 15,
			OnChangeValue = 16,
			OnChangePriority = 17,
			OnChangeEndPoint = 18,
			OnInputFieldSelection = 19, 
			OnInputFieldDeselection = 20,
			OnESC = 21,
			OnEntryOfValue = 22, 
			OnEntryOfNewValue = 23,
			OnKeyPress = 24,
			OnKeyRelease = 25,
			OnChangeChildPosition = 26, 
			OnPointingEventPress = 27,
			OnPointingEventRelease = 28,
			ReservedBegin = 29,
			ReservedEnd = 254,
			UseExtendedMacroReference = 255
		};

		// Object Pool Managment
		bool send_delete_object_pool();
		bool send_working_set_maintenance(bool initializing, VTVersion workingSetVersion);
		bool send_get_memory(std::uint32_t requiredMemory);
		bool send_get_number_of_softkeys();
		bool send_get_text_font_data();
		bool send_get_hardware();
		bool send_get_supported_widechars();
		bool send_get_window_mask_data();
		bool send_get_supported_objects();
		bool send_get_versions();
		bool send_store_version(std::array<std::uint8_t, 7> versionLabel);
		bool send_load_version(std::array<std::uint8_t, 7> versionLabel);
		bool send_delete_version(std::array<std::uint8_t, 7> versionLabel);
		bool send_extended_get_versions();
		bool send_extended_store_version(std::array<std::uint8_t, 32> versionLabel);
		bool send_extended_load_version(std::array<std::uint8_t, 32> versionLabel);
		bool send_extended_delete_version(std::array<std::uint8_t, 32> versionLabel);

		void set_state(StateMachineState value);

		static const std::uint32_t VT_STATUS_TIMEOUT_MS = 750;

		std::shared_ptr<PartneredControlFunction> partnerControlFunction;
		std::shared_ptr<InternalControlFunction> myControlFunction;

		// Status message contents from the VT
		std::uint32_t lastVTStatusTimestamp_ms;
		std::uint16_t activeWorkingSetDataMaskObjectID;
		std::uint16_t activeWorkingSetSoftkeyMaskObjectID;
		std::uint8_t activeWorkingSetMasterAddress;
		std::uint8_t busyCodesBitfield;
		std::uint8_t currentCommandFunctionCode;

		std::uint8_t connectedVTVersion;
		// Internal state
		StateMachineState state;
		std::uint32_t stateMachineTimestamp_ms;

		// Object Pool info
		DataChunkCallback objectPoolDataCallback;
		std::uint32_t objectPoolSize_bytes;
	};

} // namespace isobus

#endif // ISOBUS_VIRTUAL_TERMINAL_CLIENT_HPP