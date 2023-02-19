//================================================================================================
/// @file isobus_device_descriptor_object_pool.cpp
///
/// @brief Implements an interface for creating a Task Controller DDOP.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/isobus_device_descriptor_object_pool.hpp"

#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/to_string.hpp"

#include <cassert>

namespace isobus
{
	DeviceDescriptorObjectPool::DeviceDescriptorObjectPool(std::uint8_t taskControllerServerVersion) :
	  taskControllerCompatabilityLevel(taskControllerServerVersion)
	{
		assert(taskControllerCompatabilityLevel <= MAX_TC_VERSION_SUPPORTED);
	}

	bool DeviceDescriptorObjectPool::add_device(std::string deviceDesignator,
	                                            std::string deviceSoftwareVersion,
	                                            std::string deviceSerialNumber,
	                                            std::string deviceStructureLabel,
	                                            std::array<std::uint8_t, task_controller_object::DeviceObject::MAX_STRUCTURE_AND_LOCALIZATION_LABEL_LENGTH> deviceLocalizationLabel,
	                                            std::vector<std::uint8_t> deviceExtendedStructureLabel,
	                                            std::uint64_t clientIsoNAME)
	{
		bool retVal = true;

		for (auto &currentObject : objectList)
		{
			if (task_controller_object::ObjectTypes::Device == currentObject->get_object_type())
			{
				retVal = false;
				break;
			}
		}

		if (retVal)
		{
			if ((taskControllerCompatabilityLevel < MAX_TC_VERSION_SUPPORTED) &&
			    (deviceDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device designator " +
				                     deviceDesignator +
				                     " is greater than the max byte length of 32. Value will be truncated.");
				deviceDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH);
			}
			else if ((taskControllerCompatabilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (deviceDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device designator " +
				                     deviceDesignator +
				                     " is greater than the max byte length of 128. Value will be truncated.");
				deviceDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LENGTH);
			}
			else if ((taskControllerCompatabilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (deviceDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				CANStackLogger::info("[DDOP]: Device designator " +
				                     deviceDesignator +
				                     " byte length is greater than the max character count of 32. " +
				                     "This is only acceptable if you have 32 or fewer UTF-8 characters!" +
				                     " Please verify your DDOP configuration meets this requirement.");
			}

			if ((taskControllerCompatabilityLevel < MAX_TC_VERSION_SUPPORTED) &&
			    (deviceSerialNumber.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device serial number " +
				                     deviceSerialNumber +
				                     " is greater than the max byte length of 32. Value will be truncated.");
				deviceSerialNumber.resize(task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH);
			}
			else if ((taskControllerCompatabilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (deviceSerialNumber.size() > task_controller_object::Object::MAX_DESIGNATOR_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device serial number " +
				                     deviceSerialNumber +
				                     " is greater than the max byte length of 128. Value will be truncated.");
				deviceSerialNumber.resize(task_controller_object::Object::MAX_DESIGNATOR_LENGTH);
			}
			else if ((taskControllerCompatabilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (deviceSerialNumber.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				CANStackLogger::info("[DDOP]: Device serial number " +
				                     deviceSerialNumber +
				                     " byte length is greater than the max character count of 32. " +
				                     "This is only acceptable if you have 32 or fewer UTF-8 characters!" +
				                     " Please verify your DDOP configuration meets this requirement.");
			}

			if (deviceStructureLabel.size() > task_controller_object::DeviceObject::MAX_STRUCTURE_AND_LOCALIZATION_LABEL_LENGTH)
			{
				CANStackLogger::warn("[DDOP]: Device structure label " +
				                     deviceStructureLabel +
				                     " is greater than the max length of 7. Value will be truncated.");
				deviceStructureLabel.resize(task_controller_object::DeviceObject::MAX_STRUCTURE_AND_LOCALIZATION_LABEL_LENGTH);
			}
			if (deviceExtendedStructureLabel.size() > task_controller_object::DeviceObject::MAX_EXTENDED_STRUCTURE_LABEL_LENGTH)
			{
				CANStackLogger::warn("[DDOP]: Device extended structure label is greater than the max length of 32. Value will be truncated.");
				deviceExtendedStructureLabel.resize(task_controller_object::DeviceObject::MAX_EXTENDED_STRUCTURE_LABEL_LENGTH);
			}

			if (deviceLocalizationLabel[6] != 0xFF)
			{
				CANStackLogger::warn("[DDOP]: Device localization label byte 7 must be the reserved value 0xFF. This value will be enforced when DDOP binary is generated.");
			}
			objectList.emplace_back(new task_controller_object::DeviceObject(deviceDesignator,
			                                                                 deviceSoftwareVersion,
			                                                                 deviceSerialNumber,
			                                                                 deviceStructureLabel,
			                                                                 deviceLocalizationLabel,
			                                                                 deviceExtendedStructureLabel,
			                                                                 clientIsoNAME));
		}
		else
		{
			CANStackLogger::error("[DDOP]: Cannot add more than 1 Device object to a DDOP.");
		}
		return retVal;
	}

	bool DeviceDescriptorObjectPool::add_device_element(std::string deviceElementDesignator,
	                                                    std::uint16_t deviceElementNumber,
	                                                    std::uint16_t parentObjectID,
	                                                    task_controller_object::DeviceElementObject::Type deviceEelementType,
	                                                    std::uint16_t uniqueID)
	{
		bool retVal = check_object_id_unique(uniqueID);

		if (retVal)
		{
			// Object will be added
			// Check for warnings
			if ((taskControllerCompatabilityLevel < MAX_TC_VERSION_SUPPORTED) &&
			    (deviceElementDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device element designator " +
				                     deviceElementDesignator +
				                     " is greater than the max length of 32. Value will be truncated.");
				deviceElementDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH);
			}
			if ((taskControllerCompatabilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			    (deviceElementDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device element designator " +
				                     deviceElementDesignator +
				                     " is greater than the max length of 128. Value will be truncated.");
				deviceElementDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LENGTH);
			}

			objectList.emplace_back(new task_controller_object::DeviceElementObject(deviceElementDesignator,
			                                                                        deviceElementNumber,
			                                                                        parentObjectID,
			                                                                        deviceEelementType,
			                                                                        uniqueID));
		}
		else
		{
			CANStackLogger::error("[DDOP]: Device element ID " +
			                      isobus::to_string(static_cast<int>(uniqueID)) +
			                      " is not unique. Object will not be added to the DDOP.");
		}
		return retVal;
	}

	bool DeviceDescriptorObjectPool::add_device_process_data(std::string processDataDesignator,
	                                                         std::uint16_t processDataDDI,
	                                                         std::uint16_t deviceValuePresentationObjectID,
	                                                         std::uint8_t processDataProperties,
	                                                         std::uint8_t processDataTriggerMethods,
	                                                         std::uint16_t uniqueID)
	{
		bool retVal = check_object_id_unique(uniqueID);

		if (retVal)
		{
			// Object will be added
			// Check for warnings
			if ((processDataProperties & 0x02) && (processDataProperties & 0x04))
			{
				CANStackLogger::warn("[DDOP]: Process data object " +
				                     isobus::to_string(static_cast<int>(uniqueID)) +
				                     " has mutually exclusive options 'settable' and 'control source' set.");
			}

			if ((taskControllerCompatabilityLevel < MAX_TC_VERSION_SUPPORTED) &&
			    (processDataDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device process data designator " +
				                     processDataDesignator +
				                     " is greater than the max byte length of 32. Value will be truncated.");
				processDataDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH);
			}
			else if ((taskControllerCompatabilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (processDataDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device process data designator " +
				                     processDataDesignator +
				                     " is greater than the max byte length of 128. Value will be truncated.");
				processDataDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LENGTH);
			}
			else if ((taskControllerCompatabilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (processDataDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				CANStackLogger::info("[DDOP]: Device process data designator " +
				                     processDataDesignator +
				                     " byte length is greater than the max character count of 32. " +
				                     "This is only acceptable if you have 32 or fewer UTF-8 characters!" +
				                     " Please verify your DDOP configuration meets this requirement.");
			}

			objectList.emplace_back(new task_controller_object::DeviceProcessDataObject(processDataDesignator,
			                                                                            processDataDDI,
			                                                                            deviceValuePresentationObjectID,
			                                                                            processDataProperties,
			                                                                            processDataTriggerMethods,
			                                                                            uniqueID));
		}
		else
		{
			CANStackLogger::error("[DDOP]: Device process data ID " +
			                      isobus::to_string(static_cast<int>(uniqueID)) +
			                      " is not unique. Object will not be added to the DDOP.");
		}
		return retVal;
	}

	bool DeviceDescriptorObjectPool::add_device_property(std::string propertyDesignator,
	                                                     std::int32_t propertyValue,
	                                                     std::uint16_t propertyDDI,
	                                                     std::uint16_t valuePresentationObject,
	                                                     std::uint16_t uniqueID)
	{
		bool retVal = check_object_id_unique(uniqueID);

		if (retVal)
		{
			// Object will be added
			// Check for warnings
			if ((taskControllerCompatabilityLevel < MAX_TC_VERSION_SUPPORTED) &&
			    (propertyDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device property designator " +
				                     propertyDesignator +
				                     " is greater than the max byte length of 32. Value will be truncated.");
				propertyDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH);
			}
			else if ((taskControllerCompatabilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (propertyDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device property designator " +
				                     propertyDesignator +
				                     " is greater than the max byte length of 128. Value will be truncated.");
				propertyDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LENGTH);
			}
			else if ((taskControllerCompatabilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (propertyDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				CANStackLogger::info("[DDOP]: Device property designator " +
				                     propertyDesignator +
				                     " byte length is greater than the max character count of 32. " +
				                     "This is only acceptable if you have 32 or fewer UTF-8 characters!" +
				                     " Please verify your DDOP configuration meets this requirement.");
			}

			objectList.emplace_back(new task_controller_object::DevicePropertyObject(propertyDesignator,
			                                                                         propertyValue,
			                                                                         propertyDDI,
			                                                                         valuePresentationObject,
			                                                                         uniqueID));
		}
		else
		{
			CANStackLogger::error("[DDOP]: Device property ID " +
			                      isobus::to_string(static_cast<int>(uniqueID)) +
			                      " is not unique. Object will not be added to the DDOP.");
		}
		return retVal;
	}

	bool DeviceDescriptorObjectPool::add_device_value_presentation(std::string unitDesignator,
	                                                               std::int32_t offsetValue,
	                                                               float scaleFactor,
	                                                               std::uint8_t numberDecimals,
	                                                               std::uint16_t uniqueID)
	{
		bool retVal = check_object_id_unique(uniqueID);

		if (retVal)
		{
			// Object will be added
			// Check for warnings
			if ((taskControllerCompatabilityLevel < MAX_TC_VERSION_SUPPORTED) &&
			    (unitDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device value presentation unit designator " +
				                     unitDesignator +
				                     " is greater than the max byte length of 32. Value will be truncated.");
				unitDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH);
			}
			else if ((taskControllerCompatabilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (unitDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device value presentation unit designator " +
				                     unitDesignator +
				                     " is greater than the max byte length of 128. Value will be truncated.");
				unitDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LENGTH);
			}
			else if ((taskControllerCompatabilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (unitDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				CANStackLogger::info("[DDOP]: Device value presentation unit designator " +
				                     unitDesignator +
				                     " byte length is greater than the max character count of 32. " +
				                     "This is only acceptable if you have 32 or fewer UTF-8 characters!" +
				                     " Please verify your DDOP configuration meets this requirement.");
			}

			objectList.emplace_back(new task_controller_object::DeviceValuePresentationObject(unitDesignator,
			                                                                                  offsetValue,
			                                                                                  scaleFactor,
			                                                                                  numberDecimals,
			                                                                                  uniqueID));
		}
		else
		{
			CANStackLogger::error("[DDOP]: Device value presentation object ID " +
			                      isobus::to_string(static_cast<int>(uniqueID)) +
			                      " is not unique. Object will not be added to the DDOP.");
		}
		return retVal;
	}

	bool DeviceDescriptorObjectPool::generate_binary_object_pool(std::vector<std::uint8_t> &resultantPool)
	{
		bool retVal = true;

		resultantPool.clear();

		if (taskControllerCompatabilityLevel > MAX_TC_VERSION_SUPPORTED)
		{
			CANStackLogger::warn("[DDOP]: A DDOP is being generated for a TC version that is unsupported. This may cause issues.");
		}

		if (resolve_parent_ids_to_objects())
		{
			retVal = true;
			for (auto &currentObject : objectList)
			{
				auto objectBinary = currentObject->get_binary_object();

				if (objectBinary.size() != 0)
				{
					resultantPool.insert(resultantPool.end(), objectBinary.begin(), objectBinary.end());
				}
				else
				{
					CANStackLogger::error("[DDOP]: Failed to create all object binaries. Your DDOP is invalid.");
					retVal = false;
					break;
				}
			}
		}
		else
		{
			CANStackLogger::error("[DDOP]: Failed to resolve all object IDs in DDOP. Your DDOP contains invalid object references.");
		}
		return retVal;
	}

	task_controller_object::Object *DeviceDescriptorObjectPool::get_object_by_id(std::uint16_t objectID)
	{
		task_controller_object::Object *retVal = nullptr;

		for (auto &currentObject : objectList)
		{
			if (currentObject->get_object_id() == objectID)
			{
				retVal = currentObject.get();
				break;
			}
		}
		return retVal;
	}

	bool DeviceDescriptorObjectPool::resolve_parent_ids_to_objects()
	{
		bool retVal = true;

		for (auto &currentObject : objectList)
		{
			assert(nullptr != currentObject);
			switch (currentObject->get_object_type())
			{
				case task_controller_object::ObjectTypes::DeviceElement:
				{
					// Process parent object
					auto currentDeviceElement = reinterpret_cast<task_controller_object::DeviceElementObject *>(currentObject.get());
					if (task_controller_object::Object::NULL_OBJECT_ID != currentDeviceElement->get_parent_object())
					{
						task_controller_object::Object *parent = get_object_by_id(currentDeviceElement->get_parent_object());
						if (nullptr != parent)
						{
							switch (parent->get_object_type())
							{
								case task_controller_object::ObjectTypes::DeviceElement:
								case task_controller_object::ObjectTypes::Device:
								{
									// Device and device element are allowed
								}
								break;

								default:
								{
									CANStackLogger::error("[DDOP]: Object " +
									                      isobus::to_string(static_cast<int>(currentObject->get_object_id())) +
									                      " has an invalid parent object type. Only device element objects or device objects may be its parent.");
									retVal = false;
								}
								break;
							}
						}
						else
						{
							CANStackLogger::error("[DDOP]: Object " +
							                      isobus::to_string(static_cast<int>(currentDeviceElement->get_parent_object())) +
							                      " is not found.");
							retVal = false;
						}
					}
					else
					{
						CANStackLogger::warn("[DDOP]: Object " +
						                     isobus::to_string(static_cast<int>(currentObject->get_object_id())) +
						                     " is an orphan. It's parent is 0xFFFF!");
					}

					if (retVal)
					{
						// Process children now that parent has been validated
						for (std::size_t i = 0; i < currentDeviceElement->get_number_child_objects(); i++)
						{
							task_controller_object::Object *child = get_object_by_id(currentDeviceElement->get_child_object_id(i));
							if (nullptr == child)
							{
								CANStackLogger::error("[DDOP]: Object " +
								                      isobus::to_string(static_cast<int>(currentDeviceElement->get_child_object_id(i))) +
								                      " is not found.");
								retVal = false;
								break;
							}
						}
					}
				}
				break;

				case task_controller_object::ObjectTypes::DeviceProcessData:
				{
					auto currentProcessData = reinterpret_cast<task_controller_object::DeviceProcessDataObject *>(currentObject.get());

					if (task_controller_object::Object::NULL_OBJECT_ID != currentProcessData->get_device_value_presentation_object_id())
					{
						task_controller_object::Object *child = get_object_by_id(currentProcessData->get_device_value_presentation_object_id());
						if (nullptr == child)
						{
							CANStackLogger::error("[DDOP]: Object " +
							                      isobus::to_string(static_cast<int>(currentProcessData->get_device_value_presentation_object_id())) +
							                      " is not found.");
							retVal = false;
							break;
						}
					}
				}
				break;

				case task_controller_object::ObjectTypes::DeviceProperty:
				{
					auto currentProperty = reinterpret_cast<task_controller_object::DevicePropertyObject *>(currentObject.get());

					if (task_controller_object::Object::NULL_OBJECT_ID != currentProperty->get_device_value_presentation_object_id())
					{
						task_controller_object::Object *child = get_object_by_id(currentProperty->get_device_value_presentation_object_id());
						if (nullptr == child)
						{
							CANStackLogger::error("[DDOP]: Object " +
							                      isobus::to_string(static_cast<int>(currentProperty->get_device_value_presentation_object_id())) +
							                      " is not found.");
							retVal = false;
							break;
						}
					}
				}
				break;

				default:
				{
					// This object has no child/parent to validate
				}
				break;
			}

			if (!retVal)
			{
				break;
			}
		}
		return retVal;
	}

	bool DeviceDescriptorObjectPool::check_object_id_unique(std::uint16_t uniqueID) const
	{
		bool retVal = true;

		if ((0 != uniqueID) && (task_controller_object::Object::NULL_OBJECT_ID != uniqueID))
		{
			for (auto &currentObject : objectList)
			{
				if (uniqueID == currentObject->get_object_id())
				{
					retVal = false;
					break;
				}
			}
		}
		else
		{
			retVal = false;
		}
		return retVal;
	}

} // namespace isobus
