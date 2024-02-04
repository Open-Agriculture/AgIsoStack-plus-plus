//================================================================================================
/// @file isobus_device_descriptor_object_pool_helpers.cpp
///
/// @brief Implements helpers for the DeviceDescriptorObjectPool class.
/// @author Adrian Del Grosso
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/isobus_device_descriptor_object_pool_helpers.hpp"

#include "isobus/isobus/can_stack_logger.hpp"

namespace isobus
{
	DeviceDescriptorObjectPoolHelper::ObjectPoolValue::operator bool() const
	{
		return exists();
	}

	bool DeviceDescriptorObjectPoolHelper::ObjectPoolValue::exists() const
	{
		return isValuePresent;
	}

	bool DeviceDescriptorObjectPoolHelper::ObjectPoolValue::editable() const
	{
		return isSettable;
	}

	std::int32_t DeviceDescriptorObjectPoolHelper::ObjectPoolValue::get() const
	{
		return value;
	}

	DeviceDescriptorObjectPoolHelper::Section::Section()
	{
	}

	DeviceDescriptorObjectPoolHelper::SubBoom::SubBoom()
	{
	}

	DeviceDescriptorObjectPoolHelper::Implement DeviceDescriptorObjectPoolHelper::get_implement_geometry(DeviceDescriptorObjectPool &ddop)
	{
		Implement retVal;

		if (0 == ddop.size())
		{
			CANStackLogger::error("[DDOP Helper]: No objects in the pool.");
			return retVal; // Return empty object
		}

		// First, find the device object
		for (std::uint16_t i = 0; i < ddop.size(); i++)
		{
			auto deviceObject = ddop.get_object_by_index(i);

			if ((nullptr != deviceObject) &&
			    (task_controller_object::ObjectTypes::Device == deviceObject->get_object_type()))
			{
				// Track if we ever find a function. If not, the device will be the boom's root.
				bool foundFunction = false;
				std::shared_ptr<task_controller_object::DeviceElementObject> deviceElementObject;

				// Next, iterate through all elements whose parent is the device object
				for (std::uint16_t j = 0; j < ddop.size(); j++)
				{
					auto currentElementObject = ddop.get_object_by_index(j);

					if ((nullptr != currentElementObject) &&
					    (task_controller_object::ObjectTypes::DeviceElement == currentElementObject->get_object_type()) &&
					    (std::static_pointer_cast<task_controller_object::DeviceElementObject>(currentElementObject)->get_parent_object() == deviceObject->get_object_id()))
					{
						deviceElementObject = std::static_pointer_cast<task_controller_object::DeviceElementObject>(currentElementObject);

						// All the things we care about are likely in here, under the device element.
						for (std::uint16_t k = 0; k < ddop.size(); k++)
						{
							auto potentialFunction = ddop.get_object_by_index(k);

							if ((nullptr != potentialFunction) &&
							    (task_controller_object::ObjectTypes::DeviceElement == potentialFunction->get_object_type()) &&
							    (std::static_pointer_cast<task_controller_object::DeviceElementObject>(potentialFunction)->get_parent_object() == currentElementObject->get_object_id()) &&
							    (task_controller_object::DeviceElementObject::Type::Function == std::static_pointer_cast<task_controller_object::DeviceElementObject>(potentialFunction)->get_type()))
							{
								parse_element(ddop, std::static_pointer_cast<task_controller_object::DeviceElementObject>(potentialFunction), retVal);
								foundFunction = true;
							}
						}
						break;
					}
				}

				if (!foundFunction && (nullptr != deviceElementObject))
				{
					// If we didn't find a function, the device element object is the root of the boom.
					// So we'll reparse the device element object to get the sections and properties we care about.
					parse_element(ddop, deviceElementObject, retVal);
				}
				return retVal;
			}
		}
		CANStackLogger::error("[DDOP Helper]: No device object in the pool.");
		return retVal; // If we got here, we didn't find a device object? Return empty object
	}

	void DeviceDescriptorObjectPoolHelper::parse_element(DeviceDescriptorObjectPool &ddop,
	                                                     std::shared_ptr<task_controller_object::DeviceElementObject> elementObject,
	                                                     Implement &implementToPopulate)
	{
		Boom boomToPopulate;

		if (task_controller_object::DeviceElementObject::Type::Function == elementObject->get_type())
		{
			// Accumulate the number of functions under this function.
			// Need to search the whole pool because elements have parent links, not child links, which is not very efficient.
			for (std::uint16_t i = 0; i < ddop.size(); i++)
			{
				auto element = ddop.get_object_by_index(i);

				if ((nullptr != element) &&
				    (task_controller_object::ObjectTypes::DeviceElement == element->get_object_type()) &&
				    (std::static_pointer_cast<task_controller_object::DeviceElementObject>(element)->get_parent_object() == elementObject->get_object_id()) &&
				    (task_controller_object::DeviceElementObject::Type::Function == std::static_pointer_cast<task_controller_object::DeviceElementObject>(element)->get_type()))
				{
					boomToPopulate.subBooms.push_back(parse_sub_boom(ddop, std::static_pointer_cast<task_controller_object::DeviceElementObject>(element)));
				}
			}
		}

		if (boomToPopulate.subBooms.empty())
		{
			// Find all sections in this boom
			for (std::uint16_t i = 0; i < ddop.size(); i++)
			{
				auto section = ddop.get_object_by_index(i);

				if ((nullptr != section) &&
				    (task_controller_object::ObjectTypes::DeviceElement == section->get_object_type()) &&
				    (task_controller_object::DeviceElementObject::Type::Section == std::static_pointer_cast<task_controller_object::DeviceElementObject>(section)->get_type()) &&
				    (std::static_pointer_cast<task_controller_object::DeviceElementObject>(section)->get_parent_object() == elementObject->get_object_id()))
				{
					boomToPopulate.sections.push_back(parse_section(ddop, std::static_pointer_cast<task_controller_object::DeviceElementObject>(section)));
				}
			}

			// Find child DDIs that we care about
			for (std::uint16_t i = 0; i < elementObject->get_number_child_objects(); i++)
			{
				auto child = ddop.get_object_by_id(elementObject->get_child_object_id(i));

				if (nullptr != child)
				{
					if (task_controller_object::ObjectTypes::DeviceProperty == child->get_object_type())
					{
						auto property = std::static_pointer_cast<task_controller_object::DevicePropertyObject>(child);
						setValueFromProperty(boomToPopulate.xOffset_mm, property, DataDescriptionIndex::DeviceElementOffsetX);
						setValueFromProperty(boomToPopulate.yOffset_mm, property, DataDescriptionIndex::DeviceElementOffsetY);
						setValueFromProperty(boomToPopulate.zOffset_mm, property, DataDescriptionIndex::DeviceElementOffsetZ);
					}
					else if (task_controller_object::ObjectTypes::DeviceProcessData == child->get_object_type())
					{
						auto processData = std::static_pointer_cast<task_controller_object::DeviceProcessDataObject>(child);
						setEditableFromProcessData(boomToPopulate.xOffset_mm, processData, DataDescriptionIndex::DeviceElementOffsetX);
						setEditableFromProcessData(boomToPopulate.yOffset_mm, processData, DataDescriptionIndex::DeviceElementOffsetY);
						setEditableFromProcessData(boomToPopulate.zOffset_mm, processData, DataDescriptionIndex::DeviceElementOffsetZ);
					}
				}
			}
		}
		else
		{
			// Sections are part of the sub booms
		}
		implementToPopulate.booms.push_back(boomToPopulate);
	}

	DeviceDescriptorObjectPoolHelper::Section DeviceDescriptorObjectPoolHelper::parse_section(DeviceDescriptorObjectPool &ddop,
	                                                                                          std::shared_ptr<task_controller_object::DeviceElementObject> elementObject)
	{
		Section retVal;

		for (std::uint16_t i = 0; i < elementObject->get_number_child_objects(); i++)
		{
			auto sectionChildObject = ddop.get_object_by_id(elementObject->get_child_object_id(i));

			if (nullptr != sectionChildObject)
			{
				if (task_controller_object::ObjectTypes::DeviceProperty == sectionChildObject->get_object_type())
				{
					auto property = std::static_pointer_cast<task_controller_object::DevicePropertyObject>(sectionChildObject);
					setValueFromProperty(retVal.xOffset_mm, property, DataDescriptionIndex::DeviceElementOffsetX);
					setValueFromProperty(retVal.yOffset_mm, property, DataDescriptionIndex::DeviceElementOffsetY);
					setValueFromProperty(retVal.zOffset_mm, property, DataDescriptionIndex::DeviceElementOffsetZ);
					setValueFromProperty(retVal.width_mm, property, DataDescriptionIndex::ActualWorkingWidth);
				}
				else if (task_controller_object::ObjectTypes::DeviceProcessData == sectionChildObject->get_object_type())
				{
					auto processData = std::static_pointer_cast<task_controller_object::DeviceProcessDataObject>(sectionChildObject);
					setEditableFromProcessData(retVal.xOffset_mm, processData, DataDescriptionIndex::DeviceElementOffsetX);
					setEditableFromProcessData(retVal.yOffset_mm, processData, DataDescriptionIndex::DeviceElementOffsetY);
					setEditableFromProcessData(retVal.zOffset_mm, processData, DataDescriptionIndex::DeviceElementOffsetZ);
					setEditableFromProcessData(retVal.width_mm, processData, DataDescriptionIndex::ActualWorkingWidth);
				}
			}
		}
		return retVal;
	}

	DeviceDescriptorObjectPoolHelper::SubBoom DeviceDescriptorObjectPoolHelper::parse_sub_boom(DeviceDescriptorObjectPool &ddop,
	                                                                                           std::shared_ptr<task_controller_object::DeviceElementObject> elementObject)
	{
		SubBoom retVal;

		// Find all sections in this sub boom
		// We again have to search the whole pool because elements have parent links, not child links
		for (std::uint16_t i = 0; i < ddop.size(); i++)
		{
			auto section = ddop.get_object_by_index(static_cast<std::uint16_t>(i));

			if ((nullptr != section) &&
			    (task_controller_object::ObjectTypes::DeviceElement == section->get_object_type()) &&
			    (task_controller_object::DeviceElementObject::Type::Section == std::static_pointer_cast<task_controller_object::DeviceElementObject>(section)->get_type()) &&
			    (std::static_pointer_cast<task_controller_object::DeviceElementObject>(section)->get_parent_object() == elementObject->get_object_id()))
			{
				retVal.sections.push_back(parse_section(ddop, std::static_pointer_cast<task_controller_object::DeviceElementObject>(section)));
			}
		}

		// Process child DDIs of this sub boom to locate offset and width
		for (std::uint16_t i = 0; i < elementObject->get_number_child_objects(); i++)
		{
			auto childObject = ddop.get_object_by_id(elementObject->get_child_object_id(i));

			if (nullptr != childObject)
			{
				if (task_controller_object::ObjectTypes::DeviceProperty == childObject->get_object_type())
				{
					auto property = std::static_pointer_cast<task_controller_object::DevicePropertyObject>(childObject);
					setValueFromProperty(retVal.xOffset_mm, property, DataDescriptionIndex::DeviceElementOffsetX);
					setValueFromProperty(retVal.yOffset_mm, property, DataDescriptionIndex::DeviceElementOffsetY);
					setValueFromProperty(retVal.zOffset_mm, property, DataDescriptionIndex::DeviceElementOffsetZ);
					setValueFromProperty(retVal.width_mm, property, DataDescriptionIndex::ActualWorkingWidth);
				}
				else if (task_controller_object::ObjectTypes::DeviceProcessData == childObject->get_object_type())
				{
					auto processData = std::static_pointer_cast<task_controller_object::DeviceProcessDataObject>(childObject);
					setEditableFromProcessData(retVal.xOffset_mm, processData, DataDescriptionIndex::DeviceElementOffsetX);
					setEditableFromProcessData(retVal.yOffset_mm, processData, DataDescriptionIndex::DeviceElementOffsetY);
					setEditableFromProcessData(retVal.zOffset_mm, processData, DataDescriptionIndex::DeviceElementOffsetZ);
					setEditableFromProcessData(retVal.width_mm, processData, DataDescriptionIndex::ActualWorkingWidth);
				}
			}
		}
		return retVal;
	}

	void DeviceDescriptorObjectPoolHelper::setValueFromProperty(ObjectPoolValue &objectPoolValue,
	                                                            const std::shared_ptr<task_controller_object::DevicePropertyObject> &property,
	                                                            DataDescriptionIndex ddi)
	{
		if (property->get_ddi() == static_cast<std::uint16_t>(ddi))
		{
			objectPoolValue.value = property->get_value();
			objectPoolValue.isValuePresent = true;
		}
	}

	void DeviceDescriptorObjectPoolHelper::setEditableFromProcessData(ObjectPoolValue &objectPoolValue,
	                                                                  const std::shared_ptr<task_controller_object::DeviceProcessDataObject> &processData,
	                                                                  DataDescriptionIndex ddi)
	{
		if (processData->get_ddi() == static_cast<std::uint16_t>(ddi))
		{
			objectPoolValue.isSettable = true;
		}
	}
} // namespace isobus
