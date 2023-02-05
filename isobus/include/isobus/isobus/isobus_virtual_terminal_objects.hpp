//================================================================================================
/// @file isobus_virtual_terminal_objects.hpp
///
/// @brief Enumerates the different VT object types that can comprise a VT object pool
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef ISOBUS_VIRTUAL_TERMINAL_OBJECTS_HPP
#define ISOBUS_VIRTUAL_TERMINAL_OBJECTS_HPP

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
} // namespace isobus

#endif // ISOBUS_VIRTUAL_TERMINAL_OBJECTS_HPP
