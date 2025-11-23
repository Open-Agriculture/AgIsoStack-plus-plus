//================================================================================================
/// @file isobus_virtual_terminal_objects.hpp
///
/// @brief Defines the different VT object types that can comprise a VT object pool.
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#ifndef ISOBUS_VIRTUAL_TERMINAL_OBJECTS_HPP
#define ISOBUS_VIRTUAL_TERMINAL_OBJECTS_HPP

#include "isobus/isobus/can_constants.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace isobus
{
	class VirtualTerminalServerManagedWorkingSet;

	/// @brief The types of objects in an object pool by object type byte value
	enum class VirtualTerminalObjectType : std::uint8_t
	{
		WorkingSet = 0, ///< Top level object that describes an implement’s ECU or group of ECUs
		DataMask = 1, ///< Top level object that contains other objects. A Data Mask is activated by a Working Set to become the active set of objects on the VT display.
		AlarmMask = 2, ///< Top level object that contains other objects. Describes an alarm display.
		Container = 3, ///< Used to group objects.
		WindowMask = 34, ///< Top level object that contains other objects. The Window Mask is activated by the VT.
		SoftKeyMask = 4, ///< Top level object that contains Key objects.
		Key = 5, ///< Used to describe a Soft Key.
		Button = 6, ///< Used to describe a Button control.
		KeyGroup = 35, ///< Top level object that contains Key objects.
		InputBoolean = 7, ///< Used to input a TRUE/FALSE type input.
		InputString = 8, ///< Used to input a character string
		InputNumber = 9, ///< Used to input an integer or float numeric.
		InputList = 10, ///< Used to select an item from a pre-defined list.
		OutputString = 11, ///< Used to output a character string.
		OutputNumber = 12, ///< Used to output an integer or float numeric.
		OutputList = 37, ///< Used to output a list item.
		OutputLine = 13, ///< Used to output a line.
		OutputRectangle = 14, ///< Used to output a rectangle or square.
		OutputEllipse = 15, ///< Used to output an ellipse or circle.
		OutputPolygon = 16, ///< Used to output a polygon.
		OutputMeter = 17, ///< Used to output a meter.
		OutputLinearBarGraph = 18, ///< Used to output a linear bar graph.
		OutputArchedBarGraph = 19, ///< Used to output an arched bar graph.
		GraphicsContext = 36, ///< Used to output a graphics context.
		Animation = 44, ///< The Animation object is used to display simple animations
		PictureGraphic = 20, ///< Used to output a picture graphic (bitmap).
		GraphicData = 46, ///< Used to define the data for a graphic image
		ScaledGraphic = 48, ///< Used to display a scaled representation of a graphic object
		NumberVariable = 21, ///< Used to store a 32-bit unsigned integer value.
		StringVariable = 22, ///< Used to store a fixed length string value.
		FontAttributes = 23, ///< Used to group font based attributes. Can only be referenced by other objects.
		LineAttributes = 24, ///< Used to group line based attributes. Can only be referenced by other objects.
		FillAttributes = 25, ///< Used to group fill based attributes. Can only be referenced by other objects
		InputAttributes = 26, ///< Used to specify a list of valid characters. Can only be referenced by input field objects.
		ExtendedInputAttributes = 38, ///< Used to specify a list of valid WideChars. Can only be referenced by Input Field Objects.
		ColourMap = 39, ///< Used to specify a colour table object.
		ObjectLabelRefrenceList = 40, ///< Used to specify an object label.
		ObjectPointer = 27, ///< Used to reference another object.
		ExternalObjectDefinition = 41, ///< Used to list the objects that may be referenced from another Working Set
		ExternalReferenceNAME = 42, ///< Used to identify the WS Master of a Working Set that can be referenced
		ExternalObjectPointer = 43, ///< Used to reference an object in another Working Set
		Macro = 28, ///< Special object that contains a list of commands that can be executed in response to an event.
		AuxiliaryFunctionType1 = 29, ///< The Auxiliary Function Type 1 object defines the designator and function type for an Auxiliary Function.
		AuxiliaryInputType1 = 30, ///< The Auxiliary Input Type 1 object defines the designator, key number, and function type for an auxiliary input.
		AuxiliaryFunctionType2 = 31, ///< The Auxiliary Function Type 2 object defines the designator and function type for an Auxiliary Function.
		AuxiliaryInputType2 = 32, ///< The Auxiliary Input Type 2 object defines the designator, key number, and function type for an Auxiliary Input.
		AuxiliaryControlDesignatorType2 = 33, ///< Used to reference Auxiliary Input Type 2 object or Auxiliary Function Type 2 object.
		ManufacturerDefined1 = 240, ///< Manufacturer defined objects should not be sent to any other Vendors VT
		ManufacturerDefined2 = 241, ///< Manufacturer defined objects should not be sent to any other Vendors VT
		ManufacturerDefined3 = 242, ///< Manufacturer defined objects should not be sent to any other Vendors VT
		ManufacturerDefined4 = 243, ///< Manufacturer defined objects should not be sent to any other Vendors VT
		ManufacturerDefined5 = 244, ///< Manufacturer defined objects should not be sent to any other Vendors VT
		ManufacturerDefined6 = 245, ///< Manufacturer defined objects should not be sent to any other Vendors VT
		ManufacturerDefined7 = 246, ///< Manufacturer defined objects should not be sent to any other Vendors VT
		ManufacturerDefined8 = 247, ///< Manufacturer defined objects should not be sent to any other Vendors VT
		ManufacturerDefined9 = 248, ///< Manufacturer defined objects should not be sent to any other Vendors VT
		ManufacturerDefined10 = 249, ///< Manufacturer defined objects should not be sent to any other Vendors VT
		ManufacturerDefined11 = 250, ///< Manufacturer defined objects should not be sent to any other Vendors VT
		ManufacturerDefined12 = 251, ///< Manufacturer defined objects should not be sent to any other Vendors VT
		ManufacturerDefined13 = 252, ///< Manufacturer defined objects should not be sent to any other Vendors VT
		ManufacturerDefined14 = 253, ///< Manufacturer defined objects should not be sent to any other Vendors VT
		ManufacturerDefined15 = 254, ///< Manufacturer defined objects should not be sent to any other Vendors VT
		Reserved = 255 ///< Reserved for future use. (See Clause D.14 Get Supported Objects message)
	};

	/// @brief Enumerates VT events. Events can be uniquely associated with a Macro object to execute when the event occurs.
	/// These are defined in ISO 11783-6:2018 Table A.2
	enum class EventID : std::uint8_t
	{
		Reserved = 0, ///< Reserved
		OnActivate = 1, ///< Working set is made active
		OnDeactivate = 2, ///< Working set is made inactive
		OnShow = 3, ///< For Container objects, triggered by the hide/show command, with "show" indicated; For mask objects, when the mask is made visible on the display.
		OnHide = 4, ///< For Container objects, triggered by the hide/show command, with "hide" indicated; for mask objects, when the mask is removed from the display.
		// OnRefresh - An object that is already on display is redrawn (Macros cannot be associated with this event so no event ID is defined).
		OnEnable = 5, ///< Input object is enabled (only enabled input objects can be navigated to). An Animation object is enabled for animation
		OnDisable = 6, ///< Input object is disabled (only enabled input objects can be navigated to). An Animation object is disabled for animation.
		OnChangeActiveMask = 7, ///< Change Active mask command
		OnChangeSoftKeyMask = 8, ///< Change Soft Key mask command
		OnChangeAttribute = 9, ///< Change Attribute command
		OnChangeBackgroundColour = 10, ///< Change Background Colour command
		ChangeFontAttributes = 11, ///< Change Font Attributes command
		ChangeLineAttributes = 12, ///< Change Line Attributes command
		ChangeFillAttributes = 13, ///< Change Fill Attributes command
		ChangeChildLocation = 14, ///< Change Child Location command
		OnChangeSize = 15, ///< Change Size command
		OnChangeValue = 16, ///< Change numeric value or change string value command
		OnChangePriority = 17, ///< Change Priority command
		OnChangeEndpoint = 18, ///< Change Endpoint command
		OnInputFieldSelection = 19, ///< The input field, Key or Button has received focus, operator has navigated onto the input field or Button or the VT has received the Select Input Object command.
		OnInputFieldDeselection = 20, ///< The input field, Key or Button has lost focus, operator has navigated off of the input field or Button or the VT has received the Select Input Object command
		OnESC = 21, ///< Input aborted on an input field either by the operator or the Working Set.
		OnEntryOfAValue = 22, ///< Operator completes entry by activating the ENTER means - value does not have to change
		OnEntryOfANewValue = 23, ///< Operator completes entry by activating the ENTER means - value has changed
		OnKeyPress = 24, ///< A Soft Key or Button is pressed
		OnKeyRelease = 25, ///< A Soft Key or Button is released
		OnChangeChildPosition = 26, ///< Change Child Position command
		OnPointingEventPress = 27, ///< Operator touches/clicks an area that causes a pointing event
		OnPointingEventRelease = 28, ///< Operator touch/click is released
		ProprietaryRangeBegin = 240, ///< Proprietary range begin
		ProprietaryRangeEnd = 254, ///< Proprietary range end
		UseExtendedMacroReference = 255 ///< This is not an event. When value is found in the event list of an object, it indicates that a 16 bit Macro Object ID reference is used
	};

	/// @brief A helper structure to group a macro ID with an event ID
	struct MacroMetadata
	{
		EventID event; ///< The event that triggers this macro
		std::uint16_t macroID; ///< The ID of the macro to execute
	};

	/// @brief VT 3 component colour vector
	class VTColourVector
	{
	public:
		float r; ///< Red value for a pixel, range 0.0f to 1.0f
		float g; ///< Green value for a pixel, range 0.0f to 1.0f
		float b; ///< Blue value for a pixel, range 0.0f to 1.0f

		/// @brief Default constructor for a VT Colour, which produces the colour black
		constexpr VTColourVector() :
		  r(0.0f), g(0.0f), b(0.0f) {}

		/// @brief Constructor for a VT Colour which initializes to an arbitrary colour
		/// @param[in] red The red value for a pixel, range 0.0f to 1.0f
		/// @param[in] green The green value for a pixel, range 0.0f to 1.0f
		/// @param[in] blue The blue value for a pixel, range 0.0f to 1.0f
		constexpr VTColourVector(float red, float green, float blue) :
		  r(red), g(green), b(blue) {}
	};

	/// @brief An object that represents the VT's active colour table
	class VTColourTable
	{
	public:
		/// @brief Constructor for a VT colour table
		VTColourTable();

		/// @brief Returns the colour vector associated to the specified VT colour index, which
		/// is what gets provided normally in most VT CAN messages, so this essentially maps the index
		/// to an actually usable colour definition.
		/// @param[in] colourIndex The VT colour index to retrieve
		/// @returns An RGB colour vector associated to the specified VT colour index
		VTColourVector get_colour(std::uint8_t colourIndex) const;

		/// @brief Sets the specified VT colour index to a new RGB colour value
		/// @param[in] colourIndex The VT colour index to modify
		/// @param[in] newColour The RGB colour to set the specified index to
		void set_colour(std::uint8_t colourIndex, VTColourVector newColour);

	private:
		static constexpr std::size_t VT_COLOUR_TABLE_SIZE = 256; ///< The size of the VT colour table as specified in ISO11783-6

		std::array<VTColourVector, VT_COLOUR_TABLE_SIZE> colourTable; ///< Colour table data. Associates VT colour index with RGB value.
	};

	/// @brief Generic VT object base class
	class VTObject
	{
	public:
		/// @brief Enumerates the bit indices of the error fields that can be set when changing an attribute
		enum class AttributeError : std::uint8_t
		{
			InvalidObjectID = 0,
			InvalidAttributeID = 1,
			InvalidValue = 2,
			AnyOtherError = 4
		};

		/// @brief Constructor for a generic VT object. Sets up default values and the pointer to the member object pool
		VTObject() = default;

		/// @brief Virtual destructor for a generic VT object
		virtual ~VTObject() = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		virtual VirtualTerminalObjectType get_object_type() const = 0;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		virtual std::uint32_t get_minumum_object_length() const = 0;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool A map of all objects in the current object pool, keyed by their object ID
		/// @returns `true` if the object passed basic error checks
		virtual bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const = 0;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool A map of all objects in the current object pool, keyed by their object ID. Used to validate some object references.
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		virtual bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) = 0;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		virtual bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const = 0;

		/// @brief Returns the object ID of this VT object
		/// @returns The object ID of this VT object
		std::uint16_t get_id() const;

		/// @brief Sets the object ID of this VT object
		/// @param[in] value The new object ID for this object. Must be unique in this pool.
		void set_id(std::uint16_t value);

		/// @brief Returns the width of this object in px
		/// @returns The width of this object in px
		std::uint16_t get_width() const;

		/// @brief Sets the width of this object in px
		/// @param[in] value The new width of this object in px
		void set_width(std::uint16_t value);

		/// @brief Returns the height of this object in px
		/// @returns The height of this object in px
		std::uint16_t get_height() const;

		/// @brief Sets the height of this object in px
		/// @param[in] value The new height of this object in px
		void set_height(std::uint16_t value);

		/// @brief Returns the background color attribute of this object
		/// @returns The background color attribute of this object (index to the actual color in the color table)
		std::uint8_t get_background_color() const;

		/// @brief Sets the background color attribute of this object
		/// @param[in] value The new background color attribute for this object (index to the actual color in the color table)
		void set_background_color(std::uint8_t value);

		/// @brief Returns the number of child objects within this object
		/// @returns The number of child objects within this object
		std::uint16_t get_number_children() const;

		/// @brief Adds an object as a child to another object, which essentially creates a tree of object association
		/// @param[in] objectID The object ID of the child to add
		/// @param[in] relativeXLocation The X offset of this object to its parent
		/// @param[in] relativeYLocation The Y offset of this object to its parent
		void add_child(std::uint16_t objectID, std::int16_t relativeXLocation, std::int16_t relativeYLocation);

		/// @brief Returns the ID of the child by index, if one was added previously
		/// @note NULL_OBJECT_ID is a valid child, so you should always check the number of children to know if the return value of this is "valid"
		/// @param[in] index The index of the child to retrieve
		/// @returns The ID of the child at the specified index, or NULL_OBJECT_ID if the index is out of range
		std::uint16_t get_child_id(std::uint16_t index) const;

		/// @brief Returns the X offset of the child object associated with the specified index into the parent object
		/// @param[in] index The index of the child to retrieve
		/// @returns The relative X position of the child, and always 0 if the index is out of range
		std::int16_t get_child_x(std::uint16_t index) const;

		/// @brief Returns the Y offset of the child object associated with the specified index into the parent object
		/// @param[in] index The index of the child to retrieve
		/// @returns The relative Y position of the child, and always 0 if the index is out of range
		std::int16_t get_child_y(std::uint16_t index) const;

		/// @brief Sets the X offset of the child object associated with the specified index into the parent object
		/// @param[in] index The child index to affect
		/// @param[in] xOffset The relative X position of the child, and always 0 if the index is out of range
		void set_child_x(std::uint16_t index, std::int16_t xOffset);

		/// @brief Sets the Y offset of the child object associated with the specified index into the parent object
		/// @param[in] index The child index to affect
		/// @param[in] yOffset The relative Y position of the child, and always 0 if the index is out of range
		void set_child_y(std::uint16_t index, std::int16_t yOffset);

		/// @brief Offsets all child objects with the specified ID by the amount specified relative to its parent
		/// @param[in] childObjectID The object ID of the children to offset
		/// @param[in] xOffset The relative amount to offset the object(s) by in the X axis
		/// @param[in] yOffset The relative amount to offset the object(s) by in the Y axis
		/// @returns true if any child matched the specified object ID, otherwise false if no children were found with the specified ID.
		bool offset_all_children_with_id(std::uint16_t childObjectID, std::int8_t xOffset, std::int8_t yOffset);

		/// @brief Removes an object reference from another object. All fields must exactly match for the object to be removed.
		/// This is because objects can have multiple of the same child at different places, so we can't infer which one to
		/// remove without the exact position.
		/// @param[in] objectIDToRemove The object ID of the child to remove
		/// @param[in] relativeXLocation The X offset of this object to its parent
		/// @param[in] relativeYLocation The Y offset of this object to its parent
		void remove_child(std::uint16_t objectIDToRemove, std::int16_t relativeXLocation, std::int16_t relativeYLocation);

		/// @brief Removes the last added child object.
		/// This is meant to be a faster way to deal with objects that only have a max of 1 child.
		void pop_child();

		/// @brief Returns the number of macros referenced by this object
		/// @returns The number of macros referenced by this object
		std::uint8_t get_number_macros() const;

		/// @brief Adds a macro to the list of macros referenced by this object
		/// @param[in] macroToAdd The macro to add, which includes the event ID and macro ID
		void add_macro(MacroMetadata macroToAdd);

		/// @brief Returns the macro ID at the specified index
		/// @param[in] index The index of the macro to retrieve
		/// @returns The macro metadata at the specified index, or NULL_OBJECT_ID + EventID::Reserved if the index is out of range
		MacroMetadata get_macro(std::uint8_t index) const;

		/// @brief Returns a VT object from its member pool by ID, or the null id if it does not exist
		/// @param[in] objectID The object ID to search for
		/// @param[in] objectPool The object pool to search in
		/// @returns The object with the corresponding ID
		static std::shared_ptr<VTObject> get_object_by_id(std::uint16_t objectID, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool);

	protected:
		/// @brief Storage for child object data
		class ChildObjectData
		{
		public:
			/// @brief Default constructor for child object data with default values
			ChildObjectData() = default;

			/// @brief Constructor that initializes all members with parameters
			/// @param[in] objectId The object ID of this child object
			/// @param[in] x The x location of this child relative to the parent object
			/// @param[in] y The y location of this child relative to the parent object
			ChildObjectData(std::uint16_t objectId,
			                std::int16_t x,
			                std::int16_t y);
			std::uint16_t id = NULL_OBJECT_ID; ///< Object identifier. Shall be unique within the object pool.
			std::int16_t xLocation = 0; ///< Relative X location of the top left corner of the object
			std::int16_t yLocation = 0; ///< Relative Y location of the top left corner of the object
		};

		std::vector<ChildObjectData> children; ///< List of child objects
		std::vector<MacroMetadata> macros; ///< List of macros referenced by this object
		std::uint16_t objectID = NULL_OBJECT_ID; ///< Object identifier. Shall be unique within the object pool.
		std::uint16_t width = 0; ///< The width of the object. Not always applicable, but often used.
		std::uint16_t height = 0; ///< The height of the object. Not always applicable, but often used.
		std::uint8_t backgroundColor = 0; ///< The background color (from the VT colour table)
	};

	/// @brief Common class for VT objects having variable reference
	class VTObjectWithVariableReference : public VTObject
	{
	public:
		/// @brief Returns the object ID of a variable object that contains the value of the current object
		/// or the null ID if the "value" attribute is used instead.
		/// @returns The object ID of a variable object that contains the value of the current object, or the null ID
		std::uint16_t get_variable_reference() const;

		/// @brief Sets the object ID of the variable object that contains the value of the current object.
		/// Does no error checking on the type of the supplied object.
		/// @param[in] variableValue The object ID of the variable object that contains the value of the current object, or the null ID
		void set_variable_reference(std::uint16_t variableValue);

	protected:
		std::uint16_t variableReference = NULL_OBJECT_ID; ///< Object ID of a number variable object that contains the value of the current object
	};

	/// @brief This object shall include one or more objects that fit inside a Soft Key designator for use as an
	/// identification of the Working Set.
	class WorkingSet : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			BackgroundColour = 1,
			Selectable = 2,
			ActiveMask = 3,

			NumberOfAttributes = 4
		};

		/// @brief Constructor for a working set object
		WorkingSet() = default;

		/// @brief Virtual destructor for a working set object
		~WorkingSet() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating this object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns if the working set is currently selectable
		/// @returns `true` if the working set is currently selectable, otherwise false
		bool get_selectable() const;

		/// @brief Sets if the working set is selectable
		/// @param[in] value `true` to make the working set selectable, otherwise false
		void set_selectable(bool value);

		/// @brief Returns tha currently active mask for this working set
		/// @returns The object ID of the active mask for this working set
		std::uint16_t get_active_mask() const;

		/// @brief Sets the object id of the active mask for this working set
		/// @param[in] value The object ID of the active mask for this working set
		void set_active_mask(std::uint16_t value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 16; ///< The fewest bytes of IOP data that can represent this object

		std::vector<std::string> languageCodes; ///< A list of 2 character language codes, like "en"
		std::uint16_t activeMask = NULL_OBJECT_ID; ///< The currently active mask for this working set
		bool selectable = false; ///< If this working set is selectable right now
	};

	/// @brief The Data Mask describes the objects that will appear in the Data Mask area of the physical display.
	class DataMask : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			BackgroundColour = 1,
			SoftKeyMask = 2,

			NumberOfAttributes = 3
		};

		/// @brief Constructor for a data mask object
		DataMask() = default;

		/// @brief Virtual destructor for a data mask object
		~DataMask() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Changes the soft key mask associated to this data mask to a new object ID.
		/// Performs error checking on the type of the assigned object to ensure it is a soft key mask.
		/// @param[in] newMaskID The object ID of the new soft key mask to associate with this data mask
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @returns True if the mask was changed, false if the new ID was not valid and the mask was not changed
		bool change_soft_key_mask(std::uint16_t newMaskID, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool);

		/// @brief Changes the soft key mask associated to this data mask to a new object ID, but
		/// does no checking on the validity of the new object ID.
		/// @param[in] newMaskID The object ID of the new soft key mask to associate with this data mask
		void set_soft_key_mask(std::uint16_t newMaskID);

		/// @brief Returns the object ID of the soft key mask associated with this data mask
		/// @returns The object ID of the soft key mask associated with this data mask
		std::uint16_t get_soft_key_mask() const;

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 12; ///< The fewest bytes of IOP data that can represent this object
		std::uint16_t softKeyMaskObjectID = NULL_OBJECT_ID; ///< The object ID of the soft key mask associated with this data mask
	};

	/// @brief Similar to a data mask, but takes priority and will be shown over data masks.
	class AlarmMask : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			BackgroundColour = 1,
			SoftKeyMask = 2,
			Priority = 3,
			AcousticSignal = 4,

			NumberOfAttributes = 5
		};

		/// @brief Enumerates the different mask priorities. Higher priority masks will be shown over lower priority ones across all working sets.
		enum class Priority : std::uint8_t
		{
			High = 0, ///< High, operator is in danger or urgent machine malfunction
			Medium = 1, ///< Medium, normal alarm, machine is malfunctioning
			Low = 2 ///< Low, information only
		};

		/// @brief Enumerates the acoustic signal values for the alarm mask. Works only if your VT has a way to make sounds.
		/// @details The result of this setting is somewhat proprietary depending on your VT
		enum class AcousticSignal : std::uint8_t
		{
			Highest = 0, ///< Most aggressive beeping
			Medium = 1, ///< Medium beeping
			Lowest = 3, ///< Low beeping
			None = 4 ///< No beeping
		};

		/// @brief Constructor for a alarm mask object
		AlarmMask() = default;

		/// @brief Virtual destructor for a alarm mask object
		~AlarmMask() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the priority of the alarm mask
		/// @details Higher priority masks will be shown over lower priority ones.
		/// @returns The priority of the alarm mask
		Priority get_mask_priority() const;

		/// @brief Sets the priority of the alarm mask.
		/// @details Higher priority masks will be shown over lower priority ones.
		/// @param[in] value The priority to set
		void set_mask_priority(Priority value);

		/// @brief Returns the acoustic signal priority for the alarm mask.
		/// @details Controls how aggressive the beep is on VTs with a speaker or whistle chip.
		/// @returns The acoustic signal priority of the alarm mask
		AcousticSignal get_signal_priority() const;

		/// @brief Sets the acoustic signal priority for the alarm mask
		/// @details Controls how aggressive the beep is on VTs with a speaker or whistle chip.
		/// @param value The acoustic signal priority to set
		void set_signal_priority(AcousticSignal value);

		/// @brief Changes the soft key mask associated to this alarm mask to a new object ID.
		/// Performs error checking on the type of the assigned object to ensure it is a soft key mask.
		/// @param[in] newMaskID The object ID of the new soft key mask to associate with this data mask
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @returns True if the mask was changed, false if the new ID was not valid and the mask was not changed
		bool change_soft_key_mask(std::uint16_t newMaskID, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool);

		/// @brief Changes the soft key mask associated to this alarm mask to a new object ID, but
		/// does no checking on the validity of the new object ID.
		/// @param[in] newMaskID The object ID of the new soft key mask to associate with this alarm mask
		void set_soft_key_mask(std::uint16_t newMaskID);

		/// @brief Returns the object ID of the soft key mask associated with this alarm mask
		/// @returns The object ID of the soft key mask associated with this alarm mask
		std::uint16_t get_soft_key_mask() const;

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 10; ///< The fewest bytes of IOP data that can represent this object

		std::uint16_t softKeyMask = NULL_OBJECT_ID; ///< Object ID of a soft key mask for this alarm mask, or the null ID
		Priority maskPriority = Priority::High; ///< The priority of this mask
		AcousticSignal signalPriority = AcousticSignal::Highest; ///< The acoustic signal priority for this mask
	};

	/// @brief The Container object is used to group objects for the purpose of moving, hiding or sharing the group.
	/// @details A container is not a visible object, only a logical grouping of other objects. Unlike masks, containers can be
	/// hidden and shown at run-time
	class Container : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			Width = 1,
			Height = 2,
			Hidden = 3,

			NumberOfAttributes = 4
		};

		/// @brief Constructor for a container object
		Container() = default;

		/// @brief Virtual destructor for a container object
		~Container() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the "hidden" attribute for this container
		/// @returns true if the hidden attribute is set, otherwise `false`
		bool get_hidden() const;

		/// @brief Sets the "hidden" attribute for this container
		/// @param[in] value The new attribute state
		void set_hidden(bool value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 9; ///< The fewest bytes of IOP data that can represent this object

		bool hidden = false; ///< The hidden attribute state for this container object. True means it will be hidden when rendered.
	};

	/// @brief The Soft Key Mask is a Container object that contains Key objects, Object Pointer objects, or External Object
	/// Pointer objects.
	/// @details Keys are assigned to physical Soft Keys in the order listed. It is allowable for a Soft Key Mask to
	/// contain no Keys in order that all Soft Keys are effectively disabled when this mask is activated
	class SoftKeyMask : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			BackgroundColour = 1,

			NumberOfAttributes = 2
		};

		/// @brief Constructor for a soft key mask object
		SoftKeyMask() = default;

		/// @brief Virtual destructor for a soft key mask object
		~SoftKeyMask() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 6; ///< The fewest bytes of IOP data that can represent this object
	};

	/// @brief The Key object defines the designator and key code for a Soft Key. Any object located outside of a Soft Key
	/// designator is clipped.
	class Key : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			BackgroundColour = 1,
			KeyCode = 2,

			NumberOfAttributes = 3
		};

		/// @brief Constructor for a key object
		Key() = default;

		/// @brief Virtual destructor for a key object
		~Key() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the key code associated to this key object
		/// @returns The key code associated to this key object
		std::uint8_t get_key_code() const;

		/// @brief Sets the key code associated to this key object
		/// @param[in] value The key code to set
		void set_key_code(std::uint8_t value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 7; ///< The fewest bytes of IOP data that can represent this object

		std::uint8_t keyCode = 0; ///< They key code associated with events from this key object
	};

	/// @brief The Key objects contained in this object shall be a grouping of Key objects, or Object Pointers to Key objects
	class KeyGroup : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			Options = 1,
			Name = 2,

			NumberOfAttributes = 3
		};

		/// @brief Enumerates the options bits in the options bitfield of a KeyGroup
		enum class Options : std::uint8_t
		{
			Available = 0, ///< If 0 (FALSE) this object is not available for use at the present time, even though defined
			Transparent = 1 ///< If this bit is 1, the VT shall	ignore the background colour attribute in all child Key objects
		};

		/// @brief Constructor for a key group object
		KeyGroup() = default;

		/// @brief Virtual destructor for a key group object
		~KeyGroup() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the key group icon that represents this key group
		/// @returns Object ID of the key group icon that represents this key group
		std::uint16_t get_key_group_icon() const;

		/// @brief Sets the object ID of the icon to use when representing this key group
		/// @param[in] value Object ID of a picture graphic to use as the key group icon
		void set_key_group_icon(std::uint16_t value);

		/// @brief Returns the state of a single option in the object's option bitfield
		/// @param[in] option The option to check the value of in the object's option bitfield
		/// @returns The state of the associated option bit
		bool get_option(Options option) const;

		/// @brief Sets the options bitfield for this object to a new value
		/// @param[in] value The new value for the options bitfield
		void set_options(std::uint8_t value);

		/// @brief Sets a single option in the options bitfield to the specified value
		/// @param[in] option The option to set
		/// @param[in] value The new value of the option bit
		void set_option(Options option, bool value);

		/// @brief Sets the Object ID of an Output String object or	an Object Pointer object
		/// that points to an Output String object that contains a name for this object
		/// @returns Object ID of an Output String object or an Object Pointer object that will represent the name of this key group
		std::uint16_t get_name_object_id() const;

		/// @brief Sets the Object ID of an Output String object or	an Object Pointer object
		/// that points to an Output String object that contains a name for this object
		/// @param[in] value The object ID of the object that will represent the name of this key group, CANNOT BE the null object ID
		void set_name_object_id(std::uint16_t value);

		static constexpr std::uint8_t MAX_CHILD_KEYS = 4; ///< There shall be a max of 4 keys per group according to the standard

	private:
		/// @brief Validates that the specified name ID is valid for this object
		/// @param[in] nameIDToValidate The name's object ID to validate
		/// @param[in] objectPool The object pool to use when validating the name object
		/// @returns True if the name ID is valid for this object, otherwise false
		bool validate_name(std::uint16_t nameIDToValidate, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const;

		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 10; ///< The fewest bytes of IOP data that can represent this object

		std::uint16_t keyGroupIcon = NULL_OBJECT_ID; ///< The VT may use this in the proprietary mapping screen to represent the key group
		std::uint16_t nameID = NULL_OBJECT_ID; ///< Object ID of a string variable that contains the name of the key group
		std::uint8_t optionsBitfield = 0; ///< Bitfield of options defined in `Options` enum
	};

	/// @brief The Button object defines a button control.
	/// @details This object is intended mainly for VTs with touch screens or a
	/// pointing method but shall be supported by all VTs.
	class Button : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			Width = 1,
			Height = 2,
			BackgroundColour = 3,
			BorderColour = 4,
			KeyCode = 5,
			Options = 6, // Version 4 and later

			NumberOfAttributes = 7
		};

		/// @brief Enumerates the options encoded into the options bitfield for a button
		enum class Options : std::uint8_t
		{
			Latchable = 0, ///< If TRUE, the Button is latchable and remains pressed until the next activation. If FALSE, the Button is momentary.
			CurrentButtonStateIfLatchable = 1, ///< For latchable Buttons. 0=released, 1=latched
			SuppressBorder = 2, ///< If FALSE, VT draws the proprietary border. If TRUE, no border is ever drawn
			TransparentBackground = 3, ///< If FALSE, the Button's interior background is filled using the background colour attribute. If TRUE, the Button's background is always transparent
			Disabled = 4, ///< If FALSE, the Button is enabled and can be selected and activated by the operator. If TRUE, the Button is drawn disabled (method proprietary)
			NoBorder = 5, ///< If FALSE, the Button Border area is used by the VT as described in Bit 2. If TRUE, Bit 2 is ignored therefore no border is ever drawn and the Button Face extends to the full Button Area
			Reserved1 = 6, ///< Set to 0
			Reserved2 = 7 ///< Set to 0
		};

		/// @brief Constructor for a button object
		Button() = default;

		/// @brief Virtual destructor for a button object
		~Button() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the key code associated with this button's events
		/// @returns The key code associated with this button's events
		std::uint8_t get_key_code() const;

		/// @brief Sets the key code associated with this button's events
		/// @param[in] value The key code to set
		void set_key_code(std::uint8_t value);

		/// @brief Returns the colour of the button's border as an index into the VT colour table
		/// @returns The colour of the button's border as an index into the VT colour table
		std::uint8_t get_border_colour() const;

		/// @brief Sets the border colour
		/// @param[in] value The border colour to set as an index into the VT colour table
		void set_border_colour(std::uint8_t value);

		/// @brief Returns the state of a single option in the object's option bitfield
		/// @param[in] option The option to check the value of in the object's option bitfield
		/// @returns The state of the associated option bit
		bool get_option(Options option) const;

		/// @brief Sets the options bitfield for this object to a new value
		/// @param[in] value The new value for the options bitfield
		virtual void set_options(std::uint8_t value);

		/// @brief Sets a single option in the options bitfield to the specified value
		/// @param[in] option The option to set
		/// @param[in] value The new value of the option bit
		void set_option(Options option, bool value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 13; ///< The fewest bytes of IOP data that can represent this object

		std::uint8_t borderColour = 0; ///< Border colour.
		std::uint8_t keyCode = 0; ///< Key code assigned by ECU. VT reports this code in the Button Activation message.
		std::uint8_t optionsBitfield = 0; ///< Bitfield of options defined in `Options` enum
	};

	/// @brief The Input Boolean object is used to input a TRUE/FALSE type indication from the operator
	class InputBoolean : public VTObjectWithVariableReference
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			BackgroundColour = 1,
			Width = 2,
			ForegroundColour = 3,
			VariableReference = 4,
			Value = 5,
			Enabled = 6, // Version 4 and later

			NumberOfAttributes = 7
		};

		/// @brief Constructor for an input boolean object
		InputBoolean() = default;

		/// @brief Virtual destructor for an input boolean object
		~InputBoolean() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the value of the boolean (only matters if a reference object is not present)
		/// @note The reference object will be a child number variable object if it is present
		/// @returns The value of the boolean object
		std::uint8_t get_value() const;

		/// @brief Sets the value of the boolean object (only matters if a reference object is not present)
		/// @note The reference object will be a child number variable object if it is present
		/// @param[in] inputValue The value to set for the boolean's state
		void set_value(std::uint8_t inputValue);

		/// @brief Returns if this object is enabled based on the enabled attribute
		/// @returns `true` If the enabled attribute on this object is `true`, otherwise `false`
		bool get_enabled() const;

		/// @brief Sets the enabled attribute on this object to a new value
		/// @param[in] isEnabled The new state for the enabled attribute for this object
		void set_enabled(bool isEnabled);

		/// @brief Returns the object ID of a font attributes object that defines the foreground colour, or the null ID
		/// @returns The object ID of a font attributes object that defines the foreground colour, or the null ID
		std::uint16_t get_foreground_colour_object_id() const;

		/// @brief Sets the object ID of the foreground colour object.
		/// Does not perform error checking on the type of the supplied object.
		/// @param[in] fontAttributeValue The object ID of the foreground colour object
		void set_foreground_colour_object_id(std::uint16_t fontAttributeValue);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 13; ///< The fewest bytes of IOP data that can represent this object

		std::uint16_t foregroundColourObjectID = NULL_OBJECT_ID; ///< Object ID of a font attributes that contains the foreground colour of the Input Boolean object
		std::uint8_t value = 0; ///< Used only if it has no number variable child object
		bool enabled = false; ///< If the bool is interactable
	};

	/// @brief Base class for Input/OutputString and Input/OutputNumber
	class TextualVTObject : public VTObjectWithVariableReference
	{
	public:
		/// @brief The allowable horizontal justification options
		enum class HorizontalJustification : std::uint8_t
		{
			PositionLeft = 0, ///< The string is horizontally justified to the left side of its bounding box
			PositionMiddle = 1, ///< The string is horizontally justified to the center of its bounding box
			PositionRight = 2, ///< The string is horizontally justified to the right side of its bounding box
			Reserved = 3 ///< Reserved
		};

		/// @brief The allowable vertical justification options
		enum class VerticalJustification : std::uint8_t
		{
			PositionTop = 0, ///< The string is vertically justified to the top of its bounding box
			PositionMiddle = 1, ///< The string is vertically justified to the center of its bounding box
			PositionBottom = 2, ///< The string is vertically justified to the bottom of its bounding box
			Reserved = 3 ///< Reserved
		};

		/// @brief Returns the horizontal justification setting of the string
		/// @returns The horizontal justification setting of the string
		HorizontalJustification get_horizontal_justification() const;

		/// @brief Returns the vertical justification setting of the string
		/// @returns The vertical justification setting of the string
		VerticalJustification get_vertical_justification() const;

		/// @brief Sets the justification bitfield of the string
		/// @param[in] value The justification bitfield to set
		void set_justification_bitfield(std::uint8_t value);

		/// @brief Sets the options bitfield for this object to a new value
		/// @param[in] value The new value for the options bitfield
		void set_options(std::uint8_t value);

		/// @brief Returns the object ID of a font attributes object that defines the font attributes of the string object
		/// @returns The object ID of a font attributes object that defines the font attributes of the string object
		std::uint16_t get_font_attributes() const;

		/// @brief Sets the object ID of a font attributes object that defines the font attributes of the string object.
		/// Does no error checking on the type of the supplied object.
		/// @param[in] fontAttributesValue The object ID of a font attributes object that defines the font attributes of the string object
		void set_font_attributes(std::uint16_t fontAttributesValue);

	protected:
		std::uint16_t fontAttributes = NULL_OBJECT_ID; ///< Stores the object ID of a font attributes object that will be used to display this object.
		std::uint8_t optionsBitfield = 0; ///< Bitfield of options defined in `Options` enum
		std::uint8_t justificationBitfield = 0; ///< Bitfield of justification options
	};

	/// @brief Base class for InputString and OutputString
	class StringVTObject : public TextualVTObject
	{
	public:
		/// @brief Enumerates the option bits in the options bitfield for a string
		enum class Options
		{
			Transparent = 0, ///< If TRUE, the field is displayed with background showing through instead of using the background colour
			AutoWrap = 1, ///< Auto-Wrapping rules apply
			WrapOnHyphen = 2 ///< If TRUE, Auto-Wrapping can occur between a hyphen and the next character
		};

		/// @brief Returns the state of a single option in the object's option bitfield
		/// @param[in] option The option to check the value of in the object's option bitfield
		/// @returns The state of the associated option bit
		bool get_option(Options option) const;

		/// @brief Sets a single option in the options bitfield to the specified value
		/// @param[in] option The option to set
		/// @param[in] value The new value of the option bit
		void set_option(Options option, bool value);
	};

	/// @brief This object is used to input a character string from the operator
	class InputString : public StringVTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			Width = 1,
			Height = 2,
			BackgroundColour = 3,
			FontAttributes = 4,
			InputAttributes = 5,
			Options = 6,
			VariableReference = 7,
			Justification = 8,
			Enabled = 9, // Version 4 and later

			NumberOfAttributes = 10
		};

		/// @brief Constructor for a input string object
		InputString() = default;

		/// @brief Virtual destructor for a input string object
		~InputString() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns if the input string is enabled for text entry
		/// @returns `true` if the input string is enabled for entry
		bool get_enabled() const;

		/// @brief Sets the enable/disable state of the input string
		/// @param[in] value The new enable/disable state for the input string
		void set_enabled(bool value);

		/// @brief Returns the state of a single option in the object's option bitfield
		/// @param[in] option The option to check the value of in the object's option bitfield
		/// @returns The state of the associated option bit
		bool get_option(Options option) const;

		/// @brief Sets a single option in the options bitfield to the specified value
		/// @param[in] option The option to set
		/// @param[in] value The new value of the option bit
		void set_option(Options option, bool value);

		/// @brief Returns a copy of the stored string value. Used only when no string
		/// variable objects are children of this object.
		/// @returns The value of the string stored in this object
		std::string get_value() const;

		/// @brief Changes the stored string value. Use only when no
		/// string variable objects are children of this object.
		/// @param[in] value The new string value
		void set_value(const std::string &value);

		/// @brief Returns the object ID of a input attributes object that defines what can be input into the Input String object.
		/// @returns The object ID of a input attributes object that defines the input attributes of the Input String object
		std::uint16_t get_input_attributes() const;

		/// @brief Sets the object ID of a input attributes object that defines what can be input into the Input String object.
		/// Does no error checking on the type of the supplied object.
		/// @param[in] inputAttributesValue The object ID of a input attributes object that defines the input attributes of the Input String object
		void set_input_attributes(std::uint16_t inputAttributesValue);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 19; ///< The fewest bytes of IOP data that can represent this object

		std::string stringValue; ///< The actual string. Used only if variable reference attribute is NULL. Pad with spaces as necessary to satisfy length attribute.
		std::uint16_t inputAttributes = NULL_OBJECT_ID; ///< Stores the object ID of a input attributes object that will be used to determine what can be input into this object.
		bool enabled = false; ///< If the string is interactable
	};

	/// @brief Base class for InputNumber and OutputNumber
	class NumberVTObject : public TextualVTObject
	{
	public:
		/// @brief Options that can be applied to the input number
		enum class Options : std::uint8_t
		{
			Transparent = 0, ///< If TRUE, the input field is displayed with background showing through instead of using the background colour
			DisplayLeadingZeros = 1, ///< If TRUE, fill left to width of field with zeros; justification is applied after filling
			DisplayZeroAsBlank = 2, ///< When this option bit is set, a blank field is displayed if and only if the displayed value of the object is exactly zero
			Truncate = 3 ///< If TRUE the value shall be truncated to the specified number of decimals. Otherwise it shall be rounded off to the specified number of decimals.
		};

		/// @brief Returns the state of a single option in the object's option bitfield
		/// @param[in] option The option to check the value of in the object's option bitfield
		/// @returns The state of the associated option bit
		bool get_option(Options option) const;

		/// @brief Sets a single option in the options bitfield to the specified value
		/// @param[in] option The option to set
		/// @param[in] optionValue The new value of the option bit
		void set_option(Options option, bool optionValue);

		/// @brief Returns the value of the output number (only matters if there's no child number variable object).
		/// @returns The value of the output number.
		std::uint32_t get_value() const;

		/// @brief Sets the value of the output number (only matters if there's no child number variable object).
		/// @param[in] inputValue The value to set for the output number
		void set_value(std::uint32_t inputValue);

		/// @brief Returns the scale factor of the output number
		/// @returns The scale factor of the output number
		float get_scale() const;

		/// @brief Sets the scale factor for the output number
		/// @param[in] scaleValue The new value for the scale factor
		void set_scale(float scaleValue);

		/// @brief Returns the offset that is applied to the output number
		/// @returns The offset of the output number
		std::int32_t get_offset() const;

		/// @brief Sets the offset of the output number
		/// @param[in] offsetValue The offset to set for the output number
		void set_offset(std::int32_t offsetValue);

		/// @brief Returns the number of decimals to render in the output number
		/// @returns The number of decimals to render in the output number
		std::uint8_t get_number_of_decimals() const;

		/// @brief Sets the number of decimals to render in the output number
		/// @param[in] decimalValue The number of decimals to render in the output number
		void set_number_of_decimals(std::uint8_t decimalValue);

		/// @brief Returns if the "format" option is set for this object
		/// @details The format option determines if fixed decimal or exponential notation is used.
		/// A value of `false` is fixed decimal notation, and `true` is exponential notation
		/// @returns `true` if the format option is set
		bool get_format() const;

		/// @brief Sets the format option for this object.
		/// @details The format option determines if fixed decimal or exponential notation is used.
		/// A value of `false` is fixed decimal notation, and `true` is exponential notation
		/// @param[in] shouldFormatAsExponential `true` to use fixed decimal notation (####.nn), `false` to use exponential ([−]###.nnE[+/−]##)
		void set_format(bool shouldFormatAsExponential);

	protected:
		std::uint32_t value = 0; ///< Raw unsigned value of the output field before scaling (unsigned 32-bit integer). Used only if variable reference attribute is NULL
		float scale = 1.0f; ///< Scale to be applied to the input value and min/max values.
		std::int32_t offset = 0; ///< Offset to be applied to the input value and min/max values
		std::uint8_t numberOfDecimals = 0; ///< Specifies number of decimals to display after the decimal point
		bool format = false; ///< 0 = use fixed format decimal display (####.nn), 1 = use e
	};

	/// @brief This object is used to format, display and change a numeric value based on a supplied integer value.
	/// @details Displayed value = (value attribute + Offset) * Scaling Factor
	class InputNumber : public NumberVTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			Width = 1,
			Height = 2,
			BackgroundColour = 3,
			FontAttributes = 4,
			Options = 5,
			VariableReference = 6,
			MinValue = 7,
			MaxValue = 8,
			Offset = 9,
			Scale = 10,
			NumberOfDecimals = 11,
			Format = 12,
			Justification = 13,
			Value = 14,
			Options2 = 15, // Version 4 and after

			NumberOfAttributes = 16
		};

		/// @brief More options, for some reason they are different bytes
		enum class Options2 : std::uint8_t
		{
			Enabled = 0, ///< If TRUE the object shall be enabled
			RealTimeEditing = 1 ///< If TRUE the value shall be transmitted to the ECU as it is being changed
		};

		/// @brief Constructor for an input number object
		InputNumber() = default;

		/// @brief Virtual destructor for an input number object
		~InputNumber() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the maximum value for the input number
		/// @details The VT shall not accept values higher than this for this input number's value
		/// @returns The maximum value for the input number
		std::uint32_t get_maximum_value() const;

		/// @brief Sets the maximum value for the input number
		/// @details The VT shall not accept values higher than this for this input number's value
		/// @param[in] newMax The maximum value for the input number
		void set_maximum_value(std::uint32_t newMax);

		/// @brief Returns the minimum value for this input number
		/// @details The VT shall not accept values smaller than this value for this input number
		/// @returns The minimum value for this input number
		std::uint32_t get_minimum_value() const;

		/// @brief Sets the minimum value for the input number
		/// @details The VT shall not accept values smaller than this value for this input number
		/// @param[in] newMin The minimum value to set for the input number
		void set_minimum_value(std::uint32_t newMin);

		/// @brief Returns the state of a single option in the object's second option bitfield
		/// @param[in] newOption The option to check the value of in the object's second option bitfield
		/// @returns The state of the associated option bit
		bool get_option2(Options2 newOption) const;

		/// @brief Sets the second options bitfield for this object to a new value
		/// @param[in] newOptions The new value for the second options bitfield
		void set_options2(std::uint8_t newOptions);

		/// @brief Sets a single option in the second options bitfield to the specified value
		/// @param[in] option The option to set
		/// @param[in] newOption The new value of the option bit
		void set_option2(Options2 option, bool newOption);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 38; ///< The fewest bytes of IOP data that can represent this object

		std::uint32_t maximumValue = 0; ///< Raw maximum value for the input
		std::uint32_t minimumValue = 0; ///< Raw minimum value for the input before scaling
		std::uint8_t options2 = 0; ///< Options byte 2
	};

	/// @brief Common class for Input/OutputList objects
	class ListVTObject : public VTObjectWithVariableReference
	{
	public:
		/// @brief Returns the value of the selected list index (only matters if there is no child number variable)
		/// @returns The value of the selected list index
		std::uint8_t get_value() const;

		/// @brief Sets the selected list index (only matters when the object has no child number variable)
		/// @param[in] inputValue The new value for the selected list index
		void set_value(std::uint8_t inputValue);

		/// @brief Returns the number of items in the list
		/// @note This is not the number of children, it's the number of allocated
		/// list items. The number of children can be less than this number.
		/// @returns The number of items in the list
		std::uint8_t get_number_of_list_items() const;

		/// @brief Sets the number of items in the list
		/// @note This is not the number of children, it's the number of allocated
		/// list items. The number of children can be less than this number.
		/// @param[in] value The number of items in the list
		void set_number_of_list_items(std::uint8_t value);

	protected:
		std::uint8_t numberOfListItems = 0; ///< Number of object references to follow. The size of the list can never exceed this number and this attribute cannot be changed.
		std::uint8_t value = 0; ///< Selected list index of this object. Used only if variable reference attribute is NULL
	};

	/// @brief The Input List object is used to show one object out of a set of objects,
	/// and to allow operator selection of one object from the set.
	class InputList : public ListVTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			Width = 1,
			Height = 2,
			VariableReference = 3,
			Value = 4,
			Options = 5, // Version 4 and after

			NumberOfAttributes = 6
		};

		/// @brief Enumerates the bits in the options bitfield for an InputList
		enum class Options
		{
			Enabled = 0, ///< If true the object shall be enabled
			RealTimeEditing = 1 ///< If true the value shall be transmitted to the ECU as it is being changed
		};

		/// @brief Constructor for an input list object
		InputList() = default;

		/// @brief Virtual destructor for an input list object
		~InputList() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the state of a single option in the object's option bitfield
		/// @param[in] option The option to check the value of in the object's option bitfield
		/// @returns The state of the associated option bit
		bool get_option(Options option) const;

		/// @brief Sets the options bitfield for this object to a new value
		/// @param[in] options The new value for the options bitfield
		void set_options(std::uint8_t options);

		/// @brief Sets a single option in the options bitfield to the specified value
		/// @param[in] option The option to set
		/// @param[in] optionValue The new value of the option bit
		void set_option(Options option, bool optionValue);

		/// @brief Changes a list item to a new ID by index
		/// @param[in] index The index to change (starting from 0)
		/// @param[in] newListItem The object ID to use as the new list item at the specified index
		/// @param[in] objectPool The object pool to use to look up the object ID
		/// @returns True if the operation was successful, otherwise false (perhaps the index is out of bounds?)
		bool change_list_item(std::uint8_t index, std::uint16_t newListItem, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 13; ///< The fewest bytes of IOP data that can represent this object

		std::uint8_t optionsBitfield = 0; ///< Options byte
	};

	/// @brief This object is used to output a string of text
	class OutputString : public StringVTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			Width = 1,
			Height = 2,
			BackgroundColour = 3,
			FontAttributes = 4,
			Options = 5,
			VariableReference = 6,
			Justification = 7,

			NumberOfAttributes = 8
		};

		/// @brief Constructor for an output string object
		OutputString() = default;

		/// @brief Virtual destructor for an output string object
		~OutputString() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the state of a single option in the object's option bitfield
		/// @param[in] option The option to check the value of in the object's option bitfield
		/// @returns The state of the associated option bit
		bool get_option(Options option) const;

		/// @brief Sets a single option in the options bitfield to the specified value
		/// @param[in] option The option to set
		/// @param[in] value The new value of the option bit
		void set_option(Options option, bool value);

		/// @brief Returns the value of the string, used only if the variable reference (a child var string) is NULL_OBJECT_ID
		/// @returns The value of the string
		std::string get_value() const;

		/// @brief Returns the value of the variable (if referenced) otherwise the set value
		/// @param[in] parentWorkingSet the working set of the given OutputString object
		/// @returns The displayed value of the string
		std::string displayed_value(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet) const;

		/// @brief Sets the value of the string (only matters if it has no child string variable)
		/// @param[in] value The new value for the string
		void set_value(const std::string &value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 16; ///< The fewest bytes of IOP data that can represent this object

		std::string stringValue; ///< The actual string. Used only if variable reference attribute is NULL. Pad with spaces as necessary to satisfy length attribute.
	};

	/// @brief This object is used to format and output a numeric value based on a supplied integer value.
	class OutputNumber : public NumberVTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			Width = 1,
			Height = 2,
			BackgroundColour = 3,
			FontAttributes = 4,
			Options = 5,
			VariableReference = 6,
			Offset = 7,
			Scale = 8,
			NumberOfDecimals = 9,
			Format = 10,
			Justification = 11,

			NumberOfAttributes = 12
		};

		/// @brief Constructor for an output number object
		OutputNumber() = default;

		/// @brief Virtual destructor for an output number object
		~OutputNumber() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 29; ///< The fewest bytes of IOP data that can represent this object
	};

	/// @brief Used to show one object out of a set of objects
	class OutputList : public ListVTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			Width = 1,
			Height = 2,
			VariableReference = 3,
			Value = 4,

			NumberOfAttributes = 5
		};

		/// @brief Constructor for an output list object
		OutputList() = default;

		/// @brief Virtual destructor for an output list object
		~OutputList() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Changes a list item to a new ID by index
		/// @param[in] index The index to change (starting from 0)
		/// @param[in] newListItem The object ID to use as the new list item at the specified index
		/// @param[in] objectPool The object pool to use to look up the object ID
		/// @returns True if the operation was successful, otherwise false (perhaps the index is out of bounds?)
		bool change_list_item(std::uint8_t index, std::uint16_t newListItem, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 12; ///< The fewest bytes of IOP data that can represent this object
	};

	/// @brief This object outputs a line shape. The starting point for the line is found in the parent object
	class OutputLine : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			LineAttributes = 1,
			Width = 2,
			Height = 3,
			LineDirection = 4,

			NumberOfAttributes = 5
		};

		/// @brief Enumerates the different directions a line can be drawn
		enum class LineDirection : std::uint8_t
		{
			TopLeftToBottomRight = 0,
			BottomLeftToTopRight = 1
		};

		/// @brief Constructor for an output line object
		OutputLine() = default;

		/// @brief Virtual destructor for an output line object
		~OutputLine() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the line's direction.
		/// @details When the line direction is zero, the ine is drawn from top left to bottom right of
		/// enclosing virtual rectangle. When the line direction is 1, the line is drawn from bottom left to top right of
		/// enclosing virtual rectangle.
		/// @returns The line's direction (see details).
		LineDirection get_line_direction() const;

		/// @brief Sets the line's direction.
		/// @details When the line direction is zero, the ine is drawn from top left to bottom right of
		/// enclosing virtual rectangle. When the line direction is 1, the line is drawn from bottom left to top right of
		/// enclosing virtual rectangle.
		/// @param[in] value The line direction to set (see details).
		void set_line_direction(LineDirection value);

		/// @brief Returns the object ID of the line attributes used to display this line
		/// @returns The object ID of the line attributes used to display this line
		std::uint16_t get_line_attributes() const;

		/// @brief Sets the object ID of the line attributes used to display this line.
		/// Does not perform any error checking on the type of the object specified.
		/// @param[in] lineAttributesObject The object ID of the line attributes used to display this line
		void set_line_attributes(std::uint16_t lineAttributesObject);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 11; ///< The fewest bytes of IOP data that can represent this object

		std::uint16_t lineAttributes = NULL_OBJECT_ID; ///< Object ID of line attributes used to display this line
		std::uint8_t lineDirection = 0; ///< 0 = Line is drawn from top left to bottom right of enclosing virtual rectangle, 1 = Line is drawn from bottom left to top right
	};

	/// @brief This object outputs a rectangle shape
	class OutputRectangle : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			LineAttributes = 1,
			Width = 2,
			Height = 3,
			LineSuppression = 4,
			FillAttributes = 5,

			NumberOfAttributes = 6
		};
		/// @brief The different line suppression options
		enum class LineSuppressionOption
		{
			SuppressTopLine = 0, ///< Suppress the top line of the rectangle
			SuppressRightSideLine = 1, ///< Suppress the right side of the rectangle
			SuppressBottomLine = 2, ///< Suppress the bottom line of the rectangle
			SuppressLeftSideLine = 3 ///< Suppress the left line of the rectangle
		};

		/// @brief Constructor for an output rectangle object
		OutputRectangle() = default;

		/// @brief Virtual destructor for an output rectangle object
		~OutputRectangle() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the line suppression bitfield.
		/// @note See LineSuppressionOption for the bit definitions.
		/// @returns The line suppression bitfield (see LineSuppressionOption).
		std::uint8_t get_line_suppression_bitfield() const;

		/// @brief Sets the line suppression bitfield value.
		/// @note See LineSuppressionOption for the bit definitions.
		/// @param[in] value The line suppression bitfield to set.
		void set_line_suppression_bitfield(std::uint8_t value);

		/// @brief Returns the object ID of the line attributes used to display this rectangle's lines
		/// @returns The object ID of the line attributes used to display this rectangle's lines
		std::uint16_t get_line_attributes() const;

		/// @brief Sets the object ID of the line attributes used to display this rectangle's lines.
		/// Does not perform any error checking on the type of the object specified.
		/// @param[in] lineAttributesObject The object ID of the line attributes used to display this rectangle's lines
		void set_line_attributes(std::uint16_t lineAttributesObject);

		/// @brief Returns the object ID of the fill attributes used to display this rectangle's fill
		/// @returns The object ID of the fill attributes used to display this rectangle's fill
		std::uint16_t get_fill_attributes() const;

		/// @brief Sets the object ID of the fill attributes used to display this rectangle's fill.
		/// Does not perform any error checking on the type of the object specified.
		/// @param[in] fillAttributesObject The object ID of the fill attributes used to display this rectangle's fill
		void set_fill_attributes(std::uint16_t fillAttributesObject);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 13; ///< The fewest bytes of IOP data that can represent this object

		std::uint16_t lineAttributes = NULL_OBJECT_ID; ///< Object ID of line attributes used to display this rectangle
		std::uint16_t fillAttributes = NULL_OBJECT_ID; ///< Object ID of fill attributes used to display this rectangle
		std::uint8_t lineSuppressionBitfield = 0; ///< Bitfield of line suppression options
	};

	/// @brief This object outputs an ellipse or circle shape
	class OutputEllipse : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			LineAttributes = 1,
			Width = 2,
			Height = 3,
			EllipseType = 4,
			StartAngle = 5,
			EndAngle = 6,
			FillAttributes = 7,

			NumberOfAttributes = 8
		};

		/// @brief Types of ellipse
		enum class EllipseType
		{
			Closed = 0, ///< Closed ellipse
			OpenDefinedByStartEndAngles = 1, ///< The ellipse is defined by start and end angles
			ClosedEllipseSegment = 2,
			ClosedEllipseSection = 3
		};

		/// @brief Constructor for an output ellipse object
		OutputEllipse() = default;

		/// @brief Virtual destructor for an output ellipse object
		~OutputEllipse() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the type of the ellipse
		/// @returns The type of the ellipse
		EllipseType get_ellipse_type() const;

		/// @brief Sets the ellipse type
		/// @param[in] value The ellipse type to set
		void set_ellipse_type(EllipseType value);

		/// @brief Returns the Start angle/2 (in degrees) from positive X axis
		/// counter clockwise(90° is straight up) for the ellipse.
		/// @details The range for this is 0 to 180.
		/// @note If type > 0 and start and end angles are the same, the ellipse is drawn closed.
		/// @returns Start angle/2 (in degrees) from positive X axis counter clockwise (90° is straight up)
		std::uint8_t get_start_angle() const;

		/// @brief Sets the start angle for the ellipse
		/// @note If type > 0 and start and end angles are the same, the ellipse is drawn closed.
		/// @param[in] value Start angle/2 (in degrees) from positive X axis counter clockwise(90° is straight up)
		void set_start_angle(std::uint8_t value);

		/// @brief Returns the end angle/2 (in degrees) from positive X axis counter clockwise(90° is straight up).
		/// @details The range for this is 0 to 180.
		/// @note If type > 0 and start and end angles are the same, the ellipse is drawn closed.
		/// @returns End angle/2 (in degrees) from positive X axis counter clockwise (90° is straight up)
		std::uint8_t get_end_angle() const;

		/// @brief Sets the end angle for the ellipse.
		/// @note If type > 0 and start and end angles are the same, the ellipse is drawn closed.
		/// @param[in] value The end angle/2 (in degrees) from positive X axis counter clockwise(90° is straight up).
		void set_end_angle(std::uint8_t value);

		/// @brief Returns the object ID of the line attributes used to display this ellipse's lines
		/// @returns The object ID of the line attributes used to display this ellipse's lines
		std::uint16_t get_line_attributes() const;

		/// @brief Sets the object ID of the line attributes used to display this ellipse's lines.
		/// Does not perform any error checking on the type of the object specified.
		/// @param[in] lineAttributesObject The object ID of the line attributes used to display this ellipse's lines
		void set_line_attributes(std::uint16_t lineAttributesObject);

		/// @brief Returns the object ID of the fill attributes used to display this ellipse's fill
		/// @returns The object ID of the fill attributes used to display this ellipse's fill
		std::uint16_t get_fill_attributes() const;

		/// @brief Sets the object ID of the fill attributes used to display this ellipse's fill.
		/// Does not perform any error checking on the type of the object specified.
		/// @param[in] fillAttributesObject The object ID of the fill attributes used to display this ellipse's fill
		void set_fill_attributes(std::uint16_t fillAttributesObject);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 15; ///< The fewest bytes of IOP data that can represent this object

		std::uint16_t lineAttributes = NULL_OBJECT_ID; ///< Object ID of line attributes used to display this ellipse
		std::uint16_t fillAttributes = NULL_OBJECT_ID; ///< Object ID of fill attributes used to display this ellipse
		std::uint8_t ellipseType = 0; ///< The type of ellipse
		std::uint8_t startAngle = 0; ///< Start angle/2 (in degrees) from positive X axis counter clockwise (90° is straight up).
		std::uint8_t endAngle = 0; ///< End angle/2 (in degrees) from positive X axis counter clockwise (90° is straight up)
	};

	/// @brief This object outputs a polygon
	class OutputPolygon : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			Width = 1,
			Height = 2,
			LineAttributes = 3,
			FillAttributes = 4,
			PolygonType = 5,

			NumberOfAttributes = 6
		};

		/// @brief Polygon type. The first three types are useful only if the polygon is to be filled.
		enum class PolygonType
		{
			Convex = 0, ///< On any given horizontal line, only two points on the polygon are encountered
			NonConvex = 1, ///< On any given horizontal line, more than two points on the polygon edges can be encountered but the polygon edges do not cross
			Complex = 2, ///< Similar to Non-convex but edges cross. Uses Complex Fill Algorithm
			Open = 3 ///< This type cannot be filled
		};

		/// @brief Stores a cartesian polygon point
		struct PolygonPoint
		{
			std::uint16_t xValue; ///< X value of a point relative to the top left corner of the polygon
			std::uint16_t yValue; ///< Y value of a point relative to the top left corner of the polygon
		};

		/// @brief Constructor for an output polygon object
		OutputPolygon() = default;

		/// @brief Virtual destructor for an output polygon object
		~OutputPolygon() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Adds a point to the polygon, defined by x and y coordinates
		/// @param[in] x The X value of a point relative to the top left corner	of the polygon
		/// @param[in] y The Y value of a point relative to the top left corner	of the polygon
		void add_point(std::uint16_t x, std::uint16_t y);

		/// @brief Returns the number of polygon points
		/// @returns The number of polygon points
		std::uint8_t get_number_of_points() const;

		/// @brief Returns a point from the polygon by index
		/// @param[in] index The index of the point to retrieve
		/// @returns A point in the polygon by index, or zeros if the index is out of range.
		PolygonPoint get_point(std::uint8_t index);

		/// @brief Changes a polygon point by index
		/// @param[in] index The point index to modify
		/// @param[in] x The new X position of the point, relative to the top left corner of the polygon
		/// @param[in] y The new Y position of the point, relative to the top left corner of the polygon
		/// @returns True if the point was modified, false if the index was out of range
		bool change_point(std::uint8_t index, std::uint16_t x, std::uint16_t y);

		/// @brief Returns the polygon type of this object
		/// @returns The polygon type of this object
		PolygonType get_type() const;

		/// @brief Sets the polygon type for this object
		/// @param[in] value The new polygon type for this object
		void set_type(PolygonType value);

		/// @brief Returns the object ID of the line attributes used to display this polygon's lines
		/// @returns The object ID of the line attributes used to display this polygon's lines
		std::uint16_t get_line_attributes() const;

		/// @brief Sets the object ID of the line attributes used to display this polygon's lines.
		/// Does not perform any error checking on the type of the object specified.
		/// @param[in] lineAttributesObject The object ID of the line attributes used to display this polygon's lines
		void set_line_attributes(std::uint16_t lineAttributesObject);

		/// @brief Returns the object ID of the fill attributes used to display this polygon's fill
		/// @returns The object ID of the fill attributes used to display this polygon's fill
		std::uint16_t get_fill_attributes() const;

		/// @brief Sets the object ID of the fill attributes used to display this polygon's fill.
		/// Does not perform any error checking on the type of the object specified.
		/// @param[in] fillAttributesObject The object ID of the fill attributes used to display this polygon's fill
		void set_fill_attributes(std::uint16_t fillAttributesObject);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 14; ///< The fewest bytes of IOP data that can represent this object

		std::vector<PolygonPoint> pointList; ///< List of points that make up the polygon. Must be at least 3 points!
		std::uint16_t fillAttributes = NULL_OBJECT_ID; ///< Object ID of fill attributes used to display this polygon
		std::uint16_t lineAttributes = NULL_OBJECT_ID; ///< Object ID of line attributes used to display this polygon
		std::uint8_t polygonType = 0; ///< The polygon type. Affects how the object gets drawn.
	};

	/// @brief This object is a meter. Meter is drawn about a circle enclosed within a defined square.
	class OutputMeter : public VTObjectWithVariableReference
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			Width = 1,
			NeedleColour = 2,
			BorderColour = 3,
			ArcAndTickColour = 4,
			Options = 5,
			NumberOfTicks = 6,
			StartAngle = 7,
			EndAngle = 8,
			MinValue = 9,
			MaxValue = 10,
			VariableReference = 11,
			Value = 12,

			NumberOfAttributes = 13
		};

		/// @brief Options that can be applied to the input number
		enum class Options : std::uint8_t
		{
			DrawArc = 0, ///< Draw Arc
			DrawBorder = 1, ///< Draw Border
			DrawTicks = 2, ///< Draw Ticks
			DeflectionDirection = 3 ///< 0 = From min to max, counterclockwisee. 1 = from min to max, clockwise
		};

		/// @brief Constructor for an output meter object
		OutputMeter() = default;

		/// @brief Virtual destructor for an output meter object
		~OutputMeter() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the minimum value of the output meter
		/// @returns The minimum value of the output meter
		std::uint16_t get_min_value() const;

		/// @brief Sets the minimum value of the output meter
		/// @param[in] value The minimum value to set for the output meter
		void set_min_value(std::uint16_t value);

		/// @brief Returns the max value for the output meter
		/// @returns The max value for the output meter
		std::uint16_t get_max_value() const;

		/// @brief Sets the max value for the output meter
		/// @param[in] value The max value to set for the output meter
		void set_max_value(std::uint16_t value);

		/// @brief Returns the value for the output meter (only matters if there's no child number variable object).
		/// @returns The value of the output meter
		std::uint16_t get_value() const;

		/// @brief Sets the value of the output meter (only matters if there's no child number variable object).
		/// @param[in] value The value to set for the output meter
		void set_value(std::uint16_t value);

		/// @brief Returns the value of the needle colour
		/// @returns The value of the needle colour as an index into the VT colour table
		std::uint8_t get_needle_colour() const;

		/// @brief Sets the value of the needle colour
		/// @param[in] colourIndex The colour to set for the needle as an index into the VT colour table
		void set_needle_colour(std::uint8_t colourIndex);

		/// @brief Returns the border colour of the meter
		/// @returns The border colour of the meter as an index into the VT colour table
		std::uint8_t get_border_colour() const;

		/// @brief Sets the border colour of the meter
		/// @param[in] colourIndex The border colour to set for the meter as an index into the VT colour table
		void set_border_colour(std::uint8_t colourIndex);

		/// @brief Returns the arc and tick colour for the meter
		/// @returns The arc and tick colour for the meter as an index into the VT colour table
		std::uint8_t get_arc_and_tick_colour() const;

		/// @brief Sets the arc and tick colour for the meter
		/// @param[in] colourIndex The arc and tick colour to set for the meter as an index into the VT colour table
		void set_arc_and_tick_colour(std::uint8_t colourIndex);

		/// @brief Returns the number of ticks to render across the meter
		/// @returns The number of ticks to render across the meter
		std::uint8_t get_number_of_ticks() const;

		/// @brief Sets the number of ticks to render when drawing the meter
		/// @param[in] ticks The number of ticks to render
		void set_number_of_ticks(std::uint8_t ticks);

		/// @brief Returns the state of a single option in the object's option bitfield
		/// @param[in] option The option to check the value of in the object's option bitfield
		/// @returns The state of the associated option bit
		bool get_option(Options option) const;

		/// @brief Sets the options bitfield for this object to a new value
		/// @param[in] options The new value for the options bitfield
		void set_options(std::uint8_t options);

		/// @brief Sets a single option in the options bitfield to the specified value
		/// @param[in] option The option to set
		/// @param[in] optionValue The new value of the option bit
		void set_option(Options option, bool optionValue);

		/// @brief Returns the start angle for the meter
		/// @note If the start and end angles are the same the meter’s arc is closed.
		/// @returns Start angle/2 (in degrees) from positive X axis anticlockwise(90° is straight up).
		std::uint8_t get_start_angle() const;

		/// @brief Sets the start angle for the meter
		/// @note If the start and end angles are the same the meter’s arc is closed.
		/// @param[in] value Start angle/2 (in degrees) from positive X axis anticlockwise(90° is straight up).
		void set_start_angle(std::uint8_t value);

		/// @brief Returns the end angle of the meter.
		/// @note If the start and end angles are the same the meter’s arc is closed.
		/// @returns The end angle/2 (in degrees) from positive X axis anticlockwise(90° is straight up).
		std::uint8_t get_end_angle() const;

		/// @brief Sets the end angle for this meter in degrees from the +x axis counter clockwise
		/// @note If the start and end angles are the same the meter’s arc is closed.
		/// @param[in] value End angle/2 (in degrees) from positive X axis anticlockwise(90° is straight up).
		void set_end_angle(std::uint8_t value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 21; ///< The fewest bytes of IOP data that can represent this object

		std::uint16_t minValue = 0; ///< Minimum value. Represents value when needle is at the start of arc
		std::uint16_t maxValue = 0; ///< Maximum value. Represents when the needle is at the end of the arc.
		std::uint16_t value = 0; ///< Current value. Needle position set to this value, used if variable ref is NULL.
		std::uint8_t needleColour = 0; ///< Needle (indicator) colour
		std::uint8_t borderColour = 0; ///< Border colour (if drawn)
		std::uint8_t arcAndTickColour = 0; ///< Meter arc and tick colour (if drawn)
		std::uint8_t optionsBitfield = 0; ///< Bitfield of options defined in `Options` enum
		std::uint8_t numberOfTicks = 0; ///< Number of ticks to draw about meter arc
		std::uint8_t startAngle = 0; ///< Start angle / 2 in degrees from positive X axis counterclockwise
		std::uint8_t endAngle = 0; ///< End angle / 2 in degrees from positve X axis counterclockwise
	};

	/// @brief This is a linear bar graph or thermometer, defined by an enclosing rectangle.
	class OutputLinearBarGraph : public VTObjectWithVariableReference
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			Width = 1,
			Height = 2,
			Colour = 3,
			TargetLineColour = 4,
			Options = 5,
			NumberOfTicks = 6,
			MinValue = 7,
			MaxValue = 8,
			VariableReference = 9,
			TargetValueVariableReference = 10,
			TargetValue = 11,
			Value = 12,

			NumberOfAttributes = 13
		};

		/// @brief Options that can be applied to the input number
		enum class Options : std::uint8_t
		{
			DrawBorder = 0, ///< Draw Border
			DrawTargetLine = 1, ///< Draw Target Line
			DrawTicks = 2, ///< Draw Ticks
			BarGraphType = 3, ///< 0 = Filled, 1 = not filled with value line
			AxisOrientation = 4, ///< 0 = vertical, 1 = horizontal
			Direction = 5 ///< 0 = Grows negative, 1 = Grows positive
		};

		/// @brief Constructor for an output linear bar graph object
		OutputLinearBarGraph() = default;

		/// @brief Virtual destructor for an output linear bar graph object
		~OutputLinearBarGraph() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the minimum value on the graph. Used to scale the graph's range.
		/// @returns The minimum value that will be shown on the graph.
		std::uint16_t get_min_value() const;

		/// @brief Sets the minimum value on the graph.
		/// @details Used to scale the graph's range. Values below this will be clamped to the min.
		/// @param[in] value The minimum value to set
		void set_min_value(std::uint16_t value);

		/// @brief Returns the max value for the graph
		/// @returns The max value for the graph
		std::uint16_t get_max_value() const;

		/// @brief Sets the max value for the graph
		/// @param[in] value The max value to set for the graph
		void set_max_value(std::uint16_t value);

		/// @brief Returns the value of the graph (only matters if there's no child number variable object).
		/// @returns The value of the graph
		std::uint16_t get_value() const;

		/// @brief Sets the value of the graph (only matters if there's no child number variable object).
		/// @param[in] value The value to set for the graph
		void set_value(std::uint16_t value);

		/// @brief Returns the graph's target value (only matters if there's no target value reference).
		/// @returns The graph's target value
		std::uint16_t get_target_value() const;

		/// @brief Sets the target value for the graph (only matters if there's no target value reference).
		/// @param[in] valueTarget The target value to set
		void set_target_value(std::uint16_t valueTarget);

		/// @brief Returns the target value reference object ID
		/// @details This object will be used (if it's not NULL_OBJECT_ID)
		/// to determine the target value of the graph instead of the target value itself.
		/// @returns The object ID of a number variable to use for the target value
		std::uint16_t get_target_value_reference() const;

		/// @brief Sets the target value reference object ID
		/// @details This object will be used (if it's not NULL_OBJECT_ID)
		/// to determine the target value of the graph instead of the target value itself.
		/// @param[in] valueReferenceObjectID The object ID of a number variable to use for the target value
		void set_target_value_reference(std::uint16_t valueReferenceObjectID);

		/// @brief Returns the number of ticks to render across the graph
		/// @returns The number of ticks to render across the graph
		std::uint8_t get_number_of_ticks() const;

		/// @brief Sets the number of ticks to render when drawing the graph
		/// @param[in] value The number of ticks to graph
		void set_number_of_ticks(std::uint8_t value);

		/// @brief Returns the colour of the graph
		/// @returns The colour of the graph as an index into the VT colour table
		std::uint8_t get_colour() const;

		/// @brief Sets the colour of the graph
		/// @param[in] graphColour The colour of the graph to set as an index into the VT colour table
		void set_colour(std::uint8_t graphColour);

		/// @brief Returns the target line colour as an index into the VT colour table
		/// @returns The target line colour as an index into the VT colour table
		std::uint8_t get_target_line_colour() const;

		/// @brief Sets the target line colour
		/// @param[in] lineColour The colour to set for the target line as an index into the VT colour table
		void set_target_line_colour(std::uint8_t lineColour);

		/// @brief Returns the state of a single option in the object's option bitfield
		/// @param[in] option The option to check the value of in the object's option bitfield
		/// @returns The state of the associated option bit
		bool get_option(Options option) const;

		/// @brief Sets the options bitfield for this object to a new value
		/// @param[in] options The new value for the options bitfield
		void set_options(std::uint8_t options);

		/// @brief Sets a single option in the options bitfield to the specified value
		/// @param[in] option The option to set
		/// @param[in] optionValue The new value of the option bit
		void set_option(Options option, bool optionValue);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 24; ///< The fewest bytes of IOP data that can represent this object

		std::uint16_t minValue = 0; ///< Minimum value
		std::uint16_t maxValue = 0; ///< Maximum value
		std::uint16_t targetValue = 0; ///< Current target value. Used only if Target value variable Reference attribute is NULL.
		std::uint16_t targetValueReference = NULL_OBJECT_ID; ///< Object ID of a Number Variable object in which	to retrieve the bar graph’s target value.
		std::uint16_t value = 0; ///< Current value. Needle position set to this value, used if variable ref is NULL.
		std::uint8_t numberOfTicks = 0; ///< Number of ticks to draw along the bar graph
		std::uint8_t colour = 0; ///< Bar graph fill and border colour.
		std::uint8_t targetLineColour = 0; ///< Target line colour (if drawn).
		std::uint8_t optionsBitfield = 0; ///< Bitfield of options defined in `Options` enum
	};

	/// @brief TThis object is similar in concept to a linear bar graph but appears arched. Arched bar graphs are drawn about
	/// an Output Ellipse object enclosed within a defined rectangle
	class OutputArchedBarGraph : public VTObjectWithVariableReference
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			Width = 1,
			Height = 2,
			Colour = 3,
			TargetLineColour = 4,
			Options = 5,
			StartAngle = 6,
			EndAngle = 7,
			BarGraphWidth = 8,
			MinValue = 9,
			MaxValue = 10,
			VariableReference = 11,
			TargetValueVariableReference = 12,
			TargetValue = 13,

			NumberOfAttributes = 14
		};

		/// @brief Options that can be applied to the input number
		enum class Options : std::uint8_t
		{
			DrawBorder = 0, ///< Draw border
			DrawTargetLine = 1, ///< Draw a target line
			Undefined = 2, ///< Undefined, set to 0 recommended
			BarGraphType = 3, ///< bar graph type. If this bit is FALSE (0), bar graph is filled
			Deflection = 4 ///< 0 = anticlockwise and 1 = clockwise
		};

		/// @brief Constructor for an output arched bar graph object
		OutputArchedBarGraph() = default;

		/// @brief Virtual destructor for an output arched bar graph object
		~OutputArchedBarGraph() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the width (px) of the bar graph
		/// @returns The width (px) of the bar graph
		std::uint16_t get_bar_graph_width() const;

		/// @brief Sets the width (px) of the bar graph
		/// @param[in] width The width (px) to set for the bar graph
		void set_bar_graph_width(std::uint16_t width);

		/// @brief Returns the minimum value of the bar graph.
		/// @note Values below this will be clamped to the min when rendered.
		/// @returns The minimum value of the bar graph
		std::uint16_t get_min_value() const;

		/// @brief Sets the minimum value for the bar graph
		/// @note Values below this will be clamped to the min when rendered.
		/// @param[in] minimumValue The minimum value to set
		void set_min_value(std::uint16_t minimumValue);

		/// @brief Returns the maximum value of the bar graph
		/// @note Values above this will be clamped to the max when rendered.
		/// @returns The maximum value of the bar graph
		std::uint16_t get_max_value() const;

		/// @brief Sets the max value of the bar graph
		/// @note Values above this will be clamped to the max when rendered.
		/// @param[in] maximumValue The maximum value of the bar graph to set
		void set_max_value(std::uint16_t maximumValue);

		/// @brief Returns the value of the bar graph (only matters when no child number variable is used)
		/// @returns The value of the bar graph
		std::uint16_t get_value() const;

		/// @brief Sets the value of the bar graph (only matters when no child number variable is used)
		/// @param[in] value The value to set for the bar graph
		void set_value(std::uint16_t value);

		/// @brief Returns the colour of the target line
		/// @returns The colour of the target line as an index into the VT colour table
		std::uint8_t get_target_line_colour() const;

		/// @brief Sets the colour of the target line
		/// @param[in] value The colour to set as an index into the VT colour table
		void set_target_line_colour(std::uint8_t value);

		/// @brief Returns the colour of the bar graph
		/// @returns The colour of the bar graph as an index into the VT colour table
		std::uint8_t get_colour() const;

		/// @brief Sets the colour of the bar graph
		/// @param[in] value The colour to set for the bar graph as an index into the VT colour table
		void set_colour(std::uint8_t value);

		/// @brief Returns the state of a single option in the object's option bitfield
		/// @param[in] option The option to check the value of in the object's option bitfield
		/// @returns The state of the associated option bit
		bool get_option(Options option) const;

		/// @brief Sets the options bitfield for this object to a new value
		/// @param[in] options The new value for the options bitfield
		void set_options(std::uint8_t options);

		/// @brief Sets a single option in the options bitfield to the specified value
		/// @param[in] option The option to set
		/// @param[in] optionValue The new value of the option bit
		void set_option(Options option, bool optionValue);

		/// @brief Returns the start angle of the graph
		/// @returns Start angle/2 (in degrees) from positive X axis anticlockwise (90° is straight up) for the graph
		std::uint8_t get_start_angle() const;

		/// @brief Sets the start angle for the graph
		/// @param[in] value Start angle/2 (in degrees) from positive X axis anticlockwise (90° is straight up) for the graph
		void set_start_angle(std::uint8_t value);

		/// @brief Returns the end angle of the graph
		/// @returns End angle/2 (in degrees) from positive X axis anticlockwise (90° is straight up) for the graph
		std::uint8_t get_end_angle() const;

		/// @brief Sets the end angle for the graph
		/// @param[in] value End angle/2 (in degrees) from positive X axis anticlockwise (90° is straight up) for the graph
		void set_end_angle(std::uint8_t value);

		/// @brief Returns the target value of the graph (only matters when no target value reference is used)
		/// @returns The target value of the graph
		std::uint16_t get_target_value() const;

		/// @brief Sets the target value of the graph (only matters when no target value reference is used)
		/// @param[in] value The target value of the graph
		void set_target_value(std::uint16_t value);

		/// @brief Returns the target value reference object ID
		/// @details This object will be used (if it's not NULL_OBJECT_ID)
		/// to determine the target value of the graph instead of the target value itself.
		/// @returns The object ID of a number variable to use for the target value
		std::uint16_t get_target_value_reference() const;

		/// @brief Sets the target value reference object ID
		/// @details This object will be used (if it's not NULL_OBJECT_ID)
		/// to determine the target value of the graph instead of the target value itself.
		/// @param[in] value The object ID of a number variable to use for the target value
		void set_target_value_reference(std::uint16_t value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 27; ///< The fewest bytes of IOP data that can represent this object

		std::uint16_t barGraphWidth = 0; ///< Bar graph width in pixels. Bar graph width should be less than half the total width, or less than half the total height, whichever is least.
		std::uint16_t minValue = 0; ///< Minimum value. Represents value when needle is at the start of arc
		std::uint16_t maxValue = 0; ///< Maximum value. Represents when the needle is at the end of the arc.
		std::uint16_t value = 0; ///< Current value. Needle position set to this value, used if variable ref is NULL.
		std::uint16_t targetValue = 0; ///< Current target value. Used only if Target value variable Reference attribute is NULL.
		std::uint16_t targetValueReference = NULL_OBJECT_ID; ///< Object ID of a Number Variable object in which to retrieve the bar graph’s target value.
		std::uint8_t targetLineColour = 0; ///< Target line colour (if drawn)
		std::uint8_t colour = 0; ///< Bar graph fill and border colour
		std::uint8_t optionsBitfield = 0; ///< Bitfield of options defined in `Options` enum
		std::uint8_t startAngle = 0; ///< Start angle / 2 in degrees from positive X axis counterclockwise
		std::uint8_t endAngle = 0; ///< End angle / 2 in degrees from positive X axis counterclockwise
	};

	/// @brief This object displays a picture graphic (bitmap)
	class PictureGraphic : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			Width = 1,
			Options = 2,
			TransparencyColour = 3,
			ActualWidth = 4,
			ActualHeight = 5,
			Format = 6,

			NumberOfAttributes = 7
		};

		/// @brief Enumerates the different colour formats a picture graphic can have (mutually exclusive)
		enum class Format
		{
			Monochrome = 0, ///< Monochrome; 8 pixels per byte. Each bit represents a colour palette index of 0 or 1.
			FourBitColour = 1, ///< 2 colour pixels per byte. Each nibble(4 bits) represents a colour palette index of 0 through 15.
			EightBitColour = 2 ///< colour pixel per byte. Each byte represents a colour palette index of 0	through 255
		};

		/// @brief Enumerates the different options bits in the options bitfield
		enum class Options
		{
			Transparent = 0, ///< 0 = Opaque, 1 = Transparent
			Flashing = 1, ///< 0 = Normal, 1 = Flashing
			RunLengthEncoded = 2 ///< Data is RLE See Clause B.12.2 Picture Graphic object raw data format and compression
		};

		/// @brief Constructor for a picture graphic (bitmap) object
		PictureGraphic() = default;

		/// @brief Virtual destructor for a picture graphic (bitmap) object
		~PictureGraphic() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns a reference to the underlying bitmap data
		/// @returns A reference to the underlying bitmap data
		std::vector<std::uint8_t> &get_raw_data();

		/// @brief Sets a large chunk of data to the underlying bitmap
		/// @param[in] data Pointer to a buffer of data
		/// @param[in] size The length of the data buffer to add to the underlying bitmap
		void set_raw_data(const std::uint8_t *data, std::uint32_t size);

		/// @brief Sets one byte of raw data to the underlying bitmap
		/// @param[in] dataByte One byte of bitmap data
		void add_raw_data(std::uint8_t dataByte);

		/// @brief Returns the number of bytes in the raw data that comprises the underlying bitmap
		/// @returns The number of bytes in the raw data that comprises the underlying bitmap
		std::uint32_t get_number_of_bytes_in_raw_data() const;

		/// @brief Sets the number of bytes in the raw data that comprises the underlying bitmap
		/// @param[in] value The number of bytes in the raw data that comprises the underlying bitmap
		void set_number_of_bytes_in_raw_data(std::uint32_t value);

		/// @brief Returns the actual width of the underlying bitmap
		/// @returns The actual width of the underlying bitmap (px)
		std::uint16_t get_actual_width() const;

		/// @brief Sets the actual width of the underlying bitmap
		/// @param[in] value Actual width to set for the underlying bitmap (px)
		void set_actual_width(std::uint16_t value);

		/// @brief Returns the actual height of the underlying bitmap
		/// @returns The actual height of the underlying bitmap (px)
		std::uint16_t get_actual_height() const;

		/// @brief Sets the actual height of the underlying bitmap
		/// @param[in] value Actual height to set for the underlying bitmap (px)
		void set_actual_height(std::uint16_t value);

		/// @brief Returns the picture's colour format
		/// @returns The picture colour format
		Format get_format() const;

		/// @brief Sets the picture's colour format
		/// @param[in] value The colour format to use for this picture graphic
		void set_format(Format value);

		/// @brief Returns the state of a single option in the object's option bitfield
		/// @param[in] option The option to check the value of in the object's option bitfield
		/// @returns The state of the associated option bit
		bool get_option(Options option) const;

		/// @brief Sets the options bitfield for this object to a new value
		/// @param[in] value The new value for the options bitfield
		void set_options(std::uint8_t value);

		/// @brief Sets a single option in the options bitfield to the specified value
		/// @param[in] option The option to set
		/// @param[in] value The new value of the option bit
		void set_option(Options option, bool value);

		/// @brief Returns the transparency colour to use when rendering the object as an index into the VT colour table
		/// @returns Transparency colour to use when rendering the object as an index into the VT colour table
		std::uint8_t get_transparency_colour() const;

		/// @brief Sets the transparency colour to use when rendering the object as an index into the VT colour table
		/// @param[in] value The colour to use when rendering the object as an index into the VT colour table
		void set_transparency_colour(std::uint8_t value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 17; ///< The fewest bytes of IOP data that can represent this object

		std::vector<std::uint8_t> rawData; ///< The raw picture data. Not a standard bitmap, but rather indicies into the VT colour table.
		std::uint32_t numberOfBytesInRawData = 0; ///< Number of bytes of raw data
		std::uint16_t actualWidth = 0; ///< The actual width of the bitmap
		std::uint16_t actualHeight = 0; ///< The actual height of the bitmap
		std::uint8_t formatByte = 0; ///< The format option byte
		std::uint8_t optionsBitfield = 0; ///< Options bitfield, see the `options` enum
		std::uint8_t transparencyColour = 0; ///< The colour to render as transparent if so set in the options
	};

	/// @brief A number variable holds a 32-bit unsigned integer value
	class NumberVariable : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			Value = 1,

			NumberOfAttributes = 2
		};

		/// @brief Constructor for a number variable object
		NumberVariable() = default;

		/// @brief Virtual destructor for a number variable object
		~NumberVariable() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the number variable's value
		/// @returns The number variable's value
		std::uint32_t get_value() const;

		/// @brief Sets the number variable's value
		/// @param[in] value The value to set for the number variable
		void set_value(std::uint32_t value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 7; ///< The fewest bytes of IOP data that can represent this object

		std::uint32_t value = 0; ///< 32-bit unsigned integer value
	};

	/// @brief A String Variable holds a fixed length string.
	class StringVariable : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,

			NumberOfAttributes = 1
		};

		/// @brief Constructor for a string variable object
		StringVariable() = default;

		/// @brief Virtual destructor for a string variable object
		~StringVariable() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the actual string value stored in this object
		/// @returns The string value stored in this object
		std::string get_value() const;

		/// @brief Sets the actual string value stored in this object
		/// @param[in] value The new string value for this object
		void set_value(const std::string &value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 5; ///< The fewest bytes of IOP data that can represent this object

		std::string value; ///< The actual value of the string, for non utf-16 strings
	};

	/// @brief This object holds attributes related to fonts.
	class FontAttributes : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			FontColour = 1,
			FontSize = 2,
			FontType = 3,
			FontStyle = 4,

			NumberOfAttributes = 5
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
			Inverted = 4, ///< Inverted font style (exchange background and pen colours)
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

		/// @brief Constructor for a font attributes object
		FontAttributes() = default;

		/// @brief Virtual destructor for a font attributes object
		~FontAttributes() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the font type associated to this font attributes object
		/// @returns The font type associated to this font attributes object
		FontType get_type() const;

		/// @brief Sets the font type
		/// @param[in] value The font type to set
		void set_type(FontType value);

		/// @brief Returns the font style bitfield
		/// @returns The style bitfield, which is comprised of FontStyleBits
		std::uint8_t get_style() const;

		/// @brief Returns a specific font style bit's state
		/// @param[in] styleSetting The font style bit to check
		/// @returns The state of the selected style bit
		bool get_style(FontStyleBits styleSetting) const;

		/// @brief Sets a specific font style bit to a new value
		/// @param[in] bit The style bit to change
		/// @param[in] value The state to set for the selected style bit
		void set_style(FontStyleBits bit, bool value);

		/// @brief Sets the font style bitfield to a new value
		/// @param[in] value The value to set to the font style bitfield
		void set_style(std::uint8_t value);

		/// @brief Returns the font size
		/// @returns The font size
		FontSize get_size() const;

		/// @brief Sets the font size to a new value
		/// @param[in] value The new font size
		void set_size(FontSize value);

		/// @brief Returns the font colour as an index into the VT colour table
		/// @returns The font colour as an index into the VT colour table
		std::uint8_t get_colour() const;

		/// @brief Sets the colour of the font to a new VT colour
		/// @param[in] value An index into the VT colour table associated to the desired colour
		void set_colour(std::uint8_t value);

		/// @brief Returns the width of the associated font size in pixels
		/// @returns The width of the associated font size in pixels
		std::uint8_t get_font_width_pixels() const;

		/// @brief Returns the height of the associated font size in pixels
		/// @returns The height of the associated font size in pixels
		std::uint8_t get_font_height_pixels() const;

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 8; ///< The fewest bytes of IOP data that can represent this object

		std::uint8_t colour = 0; ///< Text colour
		std::uint8_t size = 0; ///< Font size
		std::uint8_t type = 0; ///< Encoding type
		std::uint8_t style = 0; ///< Font style
	};

	/// @brief Defines a line attributes object, which describes how lines should be displayed on the VT
	class LineAttributes : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			LineColour = 1,
			LineWidth = 2,
			LineArt = 3,

			NumberOfAttributes = 4
		};

		/// @brief Constructor for a line attributes object
		LineAttributes() = default;

		/// @brief Virtual destructor for a line attributes object
		~LineAttributes() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Sets the line art bit pattern. Each bit represents 1 pixel's on/off state.
		/// @returns The line attribute's line art bit pattern
		std::uint16_t get_line_art_bit_pattern() const;

		/// @brief Sets the line art bit patter for the line attribute
		/// @param[in] value The line art bit pattern to set
		void set_line_art_bit_pattern(std::uint16_t value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 8; ///< The fewest bytes of IOP data that can represent this object

		std::uint16_t lineArtBitpattern = 0; ///< Bit pattern art for line. Each bit represents a paintbrush spot
	};

	/// @brief This object holds attributes related to filling output shape objects
	class FillAttributes : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			FillType = 1,
			FillColour = 2,
			FillPattern = 3,

			NumberOfAttributes = 4
		};

		/// @brief Enumerates the different fill types for an object
		enum class FillType : std::uint8_t
		{
			NoFill = 0, ///< No fill will be applied
			FillWithLineColor = 1, ///< Fill with the color of the outline of the shape
			FillWithSpecifiedColorInFillColorAttribute = 2, ///< Fill with the color specified by a fill attribute
			FillWithPatternGivenByFillPatternAttribute = 3 ///< Fill with a patter provided by a fill pattern attribute
		};

		/// @brief Constructor for a fill attributes object
		FillAttributes() = default;

		/// @brief Virtual destructor for a fill attributes object
		~FillAttributes() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the fill pattern associated with this fill attributes object
		/// @returns The fill pattern for this attribute object
		std::uint16_t get_fill_pattern() const;

		/// @brief Sets the fill pattern for this fill attributes object
		/// @param[in] value The fill pattern to set for this object
		void set_fill_pattern(std::uint16_t value);

		/// @brief Returns the fill type/mode associated with this object
		/// @returns The fill type/mode associated with this object
		FillType get_type() const;

		/// @brief Sets the fill type/mode associated with this object
		/// @param[in] value The fill type/mode associated with this object
		void set_type(FillType value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 8; ///< The fewest bytes of IOP data that can represent this object

		std::uint16_t fillPattern = NULL_OBJECT_ID; ///< Object id of a Picture Graphic object to use as a Fill pattern
		FillType type = FillType::NoFill; ///< The fill type/mode associated with this object
	};

	/// @brief This object defines the valid or invalid characters for an Input String object
	class InputAttributes : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			ValidationType = 1,

			NumberOfAttributes = 2
		};

		/// @brief Enumerates the different validation types for this object, which
		/// describe how to interpret the validation string
		enum class ValidationType : std::uint8_t
		{
			ValidCharactersAreListed = 0,
			InvalidCharactersAreListed = 1
		};

		/// @brief Constructor for a input attributes object
		InputAttributes() = default;

		/// @brief Virtual destructor for a input attributes object
		~InputAttributes() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the validation string associated to this input attributes object
		/// @returns The validation string associated to this input attributes object
		std::string get_validation_string() const;

		/// @brief Sets the validation string for this object
		/// @param[in] value The new validation string for this object
		void set_validation_string(const std::string &value);

		/// @brief Returns the validation type setting for this object
		/// @returns The validation type associated to this object
		ValidationType get_validation_type() const;

		/// @brief Sets the validation type setting for this object
		/// @param[in] newValidationType The validation type
		void set_validation_type(ValidationType newValidationType);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 7; ///< The fewest bytes of IOP data that can represent this object

		std::string validationString; ///< String containing all valid or invalid character codes
		ValidationType validationType = ValidationType::ValidCharactersAreListed; ///< Describes how to interpret the validation string
	};

	/// @brief The Extended Input Attributes object, available in VT version 4 and later, defines the valid or invalid
	/// characters for an Input String object
	class ExtendedInputAttributes : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			ValidationType = 1,

			NumberOfAttributes = 2
		};

		/// @brief Enumerates the different validation types for this object, which
		/// describe how to interpret the validation string
		enum class ValidationType : std::uint8_t
		{
			ValidCharactersAreListed = 0,
			InvalidCharactersAreListed = 1
		};

		/// @brief Constructor for an extended input attributes object
		ExtendedInputAttributes() = default;

		/// @brief Virtual destructor for an extended input attributes object
		~ExtendedInputAttributes() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the number of code planes in this extended input attributes
		/// @returns The number of code planes in this extended input attributes
		std::uint8_t get_number_of_code_planes() const;

		/// @brief Sets the number of code planes in this extended input attributes object
		/// @param[in] value The new number of code planes
		void set_number_of_code_planes(std::uint8_t value);

		/// @brief Returns the validation type setting for this object
		/// @returns The validation type associated to this object
		ValidationType get_validation_type() const;

		/// @brief Sets the validation type setting for this object
		/// @param[in] value The validation type
		void set_validation_type(ValidationType value);

		/// @todo Finish ExtendedInputAttributes implementation

	private:
		/// @brief Stores data for a code plane (for utf-16 strings)
		class CodePlane
		{
		public:
			std::vector<std::vector<wchar_t>> characterRanges; ///< A list of character ranges for this code plane
			std::uint8_t numberOfCharacterRanges; ///< The number of expected character ranges for this code plane
		};
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 5; ///< The fewest bytes of IOP data that can represent this object

		std::vector<CodePlane> codePlanes; ///< Code planes to which the character ranges belong.
		ValidationType validationType = ValidationType::ValidCharactersAreListed; ///< Describes how to interpret the validation string
	};

	/// @brief Points to another object
	class ObjectPointer : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			Value = 1,

			NumberOfAttributes = 2
		};

		/// @brief Constructor for a object pointer object
		ObjectPointer() = default;

		/// @brief Virtual destructor for a object pointer object
		~ObjectPointer() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the object id of the object this object points to
		/// @returns The object id of the object this object points to
		std::uint16_t get_value() const;

		/// @brief Sets the object id of the object this object points to.
		/// Does not do error checking on the type of object this object points to.
		/// @param[in] objectIDToPointTo The object id of the object this object points to
		void set_value(std::uint16_t objectIDToPointTo);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 5; ///< The fewest bytes of IOP data that can represent this object
		std::uint16_t value = NULL_OBJECT_ID; ///< Object ID of the object this object points to, or the NULL Object ID if the pointer should not be drawn
	};

	/// @brief The External Object Pointer object, available in VT version 5 and later, allows a Working Set to display
	/// objects that exist in another Working Set’s object pool
	class ExternalObjectPointer : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			DefaultObjectID = 1,
			ExternalReferenceNAMEID = 2,
			ExternalObjectID = 3,

			NumberOfAttributes = 4
		};

		/// @brief Constructor for a object pointer object
		ExternalObjectPointer() = default;

		/// @brief Virtual destructor for a object pointer object
		~ExternalObjectPointer() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the default object id which is the
		/// object ID of an object which shall be displayed if the External Object ID is not valid,
		/// or the NULL Object ID.
		/// @returns The default object ID or the null object ID
		std::uint16_t get_default_object_id() const;

		/// @brief Sets the default object id which is the
		/// object ID of an object which shall be displayed if the External Object ID is not valid,
		/// or the NULL Object ID.
		/// @param[in] id The default object ID or the null object ID
		void set_default_object_id(std::uint16_t id);

		/// @brief Returns the external reference NAME ID
		/// @returns External reference NAME ID
		std::uint16_t get_external_reference_name_id() const;

		/// @brief Sets the external reference NAME ID
		/// @param[in] id External reference NAME ID
		void set_external_reference_name_id(std::uint16_t id);

		/// @brief Returns the external object ID.
		/// The referenced object is found in
		/// the object pool of the Working Set Master
		/// identified by the External Reference NAME
		/// ID attribute and listed in the corresponding
		/// External Object Definition object.
		/// @returns The external object ID.
		std::uint16_t get_external_object_id() const;

		/// @brief Sets the external object ID.
		/// The referenced object is found in
		/// the object pool of the Working Set Master
		/// identified by the External Reference NAME
		/// ID attribute and listed in the corresponding
		/// External Object Definition object.
		/// @param[in] id The external object ID.
		void set_external_object_id(std::uint16_t id);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 5; ///< The fewest bytes of IOP data that can represent this object

		std::uint16_t defaultObjectID = NULL_OBJECT_ID; ///< Object ID of an object which shall be displayed if the External Object ID is not valid, or the NULL Object ID
		std::uint16_t externalReferenceNAMEID = NULL_OBJECT_ID; ///< Object id of an External Reference NAME object or the NULL Object ID
		std::uint16_t externalObjectID = NULL_OBJECT_ID; ///< Object ID of a referenced object or the NULL Object ID
	};

	/// @brief Defines a macro object. Performs a list of commands based on a message or event.
	class Macro : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,

			NumberOfAttributes = 1
		};

		/// @brief A subset of the VT command multiplexors that support use in macros
		enum class Command
		{
			HideShowObject = 0xA0,
			EnableDisableObject = 0xA1,
			SelectInputObject = 0xA2,
			ControlAudioSignal = 0xA3,
			SetAudioVolume = 0xA4,
			ChangeChildLocation = 0xA5,
			ChangeSize = 0xA6,
			ChangeBackgroundColour = 0xA7,
			ChangeNumericValue = 0xA8,
			ChangeEndPoint = 0xA9,
			ChangeFontAttributes = 0xAA,
			ChangeLineAttributes = 0xAB,
			ChangeFillAttributes = 0xAC,
			ChangeActiveMask = 0xAD,
			ChangeSoftKeyMask = 0xAE,
			ChangeAttribute = 0xAF,
			ChangePriority = 0xB0,
			ChangeListItem = 0xB1,
			ChangeStringValue = 0xB3,
			ChangeChildPosition = 0xB4,
			ChangeObjectLabel = 0xB5,
			ChangePolygonPoint = 0xB6,
			LockUnlockMask = 0xBD,
			ExecuteMacro = 0xBE,
			ChangePolygonScale = 0xB7,
			GraphicsContextCommand = 0xB8,
			SelectColourMap = 0xBA,
			ExecuteExtendedMacro = 0xBC
		};

		/// @brief Constructor for a macro object
		Macro() = default;

		/// @brief Virtual destructor for a macro object
		~Macro() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Adds a macro command packet to this macro. Essentially these are CAN messages that represent normal
		/// ECU to VT commands that will be executed in order by this macro.
		/// @param[in] command The command packet (CAN message data) to add
		/// @returns true if the command was added to the macro, otherwise false (maybe the max number of commands has been hit)
		bool add_command_packet(const std::vector<std::uint8_t> &command);

		/// @brief Returns the number of stored command packets inside this macro (max 255)
		/// @returns The number of stored command packets inside this macro
		std::uint8_t get_number_of_commands() const;

		/// @brief Returns a command packet by index
		/// @param[in] index The index of the packet to retrieve
		/// @param[out] command The returned command packet if the return value is true, otherwise the returned
		/// command packet content is undefined.
		/// @returns true if a valid command packet was returned, otherwise false (index out of range)
		bool get_command_packet(std::uint8_t index, std::vector<std::uint8_t> &command);

		/// @brief Deletes a command packet from the macro by index
		/// @param[in] index The index of the packet to delete
		/// @returns true if the specified command packet was removed, otherwise false (index out of range)
		bool remove_command_packet(std::uint8_t index);

		/// @brief Returns if the command packets in this macro are valid
		/// @returns true if the command packets in this macro are valid, otherwise false
		bool get_are_command_packets_valid() const;

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 5; ///< The fewest bytes of IOP data that can represent this object
		static const std::array<std::uint8_t, 28> ALLOWED_COMMANDS_LOOKUP_TABLE; ///< The list of all allowed commands in a table for easy lookup when validating macro content
		std::vector<std::vector<std::uint8_t>> commandPackets; ///< Macro command list
	};

	/// @brief Defines a colour map object. The Colour Map object, optionally available in VT version 4 and 5, and mandatory in VT version 6 and
	/// later, allows the Working Set designer to alter the transformation of the VT colour index values to the
	/// defined RGB value. This provides a mechanism where the colours table can be changed at run-time.
	class ColourMap : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,

			NumberOfAttributes = 1
		};

		/// @brief Constructor for a colour map object
		ColourMap() = default;

		/// @brief Virtual destructor for a colour map object
		~ColourMap() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief This is used to initialize the colour map data to either 2, 16, or 256 colour indexes.
		/// Values will be initialized from the default color table to the colour map data whenever this is called.
		/// @param[in] value The number of colour indexes to initialize the colour map to
		/// @returns true if the number of colour indexes was set, otherwise false (invalid value or value is unchanged)
		bool set_number_of_colour_indexes(std::uint16_t value);

		/// @brief Returns the number of colour indexes in this colour map
		/// @returns The number of colour indexes in this colour map (2, 16, or 256)
		std::uint16_t get_number_of_colour_indexes() const;

		/// @brief Sets the colour map index to the specified value/colour
		/// @param[in] index The index to set
		/// @param[in] value The colour to set the index to
		/// @returns true if the colour map index was set, otherwise false (index out of range)
		bool set_colour_map_index(std::uint8_t index, std::uint8_t value);

		/// @brief Returns the colour index into the VT colour table at the specified index in this colour map
		/// @param[in] index The index in this map to get the VT colour index for
		/// @returns The VT colour index at the specified index in this colour map
		std::uint8_t get_colour_map_index(std::uint8_t index) const;

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 5; ///< The fewest bytes of IOP data that can represent this object
		std::vector<std::uint8_t> colourMapData; ///< The actual colour map data, which remaps each index from the default table based on the size of this vector.
	};

	/// @brief Defines a window mask object
	class WindowMask : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			BackgroundColour = 1,
			Options = 2,
			Name = 3,

			NumberOfAttributes = 4
		};

		/// @brief Enumerates the different kinds of window masks which imply how they are displayed and what they contain
		enum class WindowType : std::uint8_t
		{
			Freeform = 0, ///< the Working Set supplies and positions all child objects contained inside the window. In this case the Working Set has complete control over the look and feel of the window.
			NumericOutputValueWithUnits1x1 = 1, ///< This window displays a single numeric output with units of measure in a single window cell.
			NumericOutputValueNoUnits1x1 = 2, ///< This window displays a single numeric output with no units of measure in a single window cell.
			StringOutputValue1x1 = 3, ///< This window displays a single string output in a single window cell.
			NumericInputValueWithUnits1x1 = 4, ///< This window displays a single numeric input with units of measure in a single window cell
			NumericInputValueNoUnits1x1 = 5, ///< This window displays a single numeric input with no units of measure in a single window cell
			StringInputValue1x1 = 6, ///< This window displays a single string input in a single window cell
			HorizontalLinearBarGraphNoUnits1x1 = 7, ///< This window displays a single horizontal linear bar graph in a single window cell
			SingleButton1x1 = 8, ///< This window displays a single Button object in a single window cell
			DoubleButton1x1 = 9, ///< This window displays two Button objects in a single window cell
			NumericOutputValueWithUnits2x1 = 10, ///< This window displays a single numeric output with units of measure in two horizontal window cells
			NumericOutputValueNoUnits2x1 = 11, ///< This window displays a single numeric output with no units of measure in two horizontal window cells
			StringOutputValue2x1 = 12, ///< This window displays a single string output in two horizontal window cells.
			NumericInputValueWithUnits2x1 = 13, ///< This window displays a single numeric input with units of measure in two horizontal window cells
			NumericInputValueNoUnits2x1 = 14, ///< This window displays a single numeric input with no units of measure in two horizontal window cells
			StringInputValue2x1 = 15, ///< This window displays a single string input in two horizontal window cells.
			HorizontalLinearBarGraphNoUnits2x1 = 16, ///< This window displays a single horizontal linear bar graph in two horizontal window cells
			SingleButton2x1 = 17, ///< This window displays a single Button object in two horizontal window cells
			DoubleButton2x1 = 18 ///< This window displays two Button objects in two horizontal window cells
		};

		/// @brief Enumerates the bit indexes of options encoded in the object's options bitfield
		enum class Options
		{
			Available = 0, ///< If 0 (FALSE) this window is not available for use at the present time, even though defined.
			Transparent = 1 ///< Transparent. If this bit is 1, the background colour attribute shall not be used and the Window shall be transparent.
		};

		/// @brief Constructor for a window mask object
		WindowMask() = default;

		/// @brief Virtual destructor for a window mask object
		~WindowMask() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns object ID of an Output String object or an Object Pointer object that points
		/// to an Output String object that contains the string that gives a proper name to this object
		/// @returns Object ID corresponding to this object's proper name
		std::uint16_t get_name_object_id() const;

		/// @brief Sets the object ID of an Output String object or an Object Pointer object that points
		/// to an Output String object that contains the string that gives a proper name to this object
		/// @param[in] object The object ID that contains the string for this object's proper name
		void set_name_object_id(std::uint16_t object);

		/// @brief Returns Object ID of an Output String object or an Object Pointer
		/// object that points to an Output String object that
		/// contains the string that supplies window title text
		/// @returns Object ID corresponding to this object's window title text
		std::uint16_t get_title_object_id() const;

		/// @brief Sets the Object ID of an Output String object or an Object Pointer
		/// object that points to an Output String object that
		/// contains the string that supplies window title text
		/// @param[in] object The object ID that contains the string for this object's title text
		void set_title_object_id(std::uint16_t object);

		/// @brief Returns the object ID of an output object that contains an icon for the window.
		/// @returns The object ID of an output object that contains an icon for the window.
		std::uint16_t get_icon_object_id() const;

		/// @brief Sets the object ID of an output object that contains an icon for the window.
		/// @param[in] object The object ID of an output object that contains an icon for the window.
		void set_icon_object_id(std::uint16_t object);

		/// @brief Returns the window type for this object
		/// @returns The window type for this object
		WindowType get_window_type() const;

		/// @brief Sets the window type for this object
		/// @param[in] type The window type for this object
		void set_window_type(WindowType type);

		/// @brief Returns the state of a single option in the object's option bitfield
		/// @param[in] option The option to check the value of in the object's option bitfield
		/// @returns The state of the associated option bit
		bool get_option(Options option) const;

		/// @brief Sets the options bitfield for this object to a new value
		/// @param[in] value The new value for the options bitfield
		void set_options(std::uint8_t value);

		/// @brief Sets a single option in the options bitfield to the specified value
		/// @param[in] option The option to set
		/// @param[in] value The new value of the option bit
		void set_option(Options option, bool value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 17; ///< The fewest bytes of IOP data that can represent this object
		std::uint16_t name = NULL_OBJECT_ID; ///< Object ID of an Output String object or an Object Pointer object that points to an Output String object that contains the string that gives a proper name to this object
		std::uint16_t title = NULL_OBJECT_ID; ///< Object ID of an Output String object or an Object Pointer object that points to an Output String object that supplies window title text
		std::uint16_t icon = NULL_OBJECT_ID; ///< Object ID of an Output object or an Object Pointer object that points to an Output object that contains an icon for the window
		std::uint8_t optionsBitfield = 0; ///< Bitfield of options defined in `Options` enum
		std::uint8_t windowType = 0; ///< The window type, which implies its size
	};

	/// @brief Defines an auxiliary function type 1 object
	/// @details The Auxiliary Function Type 1 object defines the function attributes and designator of an Auxiliary Function.
	/// @note This object is parsed and validated but not utilized by version 3 or later VTs in making Auxiliary Control Assignments
	class AuxiliaryFunctionType1 : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,

			NumberOfAttributes = 1
		};

		/// @brief Enumerates the different kinds of auxiliary functions (type 1)
		enum class FunctionType : std::uint8_t
		{
			LatchingBoolean = 0,
			Analogue = 1,
			NonLatchingBoolean = 2
		};

		/// @brief Constructor for a auxiliary function type 1 object
		AuxiliaryFunctionType1() = default;

		/// @brief Virtual destructor for a auxiliary function type 1 object
		~AuxiliaryFunctionType1() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the function type
		/// @returns The function type
		FunctionType get_function_type() const;

		/// @brief Sets the function type
		/// @param[in] type The function type
		void set_function_type(FunctionType type);

	private:
		FunctionType functionType = FunctionType::LatchingBoolean; ///< The function type
	};

	/// @brief Defines an auxiliary function type 2 object
	/// @details The Auxiliary Function Type 2 object defines the function attributes and designator of an Auxiliary Function.
	class AuxiliaryFunctionType2 : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			BackgroundColour = 1,
			FunctionAttributes = 2,

			NumberOfAttributes = 3
		};

		/// @brief Aux inputs must be one of these types, and the input and function types must match.
		/// @details This is table J.5 in ISO 11783-6 (2018)
		enum class FunctionType : std::uint8_t
		{
			BooleanLatchingOnOff = 0, ///< Two-position switch (maintains position) (Single Pole, Double Throw)
			Analouge = 1, ///< Maintains position setting
			BooleanNonLatchingIncreaseValue = 2, ///< Two-position switch (return to off) (Momentary Single Pole, Double Throw)
			AnalougeReturnTo50Percent = 3, ///< Two way analogue (return to centre position)
			AnalougeReturnTo0PercentIncreaseValue = 4, ///< One way analogue input (returns to 0%)
			DualBooleanBothLatching = 5, ///< Three-Position Switch (latching in all positions) (Single Pole, Three Position, Centre Off)
			DualBooleanBothNonLatching = 6, ///< Three-Position Switch, (returning to centre position) (Momentary Single Pole, Three Position, Centre Off)
			DualBooleanLatchingUp = 7, ///< Three-Position Switch, latching in up position, momentary down (Single Pole, Three Position, Centre Off)
			DualBooleanLatchingDown = 8, ///< Three-Position Switch, latching in down position, momentary up (Single Pole, Three Position, Centre Off)
			CombinedAnalougeReturnTo50PercentWithDualBooleanLatching = 9, ///< Two way analogue (return to centre position) with latching Boolean at 0% and 100% positions
			CombinedAnalougeMaintainsPositionWithDualBooleanLatching = 10, ///< Analogue maintains position setting with latching Boolean at 0% and 100% positions
			QuadratureBooleanNonLatching = 11, ///< Two quadrature mounted Three-Position Switches, (returning to centre position) (Momentary Single Pole, Three Position, Centre Off)
			QuadratureAnalouge = 12, ///< Two quadrature mounted analogue maintain position setting. The centre position of each analogue axis is at 50 % value
			QuadratureAnalougeReturnTo50Percent = 13, ///< Two quadrature mounted analogue returns to centre position (The centre position of each analogue axis is at 50 %)
			BidirectionalEncoder = 14, ///< Count increases when turning in the encoders "increase" direction and count decreases when turning in the opposite direction
			ReservedRangeStart = 15, ///< Reserved for future use
			ReservedRangeEnd = 31 ///< Used for Remove assignment command
		};

		/// @brief Enumerates bit offsets of attributes of auxiliary functions to be assigned to an input control
		enum FunctionAttribute
		{
			CriticalControl = 5, ///< If this bit is 1, This function can only be controlled by a critical Auxiliary Input (see ISO 15077)
			AssignmentRestriction = 6, ///< If this bit is 1, This function, if assigned, can only be assigned as specified in the Preferred Assignment command
			SingleAssignment = 7, ///< If 1, Function shall not be assigned with other Auxiliary Functions to same input. Otherwise it can be assigned with other functions to the same input
		};

		/// @brief Constructor for a auxiliary function type 2 object
		AuxiliaryFunctionType2() = default;

		/// @brief Virtual destructor for a auxiliary function type 2 object
		~AuxiliaryFunctionType2() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the function type
		/// @returns The function type
		FunctionType get_function_type() const;

		/// @brief Sets the function type
		/// @param[in] type The function type
		void set_function_type(FunctionType type);

		/// @brief returns the value of a specified function attribute
		/// @param[in] attributeToCheck The function attribute to check
		/// @returns The value of a specified function attribute
		bool get_function_attribute(FunctionAttribute attributeToCheck) const;

		/// @brief Sets the value of a specified function attribute
		/// @param[in] attributeToSet The function attribute to set
		/// @param[in] value The value to set the function attribute to
		void set_function_attribute(FunctionAttribute attributeToSet, bool value);

	private:
		std::uint8_t functionAttributesBitfield = 0; ///< Bitfield of function attributes defined in `FunctionAttribute` enum plus the `FunctionType`
	};

	/// @brief Defines an auxiliary input type 1 object
	/// @details The Auxiliary Input Type 1 object defines the designator, the key, switch or dial number and the function
	/// type for an Auxiliary Input.
	/// @note This object is parsed and validated but not utilized by version 3 or later VTs in making Auxiliary Control Assignments
	class AuxiliaryInputType1 : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,

			NumberOfAttributes = 1
		};

		/// @brief Enumerates the different kinds of auxiliary functions (type 1)
		enum class FunctionType : std::uint8_t
		{
			LatchingBoolean = 0,
			Analogue = 1,
			NonLatchingBoolean = 2
		};

		/// @brief Constructor for a auxiliary input type 1 object
		AuxiliaryInputType1() = default;

		/// @brief Virtual destructor for a auxiliary input type 1 object
		~AuxiliaryInputType1() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the function type
		/// @returns The function type
		FunctionType get_function_type() const;

		/// @brief Sets the function type
		/// @param[in] type The function type
		void set_function_type(FunctionType type);

		/// @brief Returns the identification number of the input. Maximum value is 250.
		/// @details This number is used by the Auxiliary Input units to identify a
		/// particular input when sending an Auxiliary Input status message.
		/// @returns The identification number of the input
		std::uint8_t get_input_id() const;

		/// @brief Sets the identification number of the input. Maximum value is 250.
		/// @details This number is used by the Auxiliary Input units to identify a
		/// particular input when sending an Auxiliary Input status message.
		/// @param[in] id The identification number of the input
		/// @returns true if the identification number was set, otherwise false (value was >250)
		bool set_input_id(std::uint8_t id);

	private:
		FunctionType functionType = FunctionType::LatchingBoolean; ///< The function type
		std::uint8_t inputID = 0; ///< The identification number of the input. This number is used by the Auxiliary Input units to identify a particular input when sending an Auxiliary Input status message.
	};

	/// @brief Defines an auxiliary input type 2 object
	class AuxiliaryInputType2 : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			BackgroundColour = 1,
			FunctionAttributes = 2,

			NumberOfAttributes = 3
		};

		/// @brief Enumerates bit offsets of attributes of auxiliary inputs
		enum FunctionAttribute
		{
			CriticalControl = 5, ///< If this bit is 1, This input can control a critical (auxiliary) function
			AssignmentRestriction = 6, ///< Reserved, set to 0
			SingleAssignment = 7, ///< If 1, Input shall only be assigned to a single Auxiliary Function
		};

		/// @brief Constructor for a auxiliary input type 2 object
		AuxiliaryInputType2() = default;

		/// @brief Virtual destructor for a auxiliary input type 2 object
		~AuxiliaryInputType2() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the type of input function that the input control performs when assigned
		/// @returns The type of input function that the input control performs when assigned
		AuxiliaryFunctionType2::FunctionType get_function_type() const;

		/// @brief Sets the type of input function that the input control performs when assigned
		/// @param[in] type The type of input function that the input control performs when assigned
		void set_function_type(AuxiliaryFunctionType2::FunctionType type);

		/// @brief returns the value of a specified function attribute
		/// @param[in] attributeToCheck The function attribute to check
		/// @returns The value of a specified function attribute
		bool get_function_attribute(FunctionAttribute attributeToCheck) const;

		/// @brief Sets the value of a specified function attribute
		/// @param[in] attributeToSet The function attribute to set
		/// @param[in] value The value to set the function attribute to
		void set_function_attribute(FunctionAttribute attributeToSet, bool value);

	private:
		std::uint8_t functionAttributesBitfield = 0; ///< Bitfield of function attributes defined in `FunctionAttribute` enum plus the `FunctionType`
	};

	/// @brief Defines an auxiliary control designator type 2 object.
	/// Auxiliary Control Designator Type 2 Object Pointers allow the Working Set to place Auxiliary Input
	/// Type 2 and Auxiliary Function Type 2 designators in the Data Mask at Working Set defined coordinates.
	class AuxiliaryControlDesignatorType2 : public VTObject
	{
	public:
		/// @brief Enumerates this object's attributes which are assigned an attribute ID.
		/// The Change Attribute command allows any writable attribute with an AID to be changed.
		enum class AttributeName : std::uint8_t
		{
			Type = 0,
			PointerType = 1,
			AuxiliaryObjectID = 2,

			NumberOfAttributes = 3
		};

		/// @brief Constructor for a auxiliary control designator type 2 object
		AuxiliaryControlDesignatorType2() = default;

		/// @brief Virtual destructor for a auxiliary control designator type 2 object
		~AuxiliaryControlDesignatorType2() override = default;

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_length() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @param[in] objectPool The object pool to use when validating the object
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool) const override;

		/// @brief Sets an attribute and optionally returns an error code in the last parameter
		/// @param[in] attributeID The ID of the attribute to change
		/// @param[in] rawAttributeData The raw data to change the attribute to, as decoded in little endian format with unused
		/// bytes/bits set to zero.
		/// @param[in] objectPool The object pool to use when validating the objects affected by setting this attribute
		/// @param[out] returnedError If this function returns false, this will be the error code. If the function
		/// returns true, this value is undefined.
		/// @returns True if the attribute was changed, otherwise false (check the returnedError in this case to know why).
		bool set_attribute(std::uint8_t attributeID, std::uint32_t rawAttributeData, const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectPool, AttributeError &returnedError) override;

		/// @brief Gets an attribute and returns the raw data in the last parameter
		/// @param[in] attributeID The ID of the attribute to get
		/// @param[out] returnedAttributeData The raw data of the attribute, as decoded in little endian format with unused
		/// bytes/bits set to zero. You may need to cast this to the correct type. If this function
		/// returns false, this value is undefined.
		/// @returns True if the attribute was retrieved, otherwise false (the attribute ID was invalid)
		bool get_attribute(std::uint8_t attributeID, std::uint32_t &returnedAttributeData) const override;

		/// @brief Returns the object ID of the referenced auxiliary object or the null object ID.
		/// Used in conjunction with the pointer type.
		/// @returns The object ID of the referenced auxiliary object or the null object ID
		std::uint16_t get_auxiliary_object_id() const;

		/// @brief Sets the object ID of the referenced auxiliary object
		/// Used in conjunction with the pointer type.
		/// @param[in] id The object ID of the referenced auxiliary object or the null object ID
		void set_auxiliary_object_id(std::uint16_t id);

		/// @brief Returns the pointer type, which describes how this object should be rendered
		/// @details If the pointer type is 0 or 2, the pointer points to Auxiliary Object referenced in the auxiliaryObjectID, or the working set object
		/// and the VT shall display that auxiliary object designator (pointer type 0) or Working Set designator (pointer type 2).
		/// If the Auxiliary Control designator Object Pointer is of pointer type 1 or 3, then this pointer references
		/// Auxiliary Object(s) that have an assignment relationship to the object referenced by the auxiliary object
		/// attribute within this object pool.The VT shall display the assigned auxiliary object designator (pointer type 1) or
		/// its Working Set designator (pointer type 3).
		/// If the pointer type is 1, the pointer points to
		/// @returns The pointer type, which describes how this object should be rendered
		std::uint8_t get_pointer_type() const;

		/// @brief Sets the pointer type which describes how this object should be rendered
		/// @param[in] type The pointer type, which describes how this object should be rendered
		void set_pointer_type(std::uint8_t type);

	private:
		std::uint16_t auxiliaryObjectID = NULL_OBJECT_ID; ///< Object ID of a referenced Auxiliary Function or Auxiliary Input object or NULL_OBJECT_ID
		std::uint8_t pointerType = 0; ///< The pointer type, defines how this should be rendered
	};

	template<typename T>
	/// @brief A specialized replacement for std::to_string
	/// @param object_id An ID of an IsoBus object
	/// @returns in the case if the object_id is 65535 (NULL object ID) returns "NULL" otherwise it returns the number as string
	std::string object_id_to_string(T const &object_id)
	{
		if (isobus::NULL_OBJECT_ID == object_id)
		{
			return "NULL";
		}
		std::ostringstream oss;
		oss << object_id;
		return oss.str();
	}
} // namespace isobus

#endif // ISOBUS_VIRTUAL_TERMINAL_OBJECTS_HPP
