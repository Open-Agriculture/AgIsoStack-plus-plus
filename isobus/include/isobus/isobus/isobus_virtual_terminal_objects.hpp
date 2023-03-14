//================================================================================================
/// @file isobus_virtual_terminal_objects.hpp
///
/// @brief Defines the different VT object types that can comprise a VT object pool.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef ISOBUS_VIRTUAL_TERMINAL_OBJECTS_HPP
#define ISOBUS_VIRTUAL_TERMINAL_OBJECTS_HPP

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace isobus
{
	/// @brief The types of objects in an object pool by object type byte value
	enum class VirtualTerminalObjectType
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

	static constexpr std::uint16_t NULL_OBJECT_ID = 0xFFFF; ///< The NULL Object ID, usually drawn as blank space

	/// @brief Generic VT object base class
	class VTObject
	{
	public:
		/// @brief Constructor for a generic VT object. Sets up default values and the pointer to the member object pool
		/// @param[in] memberObjectPool a pointer to the object tree that this object will be a member of
		explicit VTObject(std::map<std::uint16_t, VTObject *> *memberObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		virtual VirtualTerminalObjectType get_object_type() const = 0;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		virtual std::uint32_t get_minumum_object_lenth() const = 0;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		virtual bool get_is_valid() const = 0;

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

		/// @brief Returns a VT object from its member pool by ID, or the null id if it does not exist
		/// @returns The object with the corresponding ID, or the null object ID if it was not located
		VTObject *get_object_by_id(std::uint16_t objectID) const;

		/// @brief Returns the number of child objects within this object
		std::uint16_t get_number_children() const;

		/// @brief Adds an object as a child to another object, which essentially creates a tree of object association
		/// @param[in] objectID The object ID of the child to add
		/// @param[in] relativeXLocation The X offset of this object to its parent
		/// @param[in] relativeYLocation The Y offset of this object to its parent
		void add_child(std::uint16_t objectID, std::int16_t relativeXLocation, std::int16_t relativeYLocation);

		/// @brief Returns the ID of the child by index, if one was added previously
		/// @note NULL_OBJECT_ID is a valid child, so you should always check the number of children to know if the return value of this is "valid"
		/// @returns The ID of the child at the specified index, or NULL_OBJECT_ID if the index is out of range
		std::uint16_t get_child_id(std::uint16_t index) const;

		/// @brief Returns the X offset of the child object associated with the specified index into the parent object
		/// @returns The relative X position of the child, and always 0 if the index is out of range
		std::int16_t get_child_x(std::uint16_t index) const;

		/// @brief Returns the Y offset of the child object associated with the specified index into the parent object
		/// @returns The relative Y position of the child, and always 0 if the index is out of range
		std::int16_t get_child_y(std::uint16_t index) const;

	protected:
		/// @brief Storage for child object data
		class ChildObjectData
		{
		public:
			/// @brief Default constructor for child object data with default values
			ChildObjectData();

			/// @brief Constructor that initializes all members with parameters
			/// @param[in] objectId The object ID of this child object
			/// @param[in] x The x location of this child relative to the parent object
			/// @param[in] y The y location of this child relative to the parent object
			ChildObjectData(std::uint16_t objectId,
			                std::int16_t x,
			                std::int16_t y);
			std::uint16_t id; ///< Object identifier. Shall be unique within the object pool.
			std::int16_t xLocation; ///< Relative X location of the top left corner of the object
			std::int16_t yLocation; ///< Relative Y location of the top left corner of the object
		};

		std::map<std::uint16_t, VTObject *> *thisObjectPool; ///< A pointer to the rest of the object pool. Convenient for lookups by object ID.
		std::vector<ChildObjectData> children; ///< List of child objects
		std::uint16_t objectID = NULL_OBJECT_ID; ///< Object identifier. Shall be unique within the object pool.
		std::uint16_t width = 0; ///< The width of the object. Not always applicable, but often used.
		std::uint16_t height = 0; ///< The height of the object. Not always applicable, but often used.
		std::uint8_t backgroundColor = 0; ///< The background color (from the VT colour table)
	};

	/// @brief This object shall include one or more objects that fit inside a Soft Key designator for use as an
	/// identification of the Working Set.
	class WorkingSet : public VTObject
	{
	public:
		/// @brief Constructor for a working set object
		/// @param[in] parentObjectPool A pointer to the object pool this object is a member of
		explicit WorkingSet(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

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
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 18; ///< The fewest bytes of IOP data that can represent this object

		std::vector<std::string> languageCodes; ///< A list of 2 character language codes, like "en"
		std::uint16_t activeMask; ///< The currently active mask for this working set
		bool selectable; ///< If this working set is selectable right now
	};

	/// @brief The Data Mask describes the objects that will appear in the Data Mask area of the physical display.
	class DataMask : public VTObject
	{
	public:
		/// @brief Constructor for a data mask object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit DataMask(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 12; ///< The fewest bytes of IOP data that can represent this object

		std::uint16_t softKeyMask; ///< The object ID of a soft key mask, or the null object ID if none is to be rendered
	};

	/// @brief Similar to a data mask, but takes priority and will be shown over data masks.
	class AlarmMask : public VTObject
	{
	public:
		/// @brief Enumerates the different mask priorities. Higher priority masks will be shown over lower priority ones across all working sets.
		enum class Priority : std::uint8_t
		{
			High = 0, ///< High, operator is in danger or urgent machine malfunction
			Medium = 1, ///< Medium, normal alarm, machine is malfunctioning
			Low = 2 ///< Low, information only
		};

		/// @brief Enumerates the acoustic signal values for the alarm mask. Works only if your VT has a way to make sounds.
		/// @details The result of this setting is somewhat proprietary depending on your VT
		enum AcousticSignal : std::uint8_t
		{
			Highest = 0, ///< Most aggressive beeping
			Medium = 1, ///< Medium beeping
			Lowest = 3, ///< Low beeping
			None = 4 ///< No beeping
		};

		/// @brief Constructor for a alarm mask object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit AlarmMask(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

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

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 10; ///< The fewest bytes of IOP data that can represent this object

		std::uint16_t softKeyMask; ///< Object ID of a soft key mask for this alarm mask, or the null ID
		Priority maskPriority; ///< The priority of this mask
		AcousticSignal signalPriority; ///< The acoustic signal priority for this mask
	};

	/// @brief The Container object is used to group objects for the purpose of moving, hiding or sharing the group.
	/// @details A container is not a visible object, only a logical grouping of other objects. Unlike masks, containers can be
	/// hidden and shown at run-time
	class Container : public VTObject
	{
	public:
		/// @brief Constructor for a container object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit Container(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

		/// @brief Returns the "hidden" attribute for this container
		/// @returns true if the hidden attribute is set, otherwise `false`
		bool get_hidden() const;

		/// @brief Sets the "hidden" attribute for this container
		/// @param[in] value The new attribute state
		void set_hidden(bool value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 9; ///< The fewest bytes of IOP data that can represent this object

		bool hidden; ///< The hidden attribute state for this container object. True means it will be hidden when rendered.
	};

	/// @brief The Soft Key Mask is a Container object that contains Key objects, Object Pointer objects, or External Object
	/// Pointer objects.
	/// @details Keys are assigned to physical Soft Keys in the order listed. It is allowable for a Soft Key Mask to
	/// contain no Keys in order that all Soft Keys are effectively disabled when this mask is activated
	class SoftKeyMask : public VTObject
	{
	public:
		/// @brief Constructor for a soft key mask object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit SoftKeyMask(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 6; ///< The fewest bytes of IOP data that can represent this object
	};

	/// @brief The Key object defines the designator and key code for a Soft Key. Any object located outside of a Soft Key
	/// designator is clipped.
	class Key : public VTObject
	{
	public:
		/// @brief Constructor for a key object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit Key(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

		/// @brief Returns the key code associated to this key object
		/// @returns The key code associated to this key object
		std::uint8_t get_key_code() const;

		/// @brief Sets the key code associated to this key object
		/// @param[in] value The key code to set
		void set_key_code(std::uint8_t value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 7; ///< The fewest bytes of IOP data that can represent this object

		std::uint8_t keyCode; ///< They key code associated with events from this key object
	};

	/// @brief The Key objects contained in this object shall be a grouping of Key objects, or Object Pointers to Key objects
	class KeyGroup : public VTObject
	{
	public:
		/// @brief Enumerates the options bits in the options bitfield of a KeyGroup
		enum class Options : std::uint8_t
		{
			Available = 0, ///< If 0 (FALSE) this object is not available for use at the present time, even though defined
			Transparent = 1 ///< If this bit is 1, the VT shall	ignore the background colour attribute in all child Key objects
		};

		/// @brief Constructor for a key group object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit KeyGroup(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

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

		static constexpr std::uint8_t MAX_CHILD_KEYS = 4; ///< There shall be a max of 4 keys per group according to the standard

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 10; ///< The fewest bytes of IOP data that can represent this object

		std::uint16_t keyGroupIcon; ///< The VT may use	this in the proprietary mapping screen to represent the key group
		std::uint8_t optionsBitfield; ///< Bitfield of options defined in `Options` enum
	};

	/// @brief The Button object defines a button control.
	/// @details This object is intended mainly for VTs with touch screens or a
	/// pointing method but shall be supported by all VTs.
	class Button : public VTObject
	{
	public:
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
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit Button(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

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
		void set_options(std::uint8_t value);

		/// @brief Sets a single option in the options bitfield to the specified value
		/// @param[in] option The option to set
		/// @param[in] value The new value of the option bit
		void set_option(Options option, bool value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 13; ///< The fewest bytes of IOP data that can represent this object

		std::uint8_t borderColour; ///< Border colour.
		std::uint8_t keyCode; ///< Key code assigned by ECU. VT reports this code in the Button Activation message.
		std::uint8_t optionsBitfield; ///< Bitfield of options defined in `Options` enum
	};

	/// @brief The Input Boolean object is used to input a TRUE/FALSE type indication from the operator
	class InputBoolean : public VTObject
	{
	public:
		/// @brief Constructor for an input boolean object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit InputBoolean(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

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
		/// @param[in] value The new state for the enabled attribute for this object
		void set_enabled(bool value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 13; ///< The fewest bytes of IOP data that can represent this object

		std::uint8_t value; ///< Used only if it has no number variable child object
		bool enabled; ///< If the bool is interactable
	};

	/// @brief This object is used to input a character string from the operator
	class InputString : public VTObject
	{
	public:
		/// @brief Options that can be applied to the input string
		enum class Options : std::uint8_t
		{
			Transparent = 0, ///< If TRUE, the input field is displayed with background showing through instead of using the background colour
			AutoWrap = 1, ///< Auto-Wrapping rules apply
			WrapOnHyphen = 2 ///< If TRUE, Auto-Wrapping can occur between a hyphen and the following character.
		};

		/// @brief The allowable horizontal justification options
		enum class HorizontalJustification : std::uint8_t
		{
			PositionLeft = 0, ///< The input string is horizontally justified to the left side of its bounding box
			PositionMiddle = 1, ///< The input string is horizontally justified to the center of its bounding box
			PositionRight = 2, ///< The input string is horizontally justified to the right side of its bounding box
			Reserved = 3 ///< Reserved
		};

		/// @brief The allowable vertical justification options
		enum class VerticalJustification : std::uint8_t
		{
			PositionTop = 0, ///< The input string is vertically justified to the top of its bounding box
			PositionMiddle = 1, ///< The input string is vertically justified to the center of its bounding box
			PositionBottom = 2, ///< The input string is vertically justified to the bottom of its bounding box
			Reserved = 3 ///< Reserved
		};

		/// @brief Constructor for a input string object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit InputString(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

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

		/// @brief Sets the options bitfield for this object to a new value
		/// @param[in] value The new value for the options bitfield
		void set_options(std::uint8_t value);

		/// @brief Sets a single option in the options bitfield to the specified value
		/// @param[in] option The option to set
		/// @param[in] value The new value of the option bit
		void set_option(Options option, bool value);

		/// @brief Returns the horizontal justification setting of the string
		/// @returns The horizontal justification setting of the string
		HorizontalJustification get_horizontal_justification() const;

		/// @brief Returns the vertical justification setting of the string
		/// @returns The vertical justification setting of the string
		VerticalJustification get_vertical_justification() const;

		/// @brief Sets the justification bitfield of the string
		/// @param[in] value The justification bitfield to set
		void set_justification_bitfield(std::uint8_t value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 19; ///< The fewest bytes of IOP data that can represent this object

		std::string stringValue; ///< The actual string. Used only if variable reference attribute is NULL. Pad with spaces as necessary to satisfy length attribute.
		std::uint8_t optionsBitfield; ///< Bitfield of options defined in `Options` enum
		std::uint8_t justificationBitfield; ///< Bitfield of justification options
		std::uint8_t length; ///< Maximum fixed length of the Input String object value in bytes. This may be set to 0 if a variable reference is used
		bool enabled; ///< If the string is interactable
	};

	/// @brief This object is used to format, display and change a numeric value based on a supplied integer value.
	/// @details Displayed value = (value attribute + Offset) * Scaling Factor
	class InputNumber : public VTObject
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

		/// @brief More options, for some reason they are different bytes
		enum class Options2 : std::uint8_t
		{
			Enabled = 0, ///< If TRUE the object shall be enabled
			RealTimeEditing = 1 ///< If TRUE the value shall be transmitted to the ECU as it is being changed
		};

		/// @brief The allowable horizontal justification options
		enum class HorizontalJustification : std::uint8_t
		{
			PositionLeft = 0, ///< The input number is horizontally justified to the left side of its bounding box
			PositionMiddle = 1, ///< The input number is horizontally justified to the center of its bounding box
			PositionRight = 2, ///< The input number is horizontally justified to the right side of its bounding box
			Reserved = 3 ///< Reserved
		};

		/// @brief The allowable vertical justification options
		enum class VerticalJustification : std::uint8_t
		{
			PositionTop = 0, ///< The input number is vertically justified to the top of its bounding box
			PositionMiddle = 1, ///< The input number is vertically justified to the center of its bounding box
			PositionBottom = 2, ///< The input number is vertically justified to the bottom of its bounding box
			Reserved = 3 ///< Reserved
		};

		/// @brief Constructor for an input number object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit InputNumber(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

		/// @brief Returns the horizontal justification setting of the input number
		/// @returns The horizontal justification setting of the input number
		HorizontalJustification get_horizontal_justification() const;

		/// @brief Returns the vertical justification setting of the input number
		/// @returns The vertical justification setting of the input number
		VerticalJustification get_vertical_justification() const;

		/// @brief Sets the justification bitfield of the input number
		/// @param[in] value The justification bitfield to set
		void set_justification_bitfield(std::uint8_t value);

		/// @brief Returns the scale factor that is applied to the value of the input number
		/// @returns The scale factor that is applied to the value of the input number
		float get_scale() const;

		/// @brief Sets the scale factor that is applied to the value of the input number
		/// @param[in] value The scale factor to set
		void set_scale(float value);

		/// @brief Returns the maximum value for the input number
		/// @details The VT shall not accept values higher than this for this input number's value
		/// @returns The maximum value for the input number
		std::uint32_t get_maximum_value() const;

		/// @brief Sets the maximum value for the input number
		/// @details The VT shall not accept values higher than this for this input number's value
		/// @param[in] value The maximum value for the input number
		void set_maximum_value(std::uint32_t value);

		/// @brief Returns the minimum value for this input number
		/// @details The VT shall not accept values smaller than this value for this input number
		/// @returns The minimum value for this input number
		std::uint32_t get_minimum_value() const;

		/// @brief Sets the minimum value for the input number
		/// @details The VT shall not accept values smaller than this value for this input number
		/// @param[in] value The minimum value to set for the input number
		void set_minimum_value(std::uint32_t value);

		/// @brief Returns the offset that will be applied to the number's value when it is displayed
		/// @returns The offset that will be applied to the number's value when it is displayed
		std::int32_t get_offset() const;

		/// @brief Sets the offset that will be applied to the number's value when it is displayed
		/// @param[in] value The new offset that will be applied to the number's value when it is displayed
		void set_offset(std::int32_t value);

		/// @brief Returns the number of decimals to display when rendering this input number
		/// @returns The number of decimals to display when rendering the input number
		std::uint8_t get_number_of_decimals() const;

		/// @brief Sets the number of decimals to display when rendering this number
		/// @param[in] value The number of decimals to display
		void set_number_of_decimals(std::uint8_t value);

		/// @brief Returns if the format option is set for this input number
		/// @details The format option determines if the value is shown in fixed decimal or exponential form.
		/// A value of `true` means fixed decimal (####.nn), and `false` means exponential ([−]###.nnE[+/−]##)
		/// @returns `true` if the format option is set for this input number, otherwise `false`
		bool get_format() const;

		/// @brief Sets the format option
		/// @details The format option determines if the value is shown in fixed decimal or exponential form.
		/// A value of `true` means fixed decimal (####.nn), and `false` means exponential ([−]###.nnE[+/−]##)
		/// @param[in] value The format value to set. `true` for fixed decimal, false for exponential.
		void set_format(bool value);

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

		/// @brief Returns the state of a single option in the object's second option bitfield
		/// @param[in] option The option to check the value of in the object's second option bitfield
		/// @returns The state of the associated option bit
		bool get_option2(Options option) const;

		/// @brief Sets the second options bitfield for this object to a new value
		/// @param[in] value The new value for the second options bitfield
		void set_options2(std::uint8_t value);

		/// @brief Sets a single option in the second options bitfield to the specified value
		/// @param[in] option The option to set
		/// @param[in] value The new value of the option bit
		void set_option2(Options option, bool value);

		/// @brief Returns the value of the input number (only matters if there's no child number variable object).
		/// @returns The value of the input number
		std::uint32_t get_value() const;

		/// @brief Sets the value of the input number (only matters if there's no child number variable object).
		/// @param[in] inputValue The value to set for the input number
		void set_value(std::uint32_t inputValue);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 38; ///< The fewest bytes of IOP data that can represent this object

		float scale; ///< Scale to be applied to the input value and min/max values.
		std::uint32_t maximumValue; ///< Raw maximum value for the input
		std::uint32_t minimumValue; ///< Raw minimum value for the input before scaling
		std::uint32_t value; ///< The raw value of the object, used if no number variable child has been set
		std::int32_t offset; ///< Offset to be applied to the input value and min/max values
		std::uint8_t numberOfDecimals; ///< Specifies number of decimals to display after the decimal point
		std::uint8_t options; ///< Options byte 1
		std::uint8_t options2; ///< Options byte 2
		std::uint8_t justificationBitfield; ///< Indicates how the number is positioned in the field defined by height and width
		bool format; ///< 0 = use fixed format decimal display (####.nn), 1 = use exponential format ([-]###.nnE[+/-]##) where n is set by the number of decimals
	};

	/// @brief The Input List object is used to show one object out of a set of objects,
	/// and to allow operator selection of one object from the set.
	class InputList : public VTObject
	{
	public:
		/// @brief Enumerates the bits in the options bitfield for an InputList
		enum class Options
		{
			Enabled = 0, ///< If true the object shall be enabled
			RealTimeEditing = 1 ///< If true the value shall be transmitted to the ECU as it is being changed
		};

		/// @brief Constructor for an input list object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit InputList(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

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

		/// @brief Returns the value of the selected list index (only matters if there is no child number variable)
		/// @returns The value of the selected list index
		std::uint8_t get_value() const;

		/// @brief Sets the selected list index (only matters when the object has no child number variable)
		/// @param[in] inputValue The new value for the selected list index
		void set_value(std::uint8_t inputValue);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 13; ///< The fewest bytes of IOP data that can represent this object

		std::uint8_t numberOfListItems; ///< Number of object references to follow. The size of the list can never exceed this number and this attribute cannot be changed.
		std::uint8_t optionsBitfield; ///< Options byte
		std::uint8_t value; ///< Selected list index of this object. Used only if variable reference attribute is NULL
	};

	/// @brief This object is used to output a string of text
	class OutputString : public VTObject
	{
	public:
		/// @brief Enumerates the option bits in the options bitfield for an output string
		enum class Options
		{
			Transparent = 0, ///< If TRUE, the output field is displayed with background showing through instead of using the background colour
			AutoWrap = 1, ///< Auto-Wrapping rules apply
			WrapOnHyphen = 2 ///< If TRUE, Auto-Wrapping can occur between a hyphen and the next character
		};

		/// @brief The allowable horizontal justification options
		enum class HorizontalJustification : std::uint8_t
		{
			PositionLeft = 0, ///< Output string is horizontally aligned to the left of its bounding box
			PositionMiddle = 1, ///< Output string is horizontally aligned to the center of its bounding box
			PositionRight = 2, ///< Output string is horizontally aligned to the right of its bounding box
			Reserved = 3 ///< Reserved
		};

		/// @brief The allowable vertical justification options
		enum class VerticalJustification : std::uint8_t
		{
			PositionTop = 0, ///< Output string is vertically aligned to the top of its bounding box
			PositionMiddle = 1, ///< Output string is vertically aligned to the center of its bounding box
			PositionBottom = 2, ///< Output string is vertically aligned to the bottom of its bounding box
			Reserved = 3 ///< Reserved
		};

		/// @brief Constructor for an output string object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit OutputString(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

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

		/// @brief Returns the horizontal justification of the output string within its bounding box
		/// @returns The horizontal justification of the output string within its bounding box
		HorizontalJustification get_horizontal_justification() const;

		/// @brief Returns the vertical justification of the output string within its bounding box
		/// @returns The vertical justification of the output string within its bounding box
		VerticalJustification get_vertical_justification() const;

		/// @brief Sets the justification bitfield for the object to a new value
		/// @param[in] value The new value for the justification bitfield
		void set_justification_bitfield(std::uint8_t value);

		/// @brief Returns the value of the string, used only if the variable reference (a child var string) is NULL_OBJECT_ID
		/// @returns The value of the string
		std::string get_value() const;

		/// @brief Sets the value of the string (only matters if it has no child string variable)
		/// @param[in] value The new value for the string
		void set_value(std::string value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 16; ///< The fewest bytes of IOP data that can represent this object

		std::string stringValue; ///< The actual string. Used only if variable reference attribute is NULL. Pad with spaces as necessary to satisfy length attribute.
		std::uint8_t optionsBitfield; ///< Bitfield of options defined in `Options` enum
		std::uint8_t justificationBitfield; ///< Bitfield of justification options
		std::uint8_t length; ///< Maximum fixed length of the Input String object value in bytes. This may be set to 0 if a variable reference is used
	};

	/// @brief This object is used to format and output a numeric value based on a supplied integer value.
	class OutputNumber : public VTObject
	{
	public:
		/// @brief Options that can be applied to the input number
		enum class Options : std::uint8_t
		{
			Transparent = 0, ///< If true, the input field is displayed with background showing through instead of using the background colour
			DisplayLeadingZeros = 1, ///< If true, fill left to width of field with zeros; justification is applied after filling
			DisplayZeroAsBlank = 2, ///< When this option bit is set, a blank field is displayed if and only if the displayed value of the object is exactly zero
			Truncate = 3 ///< If true the value shall be truncated to the specified number of decimals. Otherwise it shall be rounded off to the specified number of decimals.
		};

		/// @brief The allowable horizontal justification options
		enum class HorizontalJustification : std::uint8_t
		{
			PositionLeft = 0, ///< The output number is horizontally justified to the left side of its bounding box
			PositionMiddle = 1, ///< The output number is horizontally justified to the center of its bounding box
			PositionRight = 2, ///< The output number is horizontally justified to the right side of its bounding box
			Reserved = 3 ///< Reserved
		};

		/// @brief The allowable vertical justification options
		enum class VerticalJustification : std::uint8_t
		{
			PositionTop = 0, ///< The output number is vertically justified to the top of its bounding box
			PositionMiddle = 1, ///< The output number is vertically justified to the center of its bounding box
			PositionBottom = 2, ///< The output number is vertically justified to the bottom of its bounding box
			Reserved = 3 ///< Reserved
		};

		/// @brief Constructor for an output number object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		OutputNumber(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

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

		/// @brief Returns the horizontal justification of the output number within its bounding box
		/// @return The horizontal justification of the output number within its bounding box
		HorizontalJustification get_horizontal_justification() const;

		/// @brief Returns the vertical justification of the output number within its bounding box
		/// @return The vertical justification of the output number within its bounding box
		VerticalJustification get_vertical_justification() const;

		/// @brief Sets the justification bitfield to a new value
		/// @param[in] value The new value for the justification bitfield
		void set_justification_bitfield(std::uint8_t value);

		/// @brief Returns the scale factor of the output number
		/// @returns The scale factor of the output number
		float get_scale() const;

		/// @brief Sets the scale factor for the output number
		/// @param[in] value The new value for the scale factor
		void set_scale(float value);

		/// @brief Returns the offset that is applied to the output number
		/// @returns The offset of the output number
		std::int32_t get_offset() const;

		/// @brief Sets the offset of the output number
		/// @param[in] value The offset to set for the output number
		void set_offset(std::int32_t value);

		/// @brief Returns the number of decimals to render in the output number
		/// @returns The number of decimals to render in the output number
		std::uint8_t get_number_of_decimals() const;

		/// @brief Sets the number of decimals to render in the output number
		/// @param[in] value The number of decimals to render in the output number
		void set_number_of_decimals(std::uint8_t value);

		/// @brief Returns if the "format" option is set for this object
		/// @details The format option determines if fixed decimal or exponential notation is used.
		/// A value of `false` is fixed decimal notation, and `true` is exponential notation
		/// @returns `true` if the format option is set
		bool get_format() const;

		/// @brief Sets the format option for this object.
		/// @details The format option determines if fixed decimal or exponential notation is used.
		/// A value of `false` is fixed decimal notation, and `true` is exponential notation
		/// @param[in] value `true` to use fixed decimal notation (####.nn), `false` to use exponential ([−]###.nnE[+/−]##)
		void set_format(bool value);

		/// @brief Returns the value of the output number (only matters if there's no child number variable object).
		/// @returns The value of the output number.
		std::uint32_t get_value() const;

		/// @brief Sets the value of the output number (only matters if there's no child number variable object).
		/// @param[in] inputValue The value to set for the output number
		void set_value(std::uint32_t inputValue);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 29; ///< The fewest bytes of IOP data that can represent this object

		float scale; ///< Scale to be applied to the input value and min/max values.
		std::int32_t offset; ///< Offset to be applied to the input value and min/max values
		std::uint32_t value; ///< Raw unsigned value of the output field before scaling (unsigned 32-bit integer). Used only if variable reference attribute is NULL
		std::uint8_t numberOfDecimals; ///< Specifies number of decimals to display after the decimal point
		std::uint8_t optionsBitfield; ///< Bitfield of options defined in `Options` enum
		std::uint8_t justificationBitfield; ///< Bitfield of justification options
		bool format; ///< 0 = use fixed format decimal display (####.nn), 1 = use exponential format ([-]###.nnE[+/-]##) where n is set by the number of decimals
	};

	/// @brief Used to show one object out of a set of objects
	class OutputList : public VTObject
	{
	public:
		/// @brief Constructor for an output list object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit OutputList(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

		/// @brief Returns the number of items in the list
		/// @returns The number of items in the list
		std::uint8_t get_number_of_list_items() const;

		/// @brief Returns the value of the selected list index (only matters if no child number variable object is present)
		/// @returns The value of the selected list index
		std::uint8_t get_value() const;

		/// @brief Sets the value of the selected list index (only matters if no child number variable object is present)
		/// @param[in] value The value to set for the list's selected index
		void set_value(std::uint8_t value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 12; ///< The fewest bytes of IOP data that can represent this object

		std::uint8_t numberOfListItems; ///< Number of object references to follow. The size of the list can never exceed this number and this attribute cannot be changed.
		std::uint8_t value; ///< Selected list index of this object. Used only if variable reference attribute is NULL
	};

	/// @brief This object outputs a line shape. The starting point for the line is found in the parent object
	class OutputLine : public VTObject
	{
	public:
		/// @brief Constructor for an output line object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit OutputLine(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

		/// @brief Returns the line's direction.
		/// @details When the line direction is zero, the ine is drawn from top left to bottom right of
		/// enclosing virtual rectangle. When the line direction is 1, the line is drawn from bottom left to top right of
		/// enclosing virtual rectangle.
		/// @returns The line's direction (see details).
		std::uint8_t get_line_direction() const;

		/// @brief Sets the line's direction.
		/// @details When the line direction is zero, the ine is drawn from top left to bottom right of
		/// enclosing virtual rectangle. When the line direction is 1, the line is drawn from bottom left to top right of
		/// enclosing virtual rectangle.
		/// @param[in] value The line direction to set (see details).
		void set_line_direction(std::uint8_t value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 11; ///< The fewest bytes of IOP data that can represent this object

		std::uint8_t lineDirection; ///< 0 = Line is drawn from top left to bottom right of enclosing virtual rectangle, 1 = Line is drawn from bottom left to top right
	};

	/// @brief This object outputs a rectangle shape
	class OutputRectangle : public VTObject
	{
	public:
		/// @brief The different line suppression options
		enum class LineSuppressionOption
		{
			SuppressTopLine = 0, ///< Suppress the top line of the rectangle
			SuppressRightSideLine = 1, ///< Suppress the right side of the rectangle
			SuppressBottomLine = 2, ///< Suppress the bottom line of the rectangle
			SuppressLeftSideLine = 3 ///< Suppress the left line of the rectangle
		};

		/// @brief Constructor for an output rectangle object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit OutputRectangle(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

		/// @brief Returns the line suppression bitfield.
		/// @note See LineSuppressionOption for the bit definitions.
		/// @returns The line suppression bitfield (see LineSuppressionOption).
		std::uint8_t get_line_suppression_bitfield() const;

		/// @brief Sets the line suppression bitfield value.
		/// @note See LineSuppressionOption for the bit definitions.
		/// @param[in] value The line suppression bitfield to set.
		void set_line_suppression_bitfield(std::uint8_t value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 13; ///< The fewest bytes of IOP data that can represent this object

		std::uint8_t lineSuppressionBitfield; ///< Bitfield of line suppression options
	};

	/// @brief This object outputs an ellipse or circle shape
	class OutputEllipse : public VTObject
	{
	public:
		/// @brief Types of ellipse
		enum class EllipseType
		{
			Closed = 0, ///< Closed ellipse
			OpenDefinedByStartEndAngles = 1, ///< The ellise is defined by start and end angles
			ClosedEllipseSegment = 2,
			ClosedEllipseSection = 3
		};

		/// @brief Constructor for an output ellipse object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit OutputEllipse(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

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

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 15; ///< The fewest bytes of IOP data that can represent this object

		std::uint8_t ellipseType; ///< The type of ellipse
		std::uint8_t startAngle; ///< Start angle/2 (in degrees) from positive X axis counter clockwise (90° is straight up).
		std::uint8_t endAngle; ///< End angle/2 (in degrees) from positive X axis counter clockwise (90° is straight up)
	};

	/// @brief This object outputs a polygon
	class OutputPolygon : public VTObject
	{
	public:
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
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit OutputPolygon(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

		/// @brief Adds a point to the polygon, defined by x and y coordinates
		/// @param[in] x The X value of a point relative to the top left corner	of the polygon
		/// @param[in] y The Y value of a point relative to the top left corner	of the polygon
		void add_point(std::uint16_t x, std::uint16_t y);

		/// @brief Returns a point from the polygon by index
		/// @param[in] index The index of the point to retrieve
		/// @returns A point in the polygon by index, or zeros if the index is out of range.
		PolygonPoint get_point(std::uint8_t index);

		/// @brief Returns the polygon type of this object
		/// @returns The polygon type of this object
		PolygonType get_type() const;

		/// @brief Sets the polygon type for this object
		/// @param[in] value The new polygon type for this object
		void set_type(PolygonType value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 14; ///< The fewest bytes of IOP data that can represent this object

		std::vector<PolygonPoint> pointList; ///< List of points that make up the polygon. Must be at least 3 points!
		std::uint8_t polygonType; ///< The polygon type. Affects how the object gets drawn.
	};

	/// @brief This object is a meter. Meter is drawn about a circle enclosed within a defined square.
	class OutputMeter : public VTObject
	{
	public:
		/// @brief Options that can be applied to the input number
		enum class Options : std::uint8_t
		{
			DrawArc = 0, ///< Draw Arc
			DrawBorder = 1, ///< Draw Border
			DrawTicks = 2, ///< Draw Ticks
			DeflectionDirection = 3 ///< 0 = From min to max, counterclockwisee. 1 = from min to max, clockwise
		};

		/// @brief Constructor for an output meter object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit OutputMeter(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

		/// @brief Returns the minimum value of the output meter
		/// @returns The minimum value of the output meter
		std::uint16_t get_min_value() const;

		/// @brief Sets the minimum value of the output meter
		/// @param[in] value The minimum value to set for the output meter
		void set_min_value(std::uint8_t value);

		/// @brief Returns the max value for the output meter
		/// @returns The max value for the output meter
		std::uint16_t get_max_value() const;

		/// @brief Sets the max value for the output meter
		/// @param[in] value The max value to set for the output meter
		void set_max_value(std::uint8_t value);

		/// @brief Returns the value for the output meter (only matters if there's no child number variable object).
		/// @returns The value of the output meter
		std::uint16_t get_value() const;

		/// @brief Sets the value of the output meter (only matters if there's no child number variable object).
		/// @param[in] value The value to set for the output meter
		void set_value(std::uint8_t value);

		/// @brief Returns the value of the needle colour
		/// @returns The value of the needle colour as an index into the VT colour table
		std::uint8_t get_needle_colour() const;

		/// @brief Sets the value of the needle colour
		/// @param[in] value The colour to set for the needle as an index into the VT colour table
		void set_needle_colour(std::uint8_t value);

		/// @brief Returns the border colour of the meter
		/// @returns The border colour of the meter as an index into the VT colour table
		std::uint8_t get_border_colour() const;

		/// @brief Sets the border colour of the meter
		/// @param[in] value The border colour to set for the meter as an index into the VT colour table
		void set_border_colour(std::uint8_t value);

		/// @brief Returns the arc and tick colour for the meter
		/// @returns The arc and tick colour for the meter as an index into the VT colour table
		std::uint8_t get_arc_and_tick_colour() const;

		/// @brief Sets the arc and tick colour for the meter
		/// @param[in] value The arc and tick colour to set for the meter as an index into the VT colour table
		void set_arc_and_tick_colour(std::uint8_t value);

		/// @brief Returns the number of ticks to render across the meter
		/// @returns The number of ticks to render across the meter
		std::uint8_t get_number_of_ticks() const;

		/// @brief Sets the number of ticks to render when drawing the meter
		/// @param[in] value The number of ticks to render
		void set_number_of_ticks(std::uint8_t value);

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

		std::uint16_t minValue; ///< Minimum value. Represents value when needle is at the start of arc
		std::uint16_t maxValue; ///< Maximum value. Represents when the needle is at the end of the arc.
		std::uint16_t value; ///< Current value. Needle position set to this value, used if variable ref is NULL.
		std::uint8_t needleColour; ///< Needle (indicator) colour
		std::uint8_t borderColour; ///< Border colour (if drawn)
		std::uint8_t arcAndTickColour; ///< Meter arc and tick colour (if drawn)
		std::uint8_t optionsBitfield; ///< Bitfield of options defined in `Options` enum
		std::uint8_t numberOfTicks; ///< Number of ticks to draw about meter arc
		std::uint8_t startAngle; ///< Start angle / 2 in degrees from positive X axis counterclockwise
		std::uint8_t endAngle; ///< End angle / 2 in degrees from positve X axis counterclockwise
	};

	/// @brief This is a linear bar graph or thermometer, defined by an enclosing rectangle.
	class OutputLinearBarGraph : public VTObject
	{
	public:
		/// @brief Options that can be applied to the input number
		enum class Options : std::uint8_t
		{
			DrawArc = 0, ///< Draw Arc
			DrawBorder = 1, ///< Draw Border
			DrawTicks = 2, ///< Draw Ticks
			BarGraphType = 3, ///< 0 = Filled, 1 = not filled
			AxisOrientation = 4, ///< 0 = vertical, 1 = horizontal
			Direction = 5 ///< 0 = Grows negative, 1 = Grows positive
		};

		/// @brief Constructor for an output linear bar graph object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit OutputLinearBarGraph(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

		/// @brief Returns the minimum value on the graph. Used to scale the graph's range.
		/// @returns The minimum value that will be shown on the graph.
		std::uint16_t get_min_value() const;

		/// @brief Sets the minimum value on the graph.
		/// @details Used to scale the graph's range. Values below this will be clamped to the min.
		/// @param[in] value The minimum value to set
		void set_min_value(std::uint8_t value);

		/// @brief Returns the max value for the graph
		/// @returns The max value for the graph
		std::uint16_t get_max_value() const;

		/// @brief Sets the max value for the graph
		/// @param[in] value The max value to set for the graph
		void set_max_value(std::uint8_t value);

		/// @brief Returns the value of the graph (only matters if there's no child number variable object).
		/// @returns The value of the graph
		std::uint16_t get_value() const;

		/// @brief Sets the value of the graph (only matters if there's no child number variable object).
		/// @param[in] value The value to set for the graph
		void set_value(std::uint8_t value);

		/// @brief Returns the graph's target value (only matters if there's no target value reference).
		/// @returns The graph's target value
		std::uint16_t get_target_value() const;

		/// @brief Sets the target value for the graph (only matters if there's no target value reference).
		/// @param[in] value The target value to set
		void set_target_value(std::uint8_t value);

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
		/// @param[in] value The colour of the graph to set as an index into the VT colour table
		void set_colour(std::uint8_t value);

		/// @brief Returns the target line colour as an index into the VT colour table
		/// @returns The target line colour as an index into the VT colour table
		std::uint8_t get_target_line_colour() const;

		/// @brief Sets the target line colour
		/// @param[in] value The colour to set for the target line as an index into the VT colour table
		void set_target_line_colour(std::uint8_t value);

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
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 24; ///< The fewest bytes of IOP data that can represent this object

		std::uint16_t minValue; ///< Minimum value
		std::uint16_t maxValue; ///< Maximum value
		std::uint16_t targetValue; ///< Current target value. Used only if Target value variable Reference attribute is NULL.
		std::uint16_t targetValueReference; ///< Object ID of a Number Variable object in which	to retrieve the bar graph’s target value.
		std::uint16_t value; ///< Current value. Needle position set to this value, used if variable ref is NULL.
		std::uint8_t numberOfTicks; ///< Number of ticks to draw along the bar graph
		std::uint8_t colour; ///< Bar graph fill and border colour.
		std::uint8_t targetLineColour; ///< Target line colour (if drawn).
		std::uint8_t optionsBitfield; ///< Bitfield of options defined in `Options` enum
	};

	/// @brief TThis object is similar in concept to a linear bar graph but appears arched. Arched bar graphs are drawn about
	/// an Output Ellipse object enclosed within a defined rectangle
	class OutputArchedBarGraph : public VTObject
	{
	public:
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
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit OutputArchedBarGraph(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

		/// @brief Returns the width (px) of the bar graph
		/// @returns The width (px) of the bar graph
		std::uint16_t get_bar_graph_width() const;

		/// @brief Sets the width (px) of the bar graph
		/// @param[in] value The width (px) to set for the bar graph
		void set_bar_graph_width(std::uint16_t value);

		/// @brief Returns the minimum value of the bar graph.
		/// @note Values below this will be clamped to the min when rendered.
		/// @returns The minimum value of the bar graph
		std::uint16_t get_min_value() const;

		/// @brief Sets the minimum value for the bar graph
		/// @note Values below this will be clamped to the min when rendered.
		/// @param[in] value The minimum value to set
		void set_min_value(std::uint16_t value);

		/// @brief Returns the maximum value of the bar graph
		/// @note Values above this will be clamped to the max when rendered.
		/// @returns The maximum value of the bar graph
		std::uint16_t get_max_value() const;

		/// @brief Sets the max value of the bar graph
		/// @note Values above this will be clamped to the max when rendered.
		/// @param[in] value The maximum value of the bar graph to set
		void set_max_value(std::uint16_t value);

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
		/// @param[in] value The new value for the options bitfield
		void set_options(std::uint8_t value);

		/// @brief Sets a single option in the options bitfield to the specified value
		/// @param[in] option The option to set
		/// @param[in] value The new value of the option bit
		void set_option(Options option, bool value);

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

		std::uint16_t barGraphWidth; ///< Bar graph width in pixels. Bar graph width should be less than half the total width, or less than half the total height, whichever is least.
		std::uint16_t minValue; ///< Minimum value. Represents value when needle is at the start of arc
		std::uint16_t maxValue; ///< Maximum value. Represents when the needle is at the end of the arc.
		std::uint16_t value; ///< Current value. Needle position set to this value, used if variable ref is NULL.
		std::uint16_t targetValue; ///< Current target value. Used only if Target value variable Reference attribute is NULL.
		std::uint16_t targetValueReference; ///< Object ID of a Number Variable object in which	to retrieve the bar graph’s target value.
		std::uint8_t targetLineColour; ///< Target line colour (if drawn)
		std::uint8_t colour; ///< Bar graph fill and border colour
		std::uint8_t optionsBitfield; ///< Bitfield of options defined in `Options` enum
		std::uint8_t startAngle; ///< Start angle / 2 in degrees from positive X axis counterclockwise
		std::uint8_t endAngle; ///< End angle / 2 in degrees from positve X axis counterclockwise
	};

	/// @brief This object displays a picture graphic (bitmap)
	class PictureGraphic : public VTObject
	{
	public:
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
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit PictureGraphic(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

		/// @brief Returns a reference to the underlying bitmap data
		/// @returns A reference to the underlying bitmap data
		std::vector<std::uint8_t> &get_raw_data();

		/// @brief Sets a large chunk of data to the underlying bitmap
		/// @param[in] data Pointer to a buffer of data
		/// @param[in] size The length of the data buffer to add to the underlying bitmap
		void set_raw_data(std::uint8_t *data, std::uint32_t size);

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
		std::uint32_t numberOfBytesInRawData; ///< Number of bytes of raw data
		std::uint16_t actualWidth; ///< The actual width of the bitmap
		std::uint16_t actualHeight; ///< The actual height of the bitmap
		std::uint8_t formatByte; ///< The format option byte
		std::uint8_t optionsBitfield; ///< Options bitfield, see the `options` enum
		std::uint8_t transparencyColour; ///< The colour to render as transparent if so set in the options
	};

	/// @brief A number variable holds a 32-bit unsigned integer value
	class NumberVariable : public VTObject
	{
	public:
		/// @brief Constructor for a number variable object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit NumberVariable(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

		/// @brief Returns the number variable's value
		/// @returns The number variable's value
		std::uint32_t get_value() const;

		/// @brief Sets the number variable's value
		/// @param[in] value The value to set for the number variable
		void set_value(std::uint32_t value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 7; ///< The fewest bytes of IOP data that can represent this object

		std::uint32_t value; ///< 32-bit unsigned integer value
	};

	/// @brief A String Variable holds a fixed length string.
	class StringVariable : public VTObject
	{
	public:
		/// @brief Constructor for a string variable object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit StringVariable(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

		/// @brief Returns the actual string value stored in this object
		/// @returns The string value stored in this object
		std::string get_value();

		/// @brief Sets the actual string value stored in this object
		/// @param[in] value The new string value for this object
		void set_value(std::string value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 5; ///< The fewest bytes of IOP data that can represent this object

		std::string value; ///< The actual value of the string, for non utf-16 strings
	};

	/// @brief This object holds attributes related to fonts.
	class FontAttributes : public VTObject
	{
	public:
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

		/// @brief Constructor for a font attributes object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit FontAttributes(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

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
		bool get_style(FontStyleBits styleSetting);

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

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 8; ///< The fewest bytes of IOP data that can represent this object

		std::uint8_t colour; ///< Text clour
		std::uint8_t size; ///< Font size
		std::uint8_t type; ///< Encoding type
		std::uint8_t style; ///< Font style
	};

	/// @brief Defines a line attributes object, which describes how lines should be displayed on the VT
	class LineAttributes : public VTObject
	{
	public:
		/// @brief Constructor for a line attributes object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit LineAttributes(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

		/// @brief Sets the line art bit pattern. Each bit represents 1 pixel's on/off state.
		/// @returns The line attribute's line art bit pattern
		std::uint16_t get_line_art_bit_pattern() const;

		/// @brief Sets the line art bit patter for the line attribute
		/// @param[in] value The line art bit pattern to set
		void set_line_art_bit_pattern(std::uint16_t value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 8; ///< The fewest bytes of IOP data that can represent this object

		std::uint16_t lineArtBitpattern; ///< Bit pattern art for line. Each bit represents a paintbrush spot
	};

	/// @brief This object holds attributes related to filling output shape objects
	class FillAttributes : public VTObject
	{
	public:
		/// @brief Enumerates the different fill types for an object
		enum class FillType : std::uint8_t
		{
			NoFill = 0, ///< No fill will be applied
			FillWithLineColor = 1, ///< Fill with the color of the outline of the shape
			FillWithSpecifiedColorInFillColorAttribute = 2, ///< Fill with the color specified by a fill attribute
			FillWithPatternGivenByFillPatternAttribute = 3 ///< Fill with a patter provided by a fill pattern attribute
		};

		/// @brief Constructor for a fill attributes object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit FillAttributes(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

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

		std::uint16_t fillPattern; ///< Object id of a Picture Graphic object to use as a Fill pattern
		FillType type; ///< The fill type/mode associated with this object
	};

	/// @brief This object defines the valid or invalid characters for an Input String object
	class InputAttributes : public VTObject
	{
	public:
		/// @brief Constructor for a input attributes object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit InputAttributes(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

		/// @brief Returns the validation string associated to this input attributes object
		/// @returns The validation string associated to this input attributes object
		std::string get_validation_string() const;

		/// @brief Sets the validation string for this object
		/// @param[in] value The new validation string for this object
		void set_validation_string(std::string value);

		/// @brief Returns the validation type setting for this object
		/// @returns The validation type associated to this object
		std::uint8_t get_validation_type() const;

		/// @brief Sets the validation type setting for this object
		/// @param[in] value The validation type
		void set_validation_type(std::uint8_t value);

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 7; ///< The fewest bytes of IOP data that can represent this object

		std::string validationString; ///< String containing all valid or invalid character codes
		std::uint8_t validationType; ///< 0 = valid characters are listed, 1 = invalid characters are listed
	};

	/// @brief The Extended Input Attributes object, available in VT version 4 and later, defines the valid or invalid
	/// characters for an Input String object
	class ExtendedInputAttributes : public VTObject
	{
	public:
		/// @brief Constructor for an extended input attributes object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit ExtendedInputAttributes(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

		/// @brief Returns the number of code planes in this extended input attributes
		/// @returns The number of code planes in this extended input attributes
		std::uint8_t get_number_of_code_planes() const;

		/// @brief Sets the number of code planes in this extended input attributes object
		/// @param[in] value The new number of code planes
		void set_number_of_code_planes(std::uint8_t value);

		/// @brief Returns the validation type setting for this object
		/// @returns The validation type associated to this object
		std::uint8_t get_validation_type() const;

		/// @brief Sets the validation type setting for this object
		/// @param[in] value The validation type
		void set_validation_type(std::uint8_t value);

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
		std::uint8_t validationType; ///< 0 = valid characters are listed, 1 = invalid characters are listed
	};

	/// @brief Points to another object
	class ObjectPointer : public VTObject
	{
	public:
		/// @brief Constructor for a object pointer object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit ObjectPointer(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 5; ///< The fewest bytes of IOP data that can represent this object
	};

	/// @brief Defines a macro object. Performs a list of commands based on a message or event.
	class Macro : public VTObject
	{
	public:
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
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit Macro(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 5; ///< The fewest bytes of IOP data that can represent this object
		/// @todo Finish VT Macro implementation
	};

	/// @brief Defines a colour map object
	class ColourMap : public VTObject
	{
	public:
		/// @brief Constructor for a colour map object
		/// @param[in] parentObjectPool a Pointer to the rest of the object pool this object is a member of
		explicit ColourMap(std::map<std::uint16_t, VTObject *> *parentObjectPool);

		/// @brief Returns the VT object type of the underlying derived object
		/// @returns The VT object type of the underlying derived object
		VirtualTerminalObjectType get_object_type() const override;

		/// @brief Returns the minimum binary serialized length of the associated object
		/// @returns The minimum binary serialized length of the associated object
		std::uint32_t get_minumum_object_lenth() const override;

		/// @brief Performs basic error checking on the object and returns if the object is valid
		/// @returns `true` if the object passed basic error checks
		bool get_is_valid() const override;

	private:
		static constexpr std::uint32_t MIN_OBJECT_LENGTH = 5; ///< The fewest bytes of IOP data that can represent this object
	};

} // namespace isobus

#endif // ISOBUS_VIRTUAL_TERMINAL_OBJECTS_HPP
