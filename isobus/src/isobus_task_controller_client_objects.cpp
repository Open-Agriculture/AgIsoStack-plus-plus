//================================================================================================
/// @file isobus_task_controller_client_objects.cpp
///
/// @brief Implements the base functionality of the basic task controller objects.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/isobus_task_controller_client_objects.hpp"

namespace isobus
{
	namespace task_controller_object
	{
		Object::Object(std::u32string objectDesignator, std::uint16_t uniqueID) :
		  designator(objectDesignator),
		  objectID(uniqueID)
		{
		}

		std::u32string Object::get_designator() const
		{
			return designator;
		}

		std::uint16_t Object::get_object_id() const
		{
			return objectID;
		}

		const std::string DeviceObject::tableID = "DVC";

		DeviceObject::DeviceObject(std::u32string deviceDesignator,
		                           std::u32string deviceSoftwareVersion,
		                           std::u32string deviceSerialNumber,
		                           std::array<std::uint8_t, 7> deviceStructureLabel,
		                           std::array<std::uint8_t, 7> deviceLocalizationLabel,
		                           std::vector<std::uint8_t> &deviceExtendedStructureLabel,
		                           std::uint64_t clientIsoNAME) :
		  Object(deviceDesignator, 0),
		  serialNumber(deviceSerialNumber),
		  softwareVersion(deviceSoftwareVersion),
		  structureLabel(deviceStructureLabel),
		  localizationLabel(deviceLocalizationLabel),
		  extendedStructureLabel(deviceExtendedStructureLabel),
		  NAME(clientIsoNAME)
		{
		}

		std::string DeviceObject::get_table_id() const
		{
			return tableID;
		}

		std::u32string DeviceObject::get_serial_number() const
		{
			return serialNumber;
		}

		std::array<std::uint8_t, 7> DeviceObject::get_structure_label() const
		{
			return structureLabel;
		}

		std::array<std::uint8_t, 7> DeviceObject::get_localization_label() const
		{
			return localizationLabel;
		}

		std::vector<std::uint8_t> DeviceObject::get_extended_structure_label() const
		{
			return extendedStructureLabel;
		}

		std::uint64_t DeviceObject::get_iso_name() const
		{
			return NAME;
		}

		const std::string DeviceElementObject::tableID = "DET";

		DeviceElementObject::DeviceElementObject(std::u32string deviceElementDesignator,
		                                         std::uint16_t deviceElementNumber,
		                                         std::uint16_t parentObjectID,
		                                         Type deviceEelementType,
		                                         std::uint16_t uniqueID) :
		  Object(deviceElementDesignator, uniqueID),
		  elementNumber(deviceElementNumber),
		  parentObject(parentObjectID),
		  elementType(deviceEelementType)
		{
		}

		std::string DeviceElementObject::get_table_id() const
		{
			return tableID;
		}

		std::uint16_t DeviceElementObject::get_element_number() const
		{
			return elementNumber;
		}

		std::uint16_t DeviceElementObject::get_parent_object() const
		{
			return parentObject;
		}

		DeviceElementObject::Type DeviceElementObject::get_type() const
		{
			return elementType;
		}

		const std::string DeviceProcessDataObject::tableID = "DPD";

		DeviceProcessDataObject::DeviceProcessDataObject(std::u32string processDataDesignator,
		                                                 std::uint16_t processDataDDI,
		                                                 std::uint16_t deviceValuePresentationObjectID,
		                                                 std::uint8_t processDataProperties,
		                                                 std::uint8_t processDataTriggerMethods,
		                                                 std::uint16_t uniqueID) :
		  Object(processDataDesignator, uniqueID),
		  ddi(processDataDDI),
		  deviceValuePresentationObject(deviceValuePresentationObjectID),
		  propertiesBitfield(processDataProperties),
		  triggerMethodsBitfield(processDataTriggerMethods)
		{
		}

		std::string DeviceProcessDataObject::get_table_id() const
		{
			return tableID;
		}

		std::uint16_t DeviceProcessDataObject::get_ddi() const
		{
			return ddi;
		}

		std::uint16_t DeviceProcessDataObject::get_device_value_presentation_object_id() const
		{
			return deviceValuePresentationObject;
		}

		std::uint8_t DeviceProcessDataObject::get_properties_bitfield() const
		{
			return propertiesBitfield;
		}

		std::uint8_t DeviceProcessDataObject::get_trigger_methods_bitfield() const
		{
			return triggerMethodsBitfield;
		}

		const std::string DevicePropertyObject::tableID = "DPT";

		DevicePropertyObject::DevicePropertyObject(std::u32string propertyDesignator,
		                                           std::int32_t propertyValue,
		                                           std::uint16_t propertyDDI,
		                                           std::uint16_t valuePresentationObject,
		                                           std::uint16_t uniqueID) :
		  Object(propertyDesignator, uniqueID),
		  value(propertyValue),
		  ddi(propertyDDI),
		  deviceValuePresentationObject(valuePresentationObject)
		{
		}

		std::string DevicePropertyObject::get_table_id() const
		{
			return tableID;
		}

		std::int32_t DevicePropertyObject::get_value() const
		{
			return value;
		}

		void DevicePropertyObject::set_value(std::int32_t newValue)
		{
			value = newValue;
		}

		std::uint16_t DevicePropertyObject::get_ddi() const
		{
			return ddi;
		}

		std::uint16_t DevicePropertyObject::get_device_value_presentation_object() const
		{
			return deviceValuePresentationObject;
		}

		const std::string DeviceValuePresentationObject::tableID = "DVP";

		DeviceValuePresentationObject::DeviceValuePresentationObject(std::u32string unitDesignator,
		                                                             std::int32_t offsetValue,
		                                                             float scaleFactor,
		                                                             std::uint8_t numberDecimals,
		                                                             std::uint16_t uniqueID) :
		  Object(unitDesignator, uniqueID),
		  offset(offsetValue),
		  scale(scaleFactor),
		  numberOfDecimals(numberDecimals)
		{
		}

		std::string DeviceValuePresentationObject::get_table_id() const
		{
			return tableID;
		}

		std::int32_t DeviceValuePresentationObject::get_offset() const
		{
			return offset;
		}

		float DeviceValuePresentationObject::get_scale() const
		{
			return scale;
		}

		std::uint8_t DeviceValuePresentationObject::get_number_of_decimals() const
		{
			return numberOfDecimals;
		}

	} // namespace task_controller_object
} // namespace isobus
