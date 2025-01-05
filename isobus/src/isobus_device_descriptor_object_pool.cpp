//================================================================================================
/// @file isobus_device_descriptor_object_pool.cpp
///
/// @brief Implements an interface for creating a Task Controller DDOP.
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/isobus_device_descriptor_object_pool.hpp"

#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/platform_endianness.hpp"
#include "isobus/utility/to_string.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace isobus
{
	DeviceDescriptorObjectPool::DeviceDescriptorObjectPool(std::uint8_t taskControllerServerVersion) :
	  taskControllerCompatibilityLevel(taskControllerServerVersion)
	{
		assert(taskControllerCompatibilityLevel <= MAX_TC_VERSION_SUPPORTED);
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

		for (const auto &currentObject : objectList)
		{
			if (task_controller_object::ObjectTypes::Device == currentObject->get_object_type())
			{
				retVal = false;
				break;
			}
		}

		if (retVal)
		{
			if ((taskControllerCompatibilityLevel < MAX_TC_VERSION_SUPPORTED) &&
			    (deviceDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				LOG_WARNING("[DDOP]: Device designator " +
				            deviceDesignator +
				            " is greater than the max byte length of 32. Value will be truncated.");
				deviceDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH);
			}
			else if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (deviceDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LENGTH))
			{
				LOG_WARNING("[DDOP]: Device designator " +
				            deviceDesignator +
				            " is greater than the max byte length of 128. Value will be truncated.");
				deviceDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LENGTH);
			}
			else if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (deviceDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				LOG_INFO("[DDOP]: Device designator " +
				         deviceDesignator +
				         " byte length is greater than the max character count of 32. " +
				         "This is only acceptable if you have 32 or fewer UTF-8 characters!" +
				         " Please verify your DDOP configuration meets this requirement.");
			}

			if ((taskControllerCompatibilityLevel < MAX_TC_VERSION_SUPPORTED) &&
			    (deviceSerialNumber.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				LOG_WARNING("[DDOP]: Device serial number " +
				            deviceSerialNumber +
				            " is greater than the max byte length of 32. Value will be truncated.");
				deviceSerialNumber.resize(task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH);
			}
			else if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (deviceSerialNumber.size() > task_controller_object::Object::MAX_DESIGNATOR_LENGTH))
			{
				LOG_WARNING("[DDOP]: Device serial number " +
				            deviceSerialNumber +
				            " is greater than the max byte length of 128. Value will be truncated.");
				deviceSerialNumber.resize(task_controller_object::Object::MAX_DESIGNATOR_LENGTH);
			}
			else if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (deviceSerialNumber.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				LOG_INFO("[DDOP]: Device serial number " +
				         deviceSerialNumber +
				         " byte length is greater than the max character count of 32. " +
				         "This is only acceptable if you have 32 or fewer UTF-8 characters!" +
				         " Please verify your DDOP configuration meets this requirement.");
			}

			if (deviceStructureLabel.size() > task_controller_object::DeviceObject::MAX_STRUCTURE_AND_LOCALIZATION_LABEL_LENGTH)
			{
				LOG_WARNING("[DDOP]: Device structure label " +
				            deviceStructureLabel +
				            " is greater than the max length of 7. Value will be truncated.");
				deviceStructureLabel.resize(task_controller_object::DeviceObject::MAX_STRUCTURE_AND_LOCALIZATION_LABEL_LENGTH);
			}
			if (deviceExtendedStructureLabel.size() > task_controller_object::DeviceObject::MAX_EXTENDED_STRUCTURE_LABEL_LENGTH)
			{
				LOG_WARNING("[DDOP]: Device extended structure label is greater than the max length of 32. Value will be truncated.");
				deviceExtendedStructureLabel.resize(task_controller_object::DeviceObject::MAX_EXTENDED_STRUCTURE_LABEL_LENGTH);
			}

			if (deviceLocalizationLabel[6] != 0xFF)
			{
				LOG_WARNING("[DDOP]: Device localization label byte 7 must be the reserved value 0xFF. This value will be enforced when DDOP binary is generated.");
			}
			objectList.emplace_back(new task_controller_object::DeviceObject(deviceDesignator,
			                                                                 deviceSoftwareVersion,
			                                                                 deviceSerialNumber,
			                                                                 deviceStructureLabel,
			                                                                 deviceLocalizationLabel,
			                                                                 deviceExtendedStructureLabel,
			                                                                 clientIsoNAME,
			                                                                 (taskControllerCompatibilityLevel >= 4)));
		}
		else
		{
			LOG_ERROR("[DDOP]: Cannot add more than 1 Device object to a DDOP.");
		}
		return retVal;
	}

	bool DeviceDescriptorObjectPool::add_device_element(std::string deviceElementDesignator,
	                                                    std::uint16_t deviceElementNumber,
	                                                    std::uint16_t parentObjectID,
	                                                    task_controller_object::DeviceElementObject::Type deviceElementType,
	                                                    std::uint16_t uniqueID)
	{
		bool retVal = check_object_id_unique(uniqueID);

		if (retVal)
		{
			// Object will be added
			// Check for warnings
			if ((taskControllerCompatibilityLevel < MAX_TC_VERSION_SUPPORTED) &&
			    (deviceElementDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				LOG_WARNING("[DDOP]: Device element designator " +
				            deviceElementDesignator +
				            " is greater than the max length of 32. Value will be truncated.");
				deviceElementDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH);
			}
			if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			    (deviceElementDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LENGTH))
			{
				LOG_WARNING("[DDOP]: Device element designator " +
				            deviceElementDesignator +
				            " is greater than the max length of 128. Value will be truncated.");
				deviceElementDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LENGTH);
			}

			objectList.emplace_back(new task_controller_object::DeviceElementObject(deviceElementDesignator,
			                                                                        deviceElementNumber,
			                                                                        parentObjectID,
			                                                                        deviceElementType,
			                                                                        uniqueID));
		}
		else
		{
			LOG_ERROR("[DDOP]: Device element ID " +
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
				LOG_WARNING("[DDOP]: Process data object " +
				            isobus::to_string(static_cast<int>(uniqueID)) +
				            " has mutually exclusive options 'settable' and 'control source' set.");
			}

			if ((taskControllerCompatibilityLevel < MAX_TC_VERSION_SUPPORTED) &&
			    (processDataDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				LOG_WARNING("[DDOP]: Device process data designator " +
				            processDataDesignator +
				            " is greater than the max byte length of 32. Value will be truncated.");
				processDataDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH);
			}
			else if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (processDataDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LENGTH))
			{
				LOG_WARNING("[DDOP]: Device process data designator " +
				            processDataDesignator +
				            " is greater than the max byte length of 128. Value will be truncated.");
				processDataDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LENGTH);
			}
			else if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (processDataDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				LOG_INFO("[DDOP]: Device process data designator " +
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
			LOG_ERROR("[DDOP]: Device process data ID " +
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
			if ((taskControllerCompatibilityLevel < MAX_TC_VERSION_SUPPORTED) &&
			    (propertyDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				LOG_WARNING("[DDOP]: Device property designator " +
				            propertyDesignator +
				            " is greater than the max byte length of 32. Value will be truncated.");
				propertyDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH);
			}
			else if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (propertyDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LENGTH))
			{
				LOG_WARNING("[DDOP]: Device property designator " +
				            propertyDesignator +
				            " is greater than the max byte length of 128. Value will be truncated.");
				propertyDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LENGTH);
			}
			else if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (propertyDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				LOG_INFO("[DDOP]: Device property designator " +
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
			LOG_ERROR("[DDOP]: Device property ID " +
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
			if ((taskControllerCompatibilityLevel < MAX_TC_VERSION_SUPPORTED) &&
			    (unitDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				LOG_WARNING("[DDOP]: Device value presentation unit designator " +
				            unitDesignator +
				            " is greater than the max byte length of 32. Value will be truncated.");
				unitDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH);
			}
			else if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (unitDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LENGTH))
			{
				LOG_WARNING("[DDOP]: Device value presentation unit designator " +
				            unitDesignator +
				            " is greater than the max byte length of 128. Value will be truncated.");
				unitDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LENGTH);
			}
			else if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (unitDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				LOG_INFO("[DDOP]: Device value presentation unit designator " +
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
			LOG_ERROR("[DDOP]: Device value presentation object ID " +
			          isobus::to_string(static_cast<int>(uniqueID)) +
			          " is not unique. Object will not be added to the DDOP.");
		}
		return retVal;
	}

	bool DeviceDescriptorObjectPool::remove_objects_with_type(task_controller_object::ObjectTypes objectType)
	{
		return remove_where([objectType](const task_controller_object::Object &object) { return object.get_object_type() == objectType; });
	}

	bool DeviceDescriptorObjectPool::remove_object_with_id(std::uint16_t objectID)
	{
		return remove_where([objectID](const task_controller_object::Object &object) { return object.get_object_id() == objectID; });
	}

	bool DeviceDescriptorObjectPool::remove_where(std::function<bool(const task_controller_object::Object &)> predicate)
	{
		bool retVal = false;

		for (auto it = objectList.begin(); it != objectList.end();)
		{
			if (predicate(*(*it)))
			{
				it = objectList.erase(it);
				retVal = true;
			}
			else
			{
				++it;
			}
		}
		return retVal;
	}

	bool DeviceDescriptorObjectPool::deserialize_binary_object_pool(std::vector<std::uint8_t> &binaryPool, NAME clientNAME)
	{
		return deserialize_binary_object_pool(binaryPool.data(), static_cast<std::uint32_t>(binaryPool.size()), clientNAME);
	}

	bool DeviceDescriptorObjectPool::deserialize_binary_object_pool(const std::uint8_t *binaryPool, std::uint32_t binaryPoolSizeBytes, NAME clientNAME)
	{
		bool retVal = true;

		if ((nullptr != binaryPool) && (0 != binaryPoolSizeBytes))
		{
			LOG_DEBUG("[DDOP]: Attempting to deserialize a binary object pool with size %u.", binaryPoolSizeBytes);

			// Iterate over the DDOP and convert to objects.
			// Using the size to track how much is left to process.
			while (0 != binaryPoolSizeBytes)
			{
				// Verify there's enough data to read the XML namespace
				if (binaryPoolSizeBytes > 3)
				{
					std::string xmlNameSpace;
					xmlNameSpace.push_back(static_cast<char>(binaryPool[0]));
					xmlNameSpace.push_back(static_cast<char>(binaryPool[1]));
					xmlNameSpace.push_back(static_cast<char>(binaryPool[2]));

					if ("DVC" == xmlNameSpace)
					{
						// Process a Device Object
						std::uint8_t numberDesignatorBytes = 0; // Labelled "N" in 11783-10 table A.1
						std::uint8_t numberSoftwareVersionBytes = 0; // Labelled "M" in 11783-10 table A.1
						std::uint8_t numberDeviceSerialNumberBytes = 0; // Labelled "O" in 11783-10 table A.1
						std::uint8_t numberExtendedStructureLabelBytes = 0; // Version 4+ only

						if ((binaryPoolSizeBytes >= 6) &&
						    (binaryPool[5] < 128))
						{
							numberDesignatorBytes = binaryPool[5];
						}
						else
						{
							LOG_ERROR("[DDOP]: Binary device object designator has invalid length.");
							retVal = false;
						}

						if ((binaryPoolSizeBytes >= static_cast<std::uint32_t>(7 + numberDesignatorBytes)) &&
						    (binaryPool[6 + numberDesignatorBytes] < 128))
						{
							numberSoftwareVersionBytes = binaryPool[6 + numberDesignatorBytes];
						}
						else
						{
							LOG_ERROR("[DDOP]: Binary device object software version has invalid length.");
							retVal = false;
						}

						if ((binaryPoolSizeBytes >= static_cast<std::uint32_t>(16 + numberDesignatorBytes + numberSoftwareVersionBytes)) &&
						    (binaryPool[15 + numberDesignatorBytes + numberSoftwareVersionBytes] < 128))
						{
							numberDeviceSerialNumberBytes = binaryPool[15 + numberDesignatorBytes + numberSoftwareVersionBytes];
						}
						else
						{
							LOG_ERROR("[DDOP]: Binary device object serial number has invalid length.");
							retVal = false;
						}

						if (taskControllerCompatibilityLevel >= 4)
						{
							if ((binaryPoolSizeBytes >= static_cast<std::uint32_t>(31 + numberDeviceSerialNumberBytes + numberDesignatorBytes + numberSoftwareVersionBytes)) &&
							    (binaryPool[30 + numberDeviceSerialNumberBytes + numberDesignatorBytes + numberSoftwareVersionBytes] <= 32))
							{
								numberExtendedStructureLabelBytes = binaryPool[30 + numberDeviceSerialNumberBytes + numberDesignatorBytes + numberSoftwareVersionBytes];
							}
							else
							{
								LOG_ERROR("[DDOP]: Binary device object with version 4 contains invalid extended structure label length.");
								retVal = false;
							}
						}

						std::uint32_t expectedSize = (31 + numberDeviceSerialNumberBytes + numberDesignatorBytes + numberSoftwareVersionBytes + numberExtendedStructureLabelBytes);

						if (taskControllerCompatibilityLevel < 4)
						{
							expectedSize--; // One byte less due to no extended structure label length
						}

						if ((binaryPoolSizeBytes >= expectedSize) && retVal)
						{
							std::string deviceDesignator;
							std::string deviceSoftwareVersion;
							std::string deviceSerialNumber;
							std::string deviceStructureLabel;
							std::array<std::uint8_t, 7> localizationLabel;
							std::vector<std::uint8_t> extendedStructureLabel;
							std::uint64_t ddopClientNAME = 0;

							for (std::uint16_t i = 0; i < numberDesignatorBytes; i++)
							{
								deviceDesignator.push_back(binaryPool[6 + i]);
							}

							for (std::uint16_t i = 0; i < numberSoftwareVersionBytes; i++)
							{
								deviceSoftwareVersion.push_back(binaryPool[7 + numberDesignatorBytes + i]);
							}

							for (std::uint8_t i = 0; i < 8; i++)
							{
								std::uint64_t currentNameByte = binaryPool[7 + numberDesignatorBytes + numberSoftwareVersionBytes + i];
								ddopClientNAME |= (currentNameByte << (8 * i));
							}

							if ((0 != clientNAME.get_full_name()) && (ddopClientNAME != clientNAME.get_full_name()))
							{
								LOG_ERROR("[DDOP]: Failed adding deserialized device object. DDOP NAME doesn't match client's actual NAME.");
								retVal = false;
							}
							else if (0 == clientNAME.get_full_name())
							{
								clientNAME.set_full_name(ddopClientNAME);
							}

							for (std::uint16_t i = 0; i < numberDeviceSerialNumberBytes; i++)
							{
								deviceSerialNumber.push_back(binaryPool[16 + numberDesignatorBytes + numberSoftwareVersionBytes + i]);
							}

							for (std::uint16_t i = 0; i < 7; i++)
							{
								deviceStructureLabel.push_back(binaryPool[16 + numberDeviceSerialNumberBytes + numberDesignatorBytes + numberSoftwareVersionBytes + i]);
							}

							for (std::uint16_t i = 0; i < 7; i++)
							{
								localizationLabel.at(i) = (binaryPool[23 + numberDeviceSerialNumberBytes + numberDesignatorBytes + numberSoftwareVersionBytes + i]);
							}

							for (std::uint16_t i = 0; i < numberExtendedStructureLabelBytes; i++)
							{
								extendedStructureLabel.push_back(binaryPool[31 + numberDeviceSerialNumberBytes + numberDesignatorBytes + numberSoftwareVersionBytes + i]);
							}

							remove_objects_with_type(task_controller_object::ObjectTypes::Device); // Make sure the previous device object is removed
							if (add_device(deviceDesignator, deviceSoftwareVersion, deviceSerialNumber, deviceStructureLabel, localizationLabel, extendedStructureLabel, clientNAME.get_full_name()))
							{
								binaryPoolSizeBytes -= expectedSize;
								binaryPool += expectedSize;
							}
							else
							{
								LOG_ERROR("[DDOP]: Failed adding deserialized device object. DDOP schema is not valid.");
								retVal = false;
							}
						}
						else
						{
							LOG_ERROR("[DDOP]: Not enough binary DDOP data left to parse device object. DDOP schema is not valid");
							retVal = false;
						}
					}
					else if ("DET" == xmlNameSpace)
					{
						// Process a device element object
						std::uint8_t numberDesignatorBytes = 0;
						std::uint8_t numberOfObjectIDs = 0;

						if (binaryPoolSizeBytes >= 7)
						{
							numberDesignatorBytes = binaryPool[6];
						}
						else
						{
							LOG_ERROR("[DDOP]: Binary device element object has invalid length.");
							retVal = false;
						}

						if (binaryPoolSizeBytes >= static_cast<std::uint32_t>(12 + numberDesignatorBytes))
						{
							numberOfObjectIDs = binaryPool[11 + numberDesignatorBytes];
						}
						else
						{
							LOG_ERROR("[DDOP]: Binary device element object has invalid length to process referenced object IDs.");
							retVal = false;
						}

						if (binaryPool[5] > static_cast<std::uint8_t>(task_controller_object::DeviceElementObject::Type::NavigationReference))
						{
							LOG_ERROR("[DDOP]: Binary device element object has invalid element type.");
							retVal = false;
						}

						std::uint32_t expectedSize = (13 + (2 * numberOfObjectIDs) + numberDesignatorBytes);

						if ((binaryPoolSizeBytes >= expectedSize) && retVal)
						{
							std::string deviceElementDesignator;
							auto parentObject = static_cast<std::uint16_t>(static_cast<std::uint16_t>(binaryPool[9 + numberDesignatorBytes]) | (static_cast<std::uint16_t>(binaryPool[10 + numberDesignatorBytes]) << 8));
							auto uniqueID = static_cast<std::uint16_t>(static_cast<std::uint16_t>(binaryPool[3]) | (static_cast<std::uint16_t>(binaryPool[4]) << 8));
							auto elementNumber = static_cast<std::uint16_t>(static_cast<std::uint16_t>(binaryPool[7 + numberDesignatorBytes]) | (static_cast<std::uint16_t>(binaryPool[8 + numberDesignatorBytes]) << 8));
							auto type = static_cast<task_controller_object::DeviceElementObject::Type>(binaryPool[5]);

							for (std::uint16_t i = 0; i < numberDesignatorBytes; i++)
							{
								deviceElementDesignator.push_back(binaryPool[7 + i]);
							}

							remove_object_with_id(uniqueID); // Make sure the previous device element object is removed
							if (add_device_element(deviceElementDesignator, elementNumber, parentObject, type, uniqueID))
							{
								auto DETObject = std::static_pointer_cast<task_controller_object::DeviceElementObject>(get_object_by_id(uniqueID));

								if (nullptr != DETObject)
								{
									for (std::uint8_t i = 0; i < numberOfObjectIDs; i++)
									{
										std::uint16_t childID = static_cast<std::uint16_t>(static_cast<std::uint16_t>(binaryPool[13 + (2 * i) + numberDesignatorBytes]) | (static_cast<std::uint16_t>(binaryPool[14 + (2 * i) + numberDesignatorBytes]) << 8));
										DETObject->add_reference_to_child_object(childID);
									}
								}

								binaryPoolSizeBytes -= expectedSize;
								binaryPool += expectedSize;
							}
							else
							{
								LOG_ERROR("[DDOP]: Failed adding deserialized device element object. DDOP schema is not valid.");
								retVal = false;
							}
						}
						else
						{
							LOG_ERROR("[DDOP]: Not enough binary DDOP data left to parse device element object. DDOP schema is not valid");
							retVal = false;
						}
					}
					else if ("DPD" == xmlNameSpace)
					{
						// Process a device process data object
						std::uint8_t numberDesignatorBytes = 0;

						if ((binaryPoolSizeBytes >= 10) &&
						    (binaryPool[9] < 128))
						{
							numberDesignatorBytes = binaryPool[9];
						}
						else
						{
							LOG_ERROR("[DDOP]: Binary device process data object has invalid length.");
							retVal = false;
						}

						std::uint32_t expectedSize = (12 + numberDesignatorBytes);

						if ((binaryPoolSizeBytes >= expectedSize) && retVal)
						{
							std::string processDataDesignator;
							auto DDI = static_cast<std::uint16_t>(static_cast<std::uint16_t>(binaryPool[5]) | (static_cast<std::uint16_t>(binaryPool[6]) << 8));
							auto uniqueID = static_cast<std::uint16_t>(static_cast<std::uint16_t>(binaryPool[3]) | (static_cast<std::uint16_t>(binaryPool[4]) << 8));
							auto presentationObjectID = static_cast<std::uint16_t>(static_cast<std::uint16_t>(binaryPool[10 + numberDesignatorBytes]) | (static_cast<std::uint16_t>(binaryPool[11 + numberDesignatorBytes]) << 8));

							for (std::uint16_t i = 0; i < numberDesignatorBytes; i++)
							{
								processDataDesignator.push_back(binaryPool[10 + i]);
							}

							remove_object_with_id(uniqueID); // Make sure the previous device process data object is removed
							if (add_device_process_data(processDataDesignator, DDI, presentationObjectID, binaryPool[7], binaryPool[8], uniqueID))
							{
								binaryPoolSizeBytes -= expectedSize;
								binaryPool += expectedSize;
							}
							else
							{
								LOG_ERROR("[DDOP]: Failed adding deserialized device process data object. DDOP schema is not valid.");
								retVal = false;
							}
						}
						else
						{
							LOG_ERROR("[DDOP]: Not enough binary DDOP data left to parse device process data object. DDOP schema is not valid");
							retVal = false;
						}
					}
					else if ("DPT" == xmlNameSpace)
					{
						std::uint8_t numberDesignatorBytes = 0;

						if ((binaryPoolSizeBytes >= 12) &&
						    (binaryPool[11] < 128))
						{
							numberDesignatorBytes = binaryPool[11];
						}
						else
						{
							LOG_ERROR("[DDOP]: Binary device property object has invalid length.");
							retVal = false;
						}

						std::uint32_t expectedSize = (14 + numberDesignatorBytes);

						if ((binaryPoolSizeBytes >= expectedSize) && retVal)
						{
							std::string designator;
							std::int32_t propertyValue = static_cast<std::int32_t>(binaryPool[7]) |
							  (static_cast<std::int32_t>(binaryPool[8]) << 8) |
							  (static_cast<std::int32_t>(binaryPool[9]) << 16) |
							  (static_cast<std::int32_t>(binaryPool[10]) << 24);
							auto DDI = static_cast<std::uint16_t>(static_cast<std::uint16_t>(binaryPool[5]) | (static_cast<std::uint16_t>(binaryPool[6]) << 8));
							auto uniqueID = static_cast<std::uint16_t>(static_cast<std::uint16_t>(binaryPool[3]) | (static_cast<std::uint16_t>(binaryPool[4]) << 8));
							auto presentationObjectID = static_cast<std::uint16_t>(static_cast<std::uint16_t>(binaryPool[12 + numberDesignatorBytes]) | (static_cast<std::uint16_t>(binaryPool[13 + numberDesignatorBytes]) << 8));

							for (std::uint16_t i = 0; i < numberDesignatorBytes; i++)
							{
								designator.push_back(binaryPool[12 + i]);
							}

							remove_object_with_id(uniqueID); // Make sure the previous device property object is removed
							if (add_device_property(designator, propertyValue, DDI, presentationObjectID, uniqueID))
							{
								binaryPoolSizeBytes -= expectedSize;
								binaryPool += expectedSize;
							}
							else
							{
								LOG_ERROR("[DDOP]: Failed adding deserialized device property object. DDOP schema is not valid.");
								retVal = false;
							}
						}
						else
						{
							LOG_ERROR("[DDOP]: Not enough binary DDOP data left to parse device property object. DDOP schema is not valid");
							retVal = false;
						}
					}
					else if ("DVP" == xmlNameSpace)
					{
						std::uint8_t numberDesignatorBytes = 0;

						if ((binaryPoolSizeBytes >= 15) &&
						    (binaryPool[14] < 128))
						{
							numberDesignatorBytes = binaryPool[14];
						}
						else
						{
							LOG_ERROR("[DDOP]: Binary device value presentation object has invalid length.");
							retVal = false;
						}

						std::uint32_t expectedSize = (15 + numberDesignatorBytes);

						if ((binaryPoolSizeBytes >= expectedSize) && retVal)
						{
							std::string designator;
							std::int32_t offset = static_cast<std::int32_t>(binaryPool[5]) |
							  (static_cast<std::int32_t>(binaryPool[6]) << 8) |
							  (static_cast<std::int32_t>(binaryPool[7]) << 16) |
							  (static_cast<std::int32_t>(binaryPool[8]) << 24);
							std::array<std::uint8_t, sizeof(float)> scaleBytes = {
								binaryPool[9],
								binaryPool[10],
								binaryPool[11],
								binaryPool[12],
							};
							float scale = 0.0f;
							std::uint16_t uniqueID = static_cast<std::uint16_t>(static_cast<std::uint16_t>(binaryPool[3]) | (static_cast<std::uint16_t>(binaryPool[4]) << 8));

							if (is_big_endian())
							{
								std::reverse(scaleBytes.begin(), scaleBytes.end());
							}

							memcpy(&scale, scaleBytes.data(), sizeof(float));

							for (std::uint16_t i = 0; i < numberDesignatorBytes; i++)
							{
								designator.push_back(binaryPool[15 + i]);
							}

							remove_object_with_id(uniqueID); // Make sure the previous device value presentation object is removed
							if (add_device_value_presentation(designator, offset, scale, binaryPool[13], uniqueID))
							{
								binaryPoolSizeBytes -= expectedSize;
								binaryPool += expectedSize;
							}
							else
							{
								LOG_ERROR("[DDOP]: Failed adding deserialized device value presentation object. DDOP schema is not valid.");
								retVal = false;
							}
						}
						else
						{
							LOG_ERROR("[DDOP]: Not enough binary DDOP data left to parse device value presentation object. DDOP schema is not valid");
							retVal = false;
						}
					}
					else
					{
						LOG_ERROR("[DDOP]: Cannot process an unknown XML namespace from binary DDOP. DDOP schema is invalid.");
						retVal = false;
					}
				}
				else
				{
					retVal = false;
				}

				if (!retVal)
				{
					LOG_ERROR("[DDOP]: Binary DDOP deserialization aborted.");
					break;
				}
			}
		}
		else
		{
			retVal = false;
			LOG_ERROR("[DDOP]: Cannot deserialize a DDOP with zero length.");
		}
		return retVal;
	}

	bool DeviceDescriptorObjectPool::generate_binary_object_pool(std::vector<std::uint8_t> &resultantPool)
	{
		bool retVal = true;

		resultantPool.clear();

		if (taskControllerCompatibilityLevel > MAX_TC_VERSION_SUPPORTED)
		{
			LOG_WARNING("[DDOP]: A DDOP is being generated for a TC version that is unsupported. This may cause issues.");
		}

		if (resolve_parent_ids_to_objects())
		{
			retVal = true;
			for (const auto &currentObject : objectList)
			{
				auto objectBinary = currentObject->get_binary_object();

				if (!objectBinary.empty())
				{
					resultantPool.insert(resultantPool.end(), objectBinary.begin(), objectBinary.end());
				}
				else
				{
					LOG_ERROR("[DDOP]: Failed to create all object binaries. Your DDOP is invalid.");
					retVal = false;
					break;
				}
			}
		}
		else
		{
			LOG_ERROR("[DDOP]: Failed to resolve all object IDs in DDOP. Your DDOP contains invalid object references.");
			retVal = false;
		}
		return retVal;
	}

	bool DeviceDescriptorObjectPool::generate_task_data_iso_xml(std::string &resultantString)
	{
		bool retVal = true;

		resultantString.clear();

		if (taskControllerCompatibilityLevel > MAX_TC_VERSION_SUPPORTED)
		{
			LOG_WARNING("[DDOP]: An XML DDOP is being generated for a TC version that is unsupported. This may cause issues.");
		}

		if (resolve_parent_ids_to_objects())
		{
			std::ostringstream xmlOutput;
			std::ios initialStreamFormat(nullptr);
			std::size_t numberOfDevices = 1;
			std::size_t numberOfElements = 1;
			initialStreamFormat.copyfmt(xmlOutput);
			retVal = true;

			xmlOutput << R"(<?xml version="1.0" encoding="UTF-8"?>)" << std::endl;
			xmlOutput << R"(<ISO11783_TaskData VersionMajor="3" VersionMinor="0" DataTransferOrigin="1">)" << std::endl;

			// Find the device object, which will be the first object written
			for (std::size_t i = 0; i < size(); i++)
			{
				auto currentObject = get_object_by_index(static_cast<std::uint16_t>(i));

				if ((nullptr != currentObject) &&
				    (task_controller_object::ObjectTypes::Device == currentObject->get_object_type()))
				{
					// Found device
					auto rootDevice = std::static_pointer_cast<task_controller_object::DeviceObject>(currentObject);
					xmlOutput << "<DVC A=\"DVC-" << static_cast<int>(numberOfDevices);
					numberOfDevices++;
					xmlOutput << "\" B=\"" << rootDevice->get_designator();
					xmlOutput << "\" C=\"" << rootDevice->get_software_version();
					xmlOutput << "\" D=\"" << std::uppercase << std::hex << std::setfill('0') << std::setw(16) << static_cast<unsigned long long int>(rootDevice->get_iso_name());
					xmlOutput.copyfmt(initialStreamFormat);
					xmlOutput << "\" E=\"" << rootDevice->get_serial_number();
					xmlOutput << "\" F=\"";

					auto lStructureLabel = rootDevice->get_structure_label();
					for (std::uint8_t j = 0; j < 7; j++)
					{
						auto structureByte = static_cast<std::uint8_t>(lStructureLabel.at(6 - j));
						xmlOutput << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(structureByte);
					}

					xmlOutput << "\" G=\"";

					for (std::uint_fast8_t j = 0; j < 7; j++)
					{
						xmlOutput << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(rootDevice->get_localization_label().at(6 - j));
					}
					xmlOutput.copyfmt(initialStreamFormat);
					xmlOutput << "\">" << std::endl;

					// Next, process all elements
					for (std::size_t j = 0; j < this->size(); j++)
					{
						auto currentSubObject = get_object_by_index(static_cast<std::uint16_t>(j));

						if ((nullptr != currentSubObject) &&
						    (task_controller_object::ObjectTypes::DeviceElement == currentSubObject->get_object_type()))
						{
							auto deviceElement = std::static_pointer_cast<task_controller_object::DeviceElementObject>(currentSubObject);

							xmlOutput << "\t<DET A=\"DET-" << static_cast<int>(numberOfElements);
							numberOfElements++;
							xmlOutput << "\" B=\"" << static_cast<int>(deviceElement->get_object_id());
							xmlOutput << "\" C=\"" << static_cast<int>(deviceElement->get_type());
							xmlOutput << "\" D=\"" << deviceElement->get_designator();
							xmlOutput << "\" E=\"" << static_cast<int>(deviceElement->get_element_number());
							xmlOutput << "\" F=\"" << static_cast<int>(deviceElement->get_parent_object());

							if (deviceElement->get_number_child_objects() > 0)
							{
								xmlOutput << "\">" << std::endl;

								// Process a list of all device object references
								for (std::uint16_t k = 0; k < deviceElement->get_number_child_objects(); k++)
								{
									xmlOutput << "\t\t<DOR A=\"" << static_cast<int>(deviceElement->get_child_object_id(k)) << "\"/>" << std::endl;
								}
								xmlOutput << "\t</DET>" << std::endl;
							}
							else
							{
								xmlOutput << "\"/>" << std::endl;
							}
						}
					}

					// Next, process all DPDs
					for (std::size_t j = 0; j < this->size(); j++)
					{
						auto currentSubObject = get_object_by_index(static_cast<std::uint16_t>(j));

						if ((nullptr != currentSubObject) &&
						    (task_controller_object::ObjectTypes::DeviceProcessData == currentSubObject->get_object_type()))
						{
							auto deviceProcessData = std::static_pointer_cast<task_controller_object::DeviceProcessDataObject>(currentSubObject);

							xmlOutput << "\t<DPD A=\"" << static_cast<int>(deviceProcessData->get_object_id());
							xmlOutput << "\" B=\"" << std::uppercase << std::hex << std::setfill('0') << std::setw(4) << static_cast<int>(deviceProcessData->get_ddi());
							xmlOutput.copyfmt(initialStreamFormat);
							xmlOutput << "\" C=\"" << static_cast<int>(deviceProcessData->get_properties_bitfield());
							xmlOutput << "\" D=\"" << static_cast<int>(deviceProcessData->get_trigger_methods_bitfield());
							xmlOutput << "\" E=\"" << deviceProcessData->get_designator();
							if (0xFFFF != deviceProcessData->get_device_value_presentation_object_id())
							{
								xmlOutput << "\" F=\"" << static_cast<int>(deviceProcessData->get_device_value_presentation_object_id());
							}
							xmlOutput << "\"/>" << std::endl;
						}
					}

					// Next, process all child DPTs
					for (std::size_t j = 0; j < this->size(); j++)
					{
						auto currentSubObject = get_object_by_index(static_cast<std::uint16_t>(j));

						if ((nullptr != currentSubObject) &&
						    (task_controller_object::ObjectTypes::DeviceProperty == currentSubObject->get_object_type()))
						{
							auto deviceProperty = std::static_pointer_cast<task_controller_object::DevicePropertyObject>(currentSubObject);

							xmlOutput << "\t<DPT A=\"" << static_cast<int>(deviceProperty->get_object_id());
							xmlOutput << "\" B=\"" << std::uppercase << std::hex << std::setfill('0') << std::setw(4) << static_cast<int>(deviceProperty->get_ddi());
							xmlOutput.copyfmt(initialStreamFormat);
							xmlOutput << "\" C=\"" << static_cast<int>(deviceProperty->get_value());
							xmlOutput << "\" D=\"" << deviceProperty->get_designator();
							if (0xFFFF != deviceProperty->get_device_value_presentation_object_id())
							{
								xmlOutput << "\" E=\"" << static_cast<int>(deviceProperty->get_device_value_presentation_object_id());
							}
							xmlOutput << "\"/>" << std::endl;
						}
					}

					// Next, process all child DVPs
					for (std::size_t j = 0; j < this->size(); j++)
					{
						auto currentSubObject = get_object_by_index(static_cast<std::uint16_t>(j));

						if ((nullptr != currentSubObject) &&
						    (task_controller_object::ObjectTypes::DeviceValuePresentation == currentSubObject->get_object_type()))
						{
							auto deviceValuePresentation = std::static_pointer_cast<task_controller_object::DeviceValuePresentationObject>(currentSubObject);

							xmlOutput << "\t<DVP A=\"" << static_cast<int>(deviceValuePresentation->get_object_id());
							xmlOutput << "\" B=\"" << static_cast<int>(deviceValuePresentation->get_offset());
							xmlOutput << "\" C=\"" << std::fixed << std::setprecision(6) << deviceValuePresentation->get_scale();
							xmlOutput.copyfmt(initialStreamFormat);
							xmlOutput << "\" D=\"" << static_cast<int>(deviceValuePresentation->get_number_of_decimals());
							xmlOutput << "\" E=\"" << deviceValuePresentation->get_designator();
							xmlOutput << "\"/>" << std::endl;
						}
					}

					// Close DVC object
					xmlOutput << "</DVC>" << std::endl;
					xmlOutput << "</ISO11783_TaskData>" << std::endl;
					resultantString = xmlOutput.str();
					LOG_DEBUG("[DDOP]: Generated ISO XML DDOP data OK");
					break;
				}
			}
		}
		else
		{
			LOG_ERROR("[DDOP]: Failed to resolve all object IDs in DDOP. Your DDOP contains invalid object references.");
			retVal = false;
		}
		return retVal;
	}

	std::shared_ptr<task_controller_object::Object> DeviceDescriptorObjectPool::get_object_by_id(std::uint16_t objectID)
	{
		std::shared_ptr<task_controller_object::Object> retVal;

		for (const auto &currentObject : objectList)
		{
			if (currentObject->get_object_id() == objectID)
			{
				retVal = currentObject;
				break;
			}
		}
		return retVal;
	}

	std::shared_ptr<task_controller_object::Object> DeviceDescriptorObjectPool::get_object_by_index(std::uint16_t index)
	{
		std::shared_ptr<task_controller_object::Object> retVal = nullptr;

		if (index < objectList.size())
		{
			retVal = objectList.at(index);
		}
		return retVal;
	}

	bool DeviceDescriptorObjectPool::remove_object_by_id(std::uint16_t objectID)
	{
		bool retVal = false;

		for (auto object = objectList.begin(); object != objectList.end(); object++)
		{
			if ((nullptr != *object) && (*object)->get_object_id() == objectID)
			{
				objectList.erase(object);
				retVal = true;
				break;
			}
		}
		return retVal;
	}

	void DeviceDescriptorObjectPool::set_task_controller_compatibility_level(std::uint8_t tcVersion)
	{
		assert(tcVersion <= MAX_TC_VERSION_SUPPORTED); // You can't set the version higher than the max

		taskControllerCompatibilityLevel = tcVersion;

		// Manipulate the device object if it exists
		auto deviceObject = std::static_pointer_cast<task_controller_object::DeviceObject>(get_object_by_id(0));
		if (nullptr != deviceObject)
		{
			deviceObject->set_use_extended_structure_label(taskControllerCompatibilityLevel >= 4);
		}
	}

	std::uint8_t DeviceDescriptorObjectPool::get_task_controller_compatibility_level() const
	{
		return taskControllerCompatibilityLevel;
	}

	std::uint8_t DeviceDescriptorObjectPool::get_max_supported_task_controller_version()
	{
		return MAX_TC_VERSION_SUPPORTED;
	}

	void DeviceDescriptorObjectPool::clear()
	{
		objectList.clear();
	}

	std::uint16_t DeviceDescriptorObjectPool::size() const
	{
		return static_cast<std::uint16_t>(objectList.size());
	}

	bool DeviceDescriptorObjectPool::resolve_parent_ids_to_objects()
	{
		bool retVal = true;

		for (const auto &currentObject : objectList)
		{
			assert(nullptr != currentObject);
			switch (currentObject->get_object_type())
			{
				case task_controller_object::ObjectTypes::DeviceElement:
				{
					// Process parent object
					auto currentDeviceElement = reinterpret_cast<task_controller_object::DeviceElementObject *>(currentObject.get());
					if (NULL_OBJECT_ID != currentDeviceElement->get_parent_object())
					{
						auto parent = get_object_by_id(currentDeviceElement->get_parent_object());
						if (nullptr != parent.get())
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
									LOG_ERROR("[DDOP]: Object " +
									          isobus::to_string(static_cast<int>(currentObject->get_object_id())) +
									          " has an invalid parent object type. Only device element objects or device objects may be its parent.");
									retVal = false;
								}
								break;
							}
						}
						else
						{
							LOG_ERROR("[DDOP]: Object " +
							          isobus::to_string(static_cast<int>(currentDeviceElement->get_parent_object())) +
							          " is not found.");
							retVal = false;
						}
					}
					else
					{
						LOG_ERROR("[DDOP]: Object " +
						          isobus::to_string(static_cast<int>(currentObject->get_object_id())) +
						          " is an orphan. It's parent is 0xFFFF!");
						retVal = false;
					}

					if (retVal)
					{
						// Process children now that parent has been validated
						for (std::uint16_t i = 0; i < currentDeviceElement->get_number_child_objects(); i++)
						{
							auto child = get_object_by_id(currentDeviceElement->get_child_object_id(i));
							if (nullptr == child.get())
							{
								LOG_ERROR("[DDOP]: Object " +
								          isobus::to_string(static_cast<int>(currentDeviceElement->get_child_object_id(i))) +
								          " is not found.");
								retVal = false;
								break;
							}
							else if ((task_controller_object::ObjectTypes::DeviceProcessData != child->get_object_type()) &&
							         (task_controller_object::ObjectTypes::DeviceProperty != child->get_object_type()))
							{
								LOG_ERROR("[DDOP]: Object %u has child %u which is an object type that is not allowed.",
								          currentDeviceElement->get_child_object_id(i),
								          child->get_object_id());
								LOG_ERROR("[DDOP]: A DET object may only have DPD and DPT children.");
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

					if (NULL_OBJECT_ID != currentProcessData->get_device_value_presentation_object_id())
					{
						auto child = get_object_by_id(currentProcessData->get_device_value_presentation_object_id());
						if (nullptr == child.get())
						{
							LOG_ERROR("[DDOP]: Object " +
							          isobus::to_string(static_cast<int>(currentProcessData->get_device_value_presentation_object_id())) +
							          " is not found.");
							retVal = false;
							break;
						}
						else if (task_controller_object::ObjectTypes::DeviceValuePresentation != child->get_object_type())
						{
							LOG_ERROR("[DDOP]: Object %u has a child %u with an object type that is not allowed.",
							          currentProcessData->get_device_value_presentation_object_id(),
							          child->get_object_id());
							LOG_ERROR("[DDOP]: A DPD object may only have DVP children.");
							retVal = false;
							break;
						}
					}
				}
				break;

				case task_controller_object::ObjectTypes::DeviceProperty:
				{
					auto currentProperty = reinterpret_cast<task_controller_object::DevicePropertyObject *>(currentObject.get());

					if (NULL_OBJECT_ID != currentProperty->get_device_value_presentation_object_id())
					{
						auto child = get_object_by_id(currentProperty->get_device_value_presentation_object_id());
						if (nullptr == child.get())
						{
							LOG_ERROR("[DDOP]: Object " +
							          isobus::to_string(static_cast<int>(currentProperty->get_device_value_presentation_object_id())) +
							          " is not found.");
							retVal = false;
							break;
						}
						else if (task_controller_object::ObjectTypes::DeviceValuePresentation != child->get_object_type())
						{
							LOG_ERROR("[DDOP]: Object %u has a child %u with an object type that is not allowed.",
							          currentProperty->get_device_value_presentation_object_id(),
							          child->get_object_id());
							LOG_ERROR("[DDOP]: A DPT object may only have DVP children.");
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

		if ((0 != uniqueID) && (NULL_OBJECT_ID != uniqueID))
		{
			for (const auto &currentObject : objectList)
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
