//================================================================================================
/// @file isobus_task_controller_client_objects.hpp
///
/// @brief Defines a set of C++ objects that represent a DDOP
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef ISOBUS_TASK_CONTROLLER_CLIENT_OBJECTS_HPP
#define ISOBUS_TASK_CONTROLLER_CLIENT_OBJECTS_HPP

#include <array>
#include <string>
#include <vector>

namespace isobus
{
	/// @brief A namespace that contains the generic task controller objects
	namespace task_controller_object
	{
		/// @brief A base class for a Task Controller Object
		class Object
		{
		public:
			/// @brief Constructor for the TC object base class
			/// @param[in] objectDesignator Descriptive text for this object, UTF-8 encoded, 32 characters max
			/// @param[in] uniqueID The object ID of the object. Must be unique in the DDOP.
			Object(std::u32string objectDesignator, std::uint16_t uniqueID);

			/// @brief Returns the Descriptive text for this object, UTF-8 encoded, 32 characters max
			/// @returns Descriptive text for this object, UTF-8 encoded, 32 characters max
			std::u32string get_designator() const;

			/// @brief Returns the object ID of the object
			/// @returns The object ID of the object
			std::uint16_t get_object_id() const;

			/// @brief Returns the XML namespace for the object
			/// @returns the XML namespace for the object
			virtual std::string get_table_id() const = 0;

		private:
			std::u32string designator; ///< UTF-8 Descriptive text to identify this object. Max length of 32.
			std::uint16_t objectID; ///< Unique object ID in the DDOP
		};

		/// @brief Each device shall have one single DeviceObject in its device descriptor object pool.
		/// see A.2 in ISO11783-10
		class DeviceObject : public Object
		{
		public:
			/// @brief Constructor for a device object
			/// @param[in] deviceDesignator Descriptive text for the object, UTF-8, 32 chars max
			/// @param[in] deviceSoftwareVersion Software version indicating text
			/// @param[in] deviceSerialNumber Device and manufacturer-specific serial number of the Device
			/// @param[in] deviceStructureLabel This label allows the device to identify the current version of the device descriptor object pool
			/// @param[in] deviceLocalizationLabel Defined by the language command PGN
			/// @param[in] deviceExtendedStructureLabel Continuation of the Label given by Device to identify the Device descriptor Structure
			/// @param[in] clientIsoNAME NAME of client device as defined in ISO 11783-5
			DeviceObject(std::u32string deviceDesignator,
			             std::u32string deviceSoftwareVersion,
			             std::u32string deviceSerialNumber,
			             std::array<std::uint8_t, 7> deviceStructureLabel,
			             std::array<std::uint8_t, 7> deviceLocalizationLabel,
			             std::vector<std::uint8_t> &deviceExtendedStructureLabel,
			             std::uint64_t clientIsoNAME);

			/// @brief Returns the XML namespace for the object
			/// @returns the XML namespace for the object
			std::string get_table_id() const override;

			/// @brief Returns the serial number for the device
			/// @returns The serial number for the device
			std::u32string get_serial_number() const;

			/// @brief Returns the structure label for this DDOP
			/// @returns The structure label for this DDOP
			std::array<std::uint8_t, 7> get_structure_label() const;

			/// @brief Returns the localization label for this DDOP
			/// @returns The  localization label for this DDOP
			std::array<std::uint8_t, 7> get_localization_label() const;

			/// @brief Returns the extended structure label (if applicable)
			/// @returns The extended structure label (if applicable)
			std::vector<std::uint8_t> get_extended_structure_label() const;

			/// @brief Returns the ISO NAME associated with this DDOP
			/// @returns The raw ISO NAME associated with this DDOP
			std::uint64_t get_iso_name() const;

		private:
			static const std::string tableID; ///< XML element namespace for device.
			std::u32string serialNumber; ///< Device and manufacturer-specific serial number of the Device
			std::u32string softwareVersion; ///< Software version of the device
			std::array<std::uint8_t, 7> structureLabel; ///< Label given by device to identify the device descriptor structure
			std::array<std::uint8_t, 7> localizationLabel; ///< Label given by device to identify the device descriptor localization
			std::vector<std::uint8_t> extendedStructureLabel; ///< Continuation of the Label given by Device to identify the Device descriptor Structure
			std::uint64_t NAME; ///< The NAME of client device as defined in ISO 11783-5. MUST match your address claim
		};

		/// @brief DeviceElementObject is the object definition of the XML element DeviceElement.
		/// The attribute Type specifies the type of this particular element definition
		/// @details Referable Child Objects: DeviceProcessDataObject, DevicePropertyObject
		class DeviceElementObject : public Object
		{
		public:
			/// @brief Enumerates the types of device element object
			enum class Type : std::uint8_t
			{
				Device = 1, ///< The device descriptor object pool shall have one device element of type device
				Function = 2, ///< This device element type can be used as a generic device element to define individually accessible components of a device like valves or sensors
				Bin = 3, ///< This is, for instance, the tank of a sprayer or the bin of a seeder.
				Section = 4, ///< This is, for instance, the section of a spray boom, seed toolbar, or planter toolbar.
				Unit = 5, ///< This device element type is, for example, used for spray boom nozzles, seeder openers, or planter row units.
				Connector = 6, ///< This device element type specifies the mounting/connection position of the device
				NavigationReference = 7 ///< This device element type defines the navigation reference position for navigation devices such as GPS receivers
			};

			/// @brief Constructor for a DeviceElementObject
			/// @param[in] deviceElementDesignator Descriptive text for the object, UTF-8, 32 chars max
			/// @param[in] deviceElementNumber The Element number for process data variable	addressing
			/// @param[in] parentObjectID Object ID of parent DeviceElementObject or DeviceObject in order to establish a hierarchical order of DeviceElements
			/// @param[in] deviceEelementType The type of element, such as "device" or "bin"
			/// @param[in] uniqueID The object ID of the object. Must be unique in the DDOP.
			DeviceElementObject(std::u32string deviceElementDesignator,
			                    std::uint16_t deviceElementNumber,
			                    std::uint16_t parentObjectID,
			                    Type deviceEelementType,
			                    std::uint16_t uniqueID);

			/// @brief Returns the XML namespace for the object
			/// @returns The string "DET", the XML namespace for the DeviceElementObject
			std::string get_table_id() const override;

			/// @brief Returns the element number
			/// @returns The element number
			std::uint16_t get_element_number() const;

			/// @brief Returns the parent object ID
			/// @returns The parent object ID
			std::uint16_t get_parent_object() const;

			/// @brief Returns the type of the element object
			/// @returns The type of the element object
			Type get_type() const;

		private:
			static const std::string tableID; ///< XML element namespace for DeviceElement.
			std::vector<std::uint16_t> referenceList; ///< List of references to DeviceProcessDataObjects or DevicePropertyObjects
			std::uint16_t elementNumber; ///< Element number for process data variable addressing
			std::uint16_t parentObject; ///< Object ID of parent DeviceElementObject or DeviceObject in order to establish a hierarchical order of DeviceElements
			Type elementType; ///< See the comments on `Type` or ISO11783-10 table A.2
		};

		/// @brief The DeviceProcessDataObject is the object definition of the XML element DeviceProcessData. Each
		/// object contains a single process data variable definition
		/// @details Referable child object: DeviceValuePresentationObject
		class DeviceProcessDataObject : public Object
		{
		public:
			/// @brief Enumerates the properties in the properties bitset of this object
			enum class PropertiesBit : std::uint8_t
			{
				MemberOfDefaultSet = 0x01, ///< member of default set
				Settable = 0x02, ///< if this object is settable
				ControlSource = 0x04 ///< Version 4, mutually exclusive with bit 2
			};

			/// @brief Enumerates the trigger methods that can be set in the available trigger bitset of this object
			enum class AvailableTriggerMethods
			{
				TimeInterval = 0x01, ///< The device can provide these device process data based on a time interval
				DistanceInterval = 0x02, ///< The device can provide these device process data based on a distance interval.
				ThresholdLimits = 0x04, ///< The device can provide these device process data based on a surpassing of the value threshold
				OnChange = 0x08, ///< The device can provide these device process data when its value changes
				Total = 0x10 ///< These device process data are a total
			};

			/// @brief Constructor for a DeviceProcessDataObject
			/// @param[in] processDataDesignator Descriptive text for the object, UTF-8, 32 chars max
			/// @param[in] processDataDDI Identifier of process data variable (DDI) according to definitions in Annex B and ISO 11783 - 11
			/// @param[in] deviceValuePresentationObjectID Object identifier of a DeviceValuePresentationObject, or the null ID
			/// @param[in] processDataProperties A bitset of properties associated to this object. Some combination of `PropertiesBit`
			/// @param[in] processDataTriggerMethods A bitset of available trigger methods, built from some combination of `AvailableTriggerMethods`
			/// @param[in] uniqueID The object ID of the object. Must be unique in the DDOP.
			DeviceProcessDataObject(std::u32string processDataDesignator,
			                        std::uint16_t processDataDDI,
			                        std::uint16_t deviceValuePresentationObjectID,
			                        std::uint8_t processDataProperties,
			                        std::uint8_t processDataTriggerMethods,
			                        std::uint16_t uniqueID);

			/// @brief Returns the XML element namespace for DeviceProcess-Data.
			/// @returns The string "DPD", the XML element namespace for DeviceProcess-Data.
			std::string get_table_id() const override;

			/// @brief Returns the DDI
			/// @returns the DDI for this property
			std::uint16_t get_ddi() const;

			/// @brief Returns Object identifier of the DeviceValuePresentation-Object for this object, or the null ID
			/// @returns Object identifier of DeviceValuePresentation-Object for this object
			std::uint16_t get_device_value_presentation_object_id() const;

			/// @brief Returns the object's properties bitfield
			/// @returns The properties bitfield for this object
			std::uint8_t get_properties_bitfield() const;

			/// @brief Returns the object's available trigger methods
			/// @returns The available trigger methods bitfield for this object
			std::uint8_t get_trigger_methods_bitfield() const;

		private:
			static const std::string tableID; ///< XML element namespace for DeviceProcessData.
			std::uint16_t ddi; ///< Identifier of process data variable
			std::uint16_t deviceValuePresentationObject; ///< Object identifier of DeviceValuePresentation-Object
			std::uint8_t propertiesBitfield; ///<  A bitset of properties for this object
			std::uint8_t triggerMethodsBitfield; ///< A bitset defined in A.4.1 to A.4.5
		};

		/// @brief DevicePropertyObject is the object definition of the XML element DeviceProperty. Each object contains
		/// a single DeviceElementProperty definition
		/// @details Referable child object: DeviceValuePresentationObject
		class DevicePropertyObject : public Object
		{
		public:
			/// @brief Constructor for a DevicePropertyObject
			/// @param[in] propertyDesignator Descriptive text for the object, UTF-8, 32 chars max
			/// @param[in] propertyValue The value of the property
			/// @param[in] propertyDDI Identifier of property (DDI) according to definitions in Annex B and ISO 11783 - 11.
			/// @param[in] valuePresentationObject Object identifier of DeviceValuePresentationObject, or NULL object ID
			/// @param[in] uniqueID The object ID of the object. Must be unique in the DDOP.
			DevicePropertyObject(std::u32string propertyDesignator,
			                     std::int32_t propertyValue,
			                     std::uint16_t propertyDDI,
			                     std::uint16_t valuePresentationObject,
			                     std::uint16_t uniqueID);

			/// @brief Returns the XML element namespace for DeviceProperty.
			/// @returns The string "DPD", the XML element namespace for DeviceProcessData.
			std::string get_table_id() const override;

			/// @brief Returns the property's value
			/// @returns The property's value
			std::int32_t get_value() const;

			/// @brief Sets the property value
			/// @param[in] newValue The value to set the property to
			void set_value(std::int32_t newValue);

			/// @brief Returns the DDI for this object
			/// @returns The DDI for this object
			std::uint16_t get_ddi() const;

			/// @brief Returns the object identifier of an associated DeviceValuePresentationObject
			/// @returns The object identifier of an associated DeviceValuePresentationObject
			std::uint16_t get_device_value_presentation_object() const;

		private:
			static const std::string tableID; ///< XML element namespace for DeviceProperty.
			std::int32_t value; ///< The value of the property.
			std::uint16_t ddi; ///< Identifier of property (DDI) according to definitions in Annex B and ISO 11783 - 11.
			std::uint16_t deviceValuePresentationObject; ///< Object identifier of DeviceValuePresentationObject
		};

		/// @brief This object contains the presentation information to display the value of a DeviceProcessData or
		/// DeviceProperty object.The device can update these objects when the language and/or units of measure
		/// are changed by the operator
		/// @details Referable child objects: none.
		class DeviceValuePresentationObject : public Object
		{
		public:
			/// @brief The constructor for a DeviceValuePresentationObject
			/// @param[in] unitDesignator Unit designator for this value presentation
			/// @param[in] offsetValue Offset to be applied to the value for presentation.
			/// @param[in] scaleFactor Scale to be applied to the value for presentation.
			/// @param[in] numberDecimals Specifies the number of decimals to display after the decimal point.
			/// @param[in] uniqueID The object ID of the object. Must be unique in the DDOP.
			DeviceValuePresentationObject(std::u32string unitDesignator,
			                              std::int32_t offsetValue,
			                              float scaleFactor,
			                              std::uint8_t numberDecimals,
			                              std::uint16_t uniqueID);

			/// @brief Returns the XML element namespace for DeviceValuePresentation.
			/// @returns The string "DPD", the XML element namespace for DeviceProcessData.
			std::string get_table_id() const override;

			/// @brief Returns the offset that is applied to the value for presentation
			/// @returns The offset that is applied to the value for presentation
			std::int32_t get_offset() const;

			/// @brief Returns the scale that is applied to the value for presentation
			/// @returns The scale that is applied to the value for presentation
			float get_scale() const;

			/// @brief Returns the number of decimals shown after the decimal point
			/// @returns The number of decimals shown after the decimal point
			std::uint8_t get_number_of_decimals() const;

		private:
			static const std::string tableID; ///< XML element namespace for DeviceValuePresentation.
			std::int32_t offset; ///< Offset to be applied to the value for presentation
			float scale; ///< Scale to be applied to the value for presentation
			std::uint8_t numberOfDecimals; ///< Specify number of decimals to display after the decimal point
		};
	} // namespace task_controller_object
} // namespace isobus

#endif // ISOBUS_TASK_CONTROLLER_CLIENT_OBJECTS_HPP
