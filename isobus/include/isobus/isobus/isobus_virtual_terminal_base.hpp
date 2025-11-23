//================================================================================================
/// @file isobus_virtual_terminal_base.hpp
///
/// @brief A base class that stores definitions common between the VT client and the VT server.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef ISOBUS_VIRTUAL_TERMINAL_BASE_HPP
#define ISOBUS_VIRTUAL_TERMINAL_BASE_HPP

#include <cstdint>

namespace isobus
{
	/// @brief A base class for the VT client and VT server that stores common definitions
	class VirtualTerminalBase
	{
	public:
		/// @brief Enumerates the states that can be sent with a hide/show object command
		enum class HideShowObjectCommand : std::uint8_t
		{
			HideObject = 0, ///< Hides the object
			ShowObject = 1 ///< Shows an object
		};

		/// @brief Enumerates the states that can be sent with an enable/disable object command
		enum class EnableDisableObjectCommand : std::uint8_t
		{
			DisableObject = 0, ///< Disables a compatible object
			EnableObject = 1 ///< Enables a compatible object
		};

		/// @brief Enumerates the states that can be sent with a select input object options command
		enum class SelectInputObjectOptions : std::uint8_t
		{
			ActivateObjectForDataInput = 0x00, ///< Activates an object for data input
			SetFocusToObject = 0xFF ///< Focuses the object (usually this draws a temporary box around it)
		};

		/// @brief The different VT versions that a client or server might support
		enum class VTVersion
		{
			Version2OrOlder, ///< Client or server supports VT version 2 or lower
			Version3, ///< Client or server supports all of VT version 3
			Version4, ///< Client or server supports all of VT version 4
			Version5, ///< Client or server supports all of VT version 5
			Version6, ///< Client or server supports all of VT version 6
			ReservedOrUnknown, ///< Reserved value, not to be used
		};

		/// @brief Enumerates the different line directions that can be used when changing an endpoint of an object
		enum class LineDirection : std::uint8_t
		{
			TopLeftToBottomRightOfEnclosingVirtualRectangle = 0, ///< Draws the line from top left to bottom right of the enclosing virtual rectangle
			BottomLeftToTopRightOfEnclosingVirtualRectangle = 1 ///< Draws the line from bottom left to top right of the enclosing virtual rectangle
		};

		/// @brief Enumerates the different font sizes
		enum class FontSize : std::uint8_t
		{
			Size6x8 = 0, ///< 6x8 Font size
			Size8x8 = 1, ///< 8x8 Font size
			Size8x12 = 2, ///< 8x12 Font size
			Size12x16 = 3, ///< 12x16 Font size
			Size16x16 = 4, ///< 16x16 Font size
			Size16x24 = 5, ///< 16x24 Font size
			Size24x32 = 6, ///< 24x32 Font size
			Size32x32 = 7, ///< 32x32 Font size
			Size32x48 = 8, ///< 32x48 Font size
			Size48x64 = 9, ///< 48x64 Font size
			Size64x64 = 10, ///< 64x64 Font size
			Size64x96 = 11, ///< 64x96 Font size
			Size96x128 = 12, ///< 96x128 Font size
			Size128x128 = 13, ///< 128x128 Font size
			Size128x192 = 14 ///< 128x192 Font size
		};

		/// @brief Enumerates the font style options that can be encoded in a font style bitfield
		enum class FontStyleBits : std::uint8_t
		{
			Bold = 0, ///< Bold font style
			CrossedOut = 1, ///< Crossed-out font style (strikethrough)
			Underlined = 2, ///< Underlined font style
			Italic = 3, ///< Italic font style
			Inverted = 4, ///< Inverted font style (upside down)
			Flashing = 5, ///< Flashing font style
			FlashingHidden = 6, ///< Flashing between hidden and shown font style
			ProportionalFontRendering = 7 ///< Enables proportional font rendering if supported by the server
		};

		/// @brief Enumerates the different font types
		enum class FontType : std::uint8_t
		{
			ISO8859_1 = 0, ///< ISO Latin 1
			ISO8859_15 = 1, ///< ISO Latin 9
			ISO8859_2 = 2, ///< ISO Latin 2
			Reserved_1 = 3, ///< Reserved
			ISO8859_4 = 4, ///< ISO Latin 4
			ISO8859_5 = 5, ///< Cyrillic
			Reserved_2 = 6, ///< Reserved
			ISO8859_7 = 7, ///< Greek
			ReservedEnd = 239, ///< Reserved from ISO8859_7 to this value
			ProprietaryBegin = 240, ///< The beginning of the proprietary range
			ProprietaryEnd = 255 ///< The end of the proprietary region
		};

		/// @brief Enumerates the different fill types for an object
		enum class FillType : std::uint8_t
		{
			NoFill = 0, ///< No fill will be applied
			FillWithLineColour = 1, ///< Fill with the colour of the outline of the shape
			FillWithSpecifiedColourInFillColourAttribute = 2, ///< Fill with the colour specified by a fill attribute
			FillWithPatternGivenByFillPatternAttribute = 3 ///< Fill with a patter provided by a fill pattern attribute
		};

		/// @brief The types of object pool masks
		enum class MaskType : std::uint8_t
		{
			DataMask = 1, ///< A data mask, used in normal circumstances
			AlarmMask = 2 ///< An alarm mask, which has different metadata related to popping up alarms, like priority
		};

		/// @brief The allowable priorities of an alarm mask
		enum class AlarmMaskPriority : std::uint8_t
		{
			High = 0, ///< Overrides lower priority alarm masks
			Medium = 1, ///< Overrides low priority alarm masks
			Low = 2 ///< Overrides data masks
		};

		/// @brief Denotes the lock/unlock state of a mask. Used to freeze/unfreeze rendering of a mask.
		enum class MaskLockState : std::uint8_t
		{
			UnlockMask = 0, ///< Renders the mask normally
			LockMask = 1 ///< Locks the mask so rendering of it is not updated until it is unlocked or a timeout occurs
		};

		/// @brief The different key activation codes that a button press can generate
		enum class KeyActivationCode : std::uint8_t
		{
			ButtonUnlatchedOrReleased = 0, ///< Button is released
			ButtonPressedOrLatched = 1, ///< Button is pressed
			ButtonStillHeld = 2, ///< Button is being held down (sent cyclically)
			ButtonPressAborted = 3 ///< Press was aborted (user navigated away from the button and did not release it)
		};

		/// @brief Enumerates the errors that can be present in an ESC message
		enum class ESCMessageErrorCode : std::uint8_t
		{
			NoError = 0, ///< No error occurred
			NoInputFieldOpen = 1, ///< No input field is open
			OtherError = 5 ///< Error is not one of the above
		};

		/// @brief Enumerates the different events that can be associated with a macro
		enum class MacroEventID : std::uint8_t
		{
			Reserved = 0, ///< Reserved
			OnActivate = 1, ///< Event on activation of an object (such as for data input)
			OnDeactivate = 2, ///< Event on deactivation of an object
			OnShow = 3, ///< Event on an object being shown
			OnHide = 4, ///< Event on an object being hidden
			OnEnable = 5, ///< Event on enable of an object
			OnDisable = 6, ///< Event on disabling an object
			OnChangeActiveMask = 7, ///< Event on changing the active mask
			OnChangeSoftKeyMask = 8, ///< Event on change of the soft key mask
			OnChangeAttribute = 9, ///< Event on change of an attribute value
			OnChangeBackgroundColour = 10, ///< Event on change of a background colour
			OnChangeFontAttributes = 11, ///< Event on change of a font attribute
			OnChangeLineAttributes = 12, ///< Event on change of a line attribute
			OnChangeFillAttributes = 13, ///< Event on change of a fill attribute
			OnChangeChildLocation = 14, ///< Event on change of a child objects location
			OnChangeSize = 15, ///< Event on change of an object size
			OnChangeValue = 16, ///< Event on change of an object value (like via `change numeric value`)
			OnChangePriority = 17, ///< Event on change of a mask's priority
			OnChangeEndPoint = 18, ///< Event on change of an object endpoint
			OnInputFieldSelection = 19, ///< Event when an input field is selected
			OnInputFieldDeselection = 20, ///< Event on deselection of an input field
			OnESC = 21, ///< Event on ESC (escape)
			OnEntryOfValue = 22, ///< Event on entry of a value
			OnEntryOfNewValue = 23, ///< Event on entry of a *new* value
			OnKeyPress = 24, ///< Event on the press of a key
			OnKeyRelease = 25, ///< Event on the release of a key
			OnChangeChildPosition = 26, ///< Event on changing a child object's position
			OnPointingEventPress = 27, ///< Event on a pointing event press
			OnPointingEventRelease = 28, ///< Event on a pointing event release
			ReservedBegin = 29, ///< Beginning of the reserved range
			ReservedEnd = 254, ///< End of the reserved range
			UseExtendedMacroReference = 255 ///< Use extended macro reference
		};

		/// @brief Enumerates the various VT server graphics modes
		enum class GraphicMode : std::uint8_t
		{
			Monochrome = 0, ///< Monochromatic graphics mode (1 bit)
			SixteenColour = 1, ///< 16 Colour mode (4 bit)
			TwoHundredFiftySixColour = 2 ///< 256 Colour mode (8 bit)
		};

		/// @brief Enumerates the various auxiliary input function types
		enum class AuxiliaryTypeTwoFunctionType : std::uint8_t
		{
			BooleanLatching = 0, ///< Two-position switch (maintains position) (Single Pole, Double Throw)
			AnalogueLatching = 1, ///< Two-way analogue (Maintains position setting)
			BooleanMomentary = 2, ///< Two-position switch (returns to off) (Momentary Single Pole, Single Throw)
			AnalogueMomentaryTwoWay = 3, ///< Two-way analogue (returns to centre position - 50%)
			AnalogueMomentaryOneWay = 4, ///< One-way analogue (returns to 0%)
			DualBooleanLatching = 5, ///< Three-position switch (maintains position) (Single Pole, Three Positions, Centre Off)
			DualBooleanMomentary = 6, ///< Three-position switch (returns to off/centre position) (Momentary Single Pole, Three Positions, Centre Off)
			DualBooleanLatchingUpOnly = 7, ///< Three-position switch (maintains position only in up position) (Single Pole, Three Positions, Centre Off)
			DualBooleanLatchingDownpOnly = 8, ///< Three-position switch (maintains position only in down position) (Momentary Single Pole, Three Positions, Centre Off)
			AnalogueMomentaryBooleanLatching = 9, ///< two-way analogue (returns to centre position) with latching Boolean at 0% and 100% positions
			AnalogueLatchingBooleanLatching = 10, ///< two-way analogue (maintains position setting) with momentary Boolean at 0% and 100% positions
			QuadratureBooleanMomentary = 11, ///< Two Quadrature mounted Three-position switches (returns to centre position) (Momentary Single Pole, Three Position Single Throw, Centre Off)
			QuadratureAnalogueLatching = 12, ///< Two Quadrature mounted Two-way analogue (maintains position)
			QuadratureAnalogueMomentary = 13, ///< Two Quadrature mounted Two-way analogue (returns to centre position - 50%)
			BidirectionalEncoder = 14, ///< Count increases when turning in the encoders "increase" direction, and decreases when turning in the opposite direction
			Reserved = 30, ///< 15-30 Reserved
			ReservedRemoveAssignment = 31 ///< Used for Remove assignment command
		};

		/// @brief The internal state machine state of the VT client, mostly just public so tests can access it
		enum class StateMachineState : std::uint8_t
		{
			Disconnected, ///< VT is not connected, and is not trying to connect yet
			WaitForPartnerVTStatusMessage, ///< VT client is initialized, waiting for a VT server to come online
			SendWorkingSetMasterMessage, ///< Client is sending the working state master message
			ReadyForObjectPool, ///< Client needs an object pool before connection can continue
			SendGetMemory, ///< Client is sending the "get memory" message to see if VT has enough memory available
			WaitForGetMemoryResponse, ///< Client is waiting for a response to the "get memory" message
			SendGetNumberSoftkeys, ///< Client is sending the "get number of soft keys" message
			WaitForGetNumberSoftKeysResponse, ///< Client is waiting for a response to the "get number of soft keys" message
			SendGetTextFontData, ///< Client is sending the "get text font data" message
			WaitForGetTextFontDataResponse, ///< Client is waiting for a response to the "get text font data" message
			SendGetHardware, ///< Client is sending the "get hardware" message
			WaitForGetHardwareResponse, ///< Client is waiting for a response to the "get hardware" message
			SendGetVersions, ///< If a version label was specified, check to see if the VT has that version already
			WaitForGetVersionsResponse, ///< Client is waiting for a response to the "get versions" message
			SendStoreVersion, ///< Sending the store version command
			WaitForStoreVersionResponse, ///< Client is waiting for a response to the store version command
			SendLoadVersion, ///< Sending the load version command
			WaitForLoadVersionResponse, ///< Client is waiting for the VT to respond to the "Load Version" command
			UploadObjectPool, ///< Client is uploading the object pool
			SendEndOfObjectPool, ///< Client is sending the end of object pool message
			WaitForEndOfObjectPoolResponse, ///< Client is waiting for the end of object pool response message
			Connected, ///< Client is connected to the VT server and the application layer is in control
			Failed ///< Client could not connect to the VT due to an error
		};

		/// @brief Enumerates the errors that can occur when requesting the supported wide chars from a VT
		enum class SupportedWideCharsErrorCode
		{
			TooManyRanges = 0x01, ///< Too many ranges (more than 255 sub-ranges in the requested range)
			ErrorInCodePlane = 0x02,
			AnyOtherError = 0x08
		};

		/// @brief A struct for storing information of a function assigned to an auxiliary input
		class AssignedAuxiliaryFunction
		{
		public:
			/// @brief Constructs a `AssignedAuxiliaryFunction`, sets default values
			/// @param[in] functionObjectID the object ID of the function present in our object pool
			/// @param[in] inputObjectID the object ID assigned on the auxiliary inputs end
			/// @param[in] functionType the type of function
			AssignedAuxiliaryFunction(std::uint16_t functionObjectID, std::uint16_t inputObjectID, AuxiliaryTypeTwoFunctionType functionType);

			/// @brief Allows easy comparison of two `AssignedAuxiliaryFunction` objects
			/// @param[in] other the object to compare against
			/// @returns true if the two objects are equal, otherwise false
			bool operator==(const AssignedAuxiliaryFunction &other) const;

			std::uint16_t functionObjectID; ///< The object ID of the function present in our object pool
			std::uint16_t inputObjectID; ///< The object ID assigned on the auxiliary inputs end
			AuxiliaryTypeTwoFunctionType functionType; ///< The type of function
		};

	protected:
		/// @brief Enumerates the multiplexor byte values for VT commands
		enum class Function : std::uint8_t
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
			ScreenCapture = 0xC6,
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

		/// @brief Enumerates the command types for graphics context objects
		enum class GraphicsContextSubCommandID : std::uint8_t
		{
			SetGraphicsCursor = 0x00, ///< Sets the graphics cursor x/y attributes
			MoveGraphicsCursor = 0x01, ///< Moves the cursor relative to current location
			SetForegroundColour = 0x02, ///< Sets the foreground colour
			SetBackgroundColour = 0x03, ///< Sets the background colour
			SetLineAttributesObjectID = 0x04, ///< Sets the line attribute object ID
			SetFillAttributesObjectID = 0x05, ///< Sets the fill attribute object ID
			SetFontAttributesObjectID = 0x06, ///< Sets the font attribute object ID
			EraseRectangle = 0x07, ///< Erases a rectangle
			DrawPoint = 0x08, ///< Draws a point
			DrawLine = 0x09, ///< Draws a line
			DrawRectangle = 0x0A, ///< Draws a rectangle
			DrawClosedEllipse = 0x0B, ///< Draws a closed ellipse
			DrawPolygon = 0x0C, ///< Draws polygon
			DrawText = 0x0D, ///< Draws text
			PanViewport = 0x0E, ///< Pans viewport
			ZoomViewport = 0x0F, ///< Zooms the viewport
			PanAndZoomViewport = 0x10, ///< Pan and zooms the viewport
			ChangeViewportSize = 0x11, ///< Changes the viewport size
			DrawVTObject = 0x12, ///< Draws a VT object
			CopyCanvasToPictureGraphic = 0x13, ///< Copies the canvas to picture graphic object
			CopyViewportToPictureGraphic = 0x14 ///< Copies the viewport to picture graphic object
		};
	};
} // namespace isobus
#endif // ISOBUS_VIRTUAL_TERMINAL_BASE_HPP
