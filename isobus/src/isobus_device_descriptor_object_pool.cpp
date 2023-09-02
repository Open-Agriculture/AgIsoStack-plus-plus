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
#include "isobus/utility/platform_endianness.hpp"
#include "isobus/utility/to_string.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>

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
			if ((taskControllerCompatibilityLevel < MAX_TC_VERSION_SUPPORTED) &&
			    (deviceDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device designator " +
				                     deviceDesignator +
				                     " is greater than the max byte length of 32. Value will be truncated.");
				deviceDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH);
			}
			else if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (deviceDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device designator " +
				                     deviceDesignator +
				                     " is greater than the max byte length of 128. Value will be truncated.");
				deviceDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LENGTH);
			}
			else if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (deviceDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				CANStackLogger::info("[DDOP]: Device designator " +
				                     deviceDesignator +
				                     " byte length is greater than the max character count of 32. " +
				                     "This is only acceptable if you have 32 or fewer UTF-8 characters!" +
				                     " Please verify your DDOP configuration meets this requirement.");
			}

			if ((taskControllerCompatibilityLevel < MAX_TC_VERSION_SUPPORTED) &&
			    (deviceSerialNumber.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device serial number " +
				                     deviceSerialNumber +
				                     " is greater than the max byte length of 32. Value will be truncated.");
				deviceSerialNumber.resize(task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH);
			}
			else if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (deviceSerialNumber.size() > task_controller_object::Object::MAX_DESIGNATOR_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device serial number " +
				                     deviceSerialNumber +
				                     " is greater than the max byte length of 128. Value will be truncated.");
				deviceSerialNumber.resize(task_controller_object::Object::MAX_DESIGNATOR_LENGTH);
			}
			else if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
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
			                                                                 clientIsoNAME,
			                                                                 (taskControllerCompatibilityLevel >= 4)));
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
				CANStackLogger::warn("[DDOP]: Device element designator " +
				                     deviceElementDesignator +
				                     " is greater than the max length of 32. Value will be truncated.");
				deviceElementDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH);
			}
			if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
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
			                                                                        deviceElementType,
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

			if ((taskControllerCompatibilityLevel < MAX_TC_VERSION_SUPPORTED) &&
			    (processDataDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device process data designator " +
				                     processDataDesignator +
				                     " is greater than the max byte length of 32. Value will be truncated.");
				processDataDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH);
			}
			else if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (processDataDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device process data designator " +
				                     processDataDesignator +
				                     " is greater than the max byte length of 128. Value will be truncated.");
				processDataDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LENGTH);
			}
			else if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
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
			if ((taskControllerCompatibilityLevel < MAX_TC_VERSION_SUPPORTED) &&
			    (propertyDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device property designator " +
				                     propertyDesignator +
				                     " is greater than the max byte length of 32. Value will be truncated.");
				propertyDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH);
			}
			else if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (propertyDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device property designator " +
				                     propertyDesignator +
				                     " is greater than the max byte length of 128. Value will be truncated.");
				propertyDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LENGTH);
			}
			else if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
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
			if ((taskControllerCompatibilityLevel < MAX_TC_VERSION_SUPPORTED) &&
			    (unitDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device value presentation unit designator " +
				                     unitDesignator +
				                     " is greater than the max byte length of 32. Value will be truncated.");
				unitDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LEGACY_LENGTH);
			}
			else if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
			         (unitDesignator.size() > task_controller_object::Object::MAX_DESIGNATOR_LENGTH))
			{
				CANStackLogger::warn("[DDOP]: Device value presentation unit designator " +
				                     unitDesignator +
				                     " is greater than the max byte length of 128. Value will be truncated.");
				unitDesignator.resize(task_controller_object::Object::MAX_DESIGNATOR_LENGTH);
			}
			else if ((taskControllerCompatibilityLevel == MAX_TC_VERSION_SUPPORTED) &&
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

	bool DeviceDescriptorObjectPool::deserialize_binary_object_pool(std::vector<std::uint8_t> &binaryPool, NAME clientNAME)
	{
		return deserialize_binary_object_pool(binaryPool.data(), static_cast<std::uint32_t>(binaryPool.size()), clientNAME);
	}

	bool DeviceDescriptorObjectPool::deserialize_binary_object_pool(const std::uint8_t *binaryPool, std::uint32_t binaryPoolSizeBytes, NAME clientNAME)
	{
		bool retVal = true;

		if ((nullptr != binaryPool) && (0 != binaryPoolSizeBytes))
		{
			CANStackLogger::debug("[DDOP]: Attempting to deserialize a binary object pool with size %u.", binaryPoolSizeBytes);
			clear();

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
							CANStackLogger::error("[DDOP]: Binary device object designator has invalid length.");
							retVal = false;
						}

						if ((binaryPoolSizeBytes >= static_cast<std::uint32_t>(7 + numberDesignatorBytes)) &&
						    (binaryPool[6 + numberDesignatorBytes] < 128))
						{
							numberSoftwareVersionBytes = binaryPool[6 + numberDesignatorBytes];
						}
						else
						{
							CANStackLogger::error("[DDOP]: Binary device object software version has invalid length.");
							retVal = false;
						}

						if ((binaryPoolSizeBytes >= static_cast<std::uint32_t>(16 + numberDesignatorBytes + numberSoftwareVersionBytes)) &&
						    (binaryPool[15 + numberDesignatorBytes + numberSoftwareVersionBytes] < 128))
						{
							numberDeviceSerialNumberBytes = binaryPool[15 + numberDesignatorBytes + numberSoftwareVersionBytes];
						}
						else
						{
							CANStackLogger::error("[DDOP]: Binary device object serial number has invalid length.");
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
								CANStackLogger::error("[DDOP]: Binary device object with version 4 contains invalid extended structure label length.");
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
								CANStackLogger::error("[DDOP]: Failed adding deserialized device object. DDOP NAME doesn't match client's actual NAME.");
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

							if (add_device(deviceDesignator, deviceSoftwareVersion, deviceSerialNumber, deviceStructureLabel, localizationLabel, extendedStructureLabel, clientNAME.get_full_name()))
							{
								binaryPoolSizeBytes -= expectedSize;
								binaryPool += expectedSize;
							}
							else
							{
								CANStackLogger::error("[DDOP]: Failed adding deserialized device object. DDOP schema is not valid.");
								retVal = false;
							}
						}
						else
						{
							CANStackLogger::error("[DDOP]: Not enough binary DDOP data left to parse device object. DDOP schema is not valid");
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
							CANStackLogger::error("[DDOP]: Binary device element object has invalid length.");
							retVal = false;
						}

						if (binaryPoolSizeBytes >= static_cast<std::uint32_t>(12 + numberDesignatorBytes))
						{
							numberOfObjectIDs = binaryPool[11 + numberDesignatorBytes];
						}
						else
						{
							CANStackLogger::error("[DDOP]: Binary device element object has invalid length to process referenced object IDs.");
							retVal = false;
						}

						if (binaryPool[5] > static_cast<std::uint8_t>(task_controller_object::DeviceElementObject::Type::NavigationReference))
						{
							CANStackLogger::error("[DDOP]: Binary device element object has invalid element type.");
							retVal = false;
						}

						std::uint32_t expectedSize = (13 + (2 * numberOfObjectIDs) + numberDesignatorBytes);

						if ((binaryPoolSizeBytes >= expectedSize) && retVal)
						{
							std::string deviceElementDesignator;
							std::uint16_t parentObject = static_cast<std::uint16_t>(static_cast<std::uint16_t>(binaryPool[9 + numberDesignatorBytes]) | (static_cast<std::uint16_t>(binaryPool[10 + numberDesignatorBytes]) << 8));
							std::uint16_t uniqueID = static_cast<std::uint16_t>(static_cast<std::uint16_t>(binaryPool[3]) | (static_cast<std::uint16_t>(binaryPool[4]) << 8));
							auto type = static_cast<task_controller_object::DeviceElementObject::Type>(binaryPool[5]);

							for (std::uint16_t i = 0; i < numberDesignatorBytes; i++)
							{
								deviceElementDesignator.push_back(binaryPool[7 + i]);
							}

							if (add_device_element(deviceElementDesignator, binaryPool[7 + numberDesignatorBytes], parentObject, type, uniqueID))
							{
								auto DETObject = std::dynamic_pointer_cast<task_controller_object::DeviceElementObject>(get_object_by_id(uniqueID));

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
								CANStackLogger::error("[DDOP]: Failed adding deserialized device element object. DDOP schema is not valid.");
								retVal = false;
							}
						}
						else
						{
							CANStackLogger::error("[DDOP]: Not enough binary DDOP data left to parse device element object. DDOP schema is not valid");
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
							CANStackLogger::error("[DDOP]: Binary device process data object has invalid length.");
							retVal = false;
						}

						std::uint32_t expectedSize = (12 + numberDesignatorBytes);

						if ((binaryPoolSizeBytes >= expectedSize) && retVal)
						{
							std::string processDataDesignator;
							std::uint16_t DDI = static_cast<std::uint16_t>(static_cast<std::uint16_t>(binaryPool[5]) | (static_cast<std::uint16_t>(binaryPool[6]) << 8));
							std::uint16_t uniqueID = static_cast<std::uint16_t>(static_cast<std::uint16_t>(binaryPool[3]) | (static_cast<std::uint16_t>(binaryPool[4]) << 8));
							std::uint16_t presentationObjectID = static_cast<std::uint16_t>(static_cast<std::uint16_t>(binaryPool[10 + numberDesignatorBytes]) | (static_cast<std::uint16_t>(binaryPool[11 + numberDesignatorBytes]) << 8));

							for (std::uint16_t i = 0; i < numberDesignatorBytes; i++)
							{
								processDataDesignator.push_back(binaryPool[10 + i]);
							}

							if (add_device_process_data(processDataDesignator, DDI, presentationObjectID, binaryPool[7], binaryPool[8], uniqueID))
							{
								binaryPoolSizeBytes -= expectedSize;
								binaryPool += expectedSize;
							}
							else
							{
								CANStackLogger::error("[DDOP]: Failed adding deserialized device process data object. DDOP schema is not valid.");
								retVal = false;
							}
						}
						else
						{
							CANStackLogger::error("[DDOP]: Not enough binary DDOP data left to parse device process data object. DDOP schema is not valid");
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
							CANStackLogger::error("[DDOP]: Binary device property object has invalid length.");
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
							std::uint16_t DDI = static_cast<std::uint16_t>(static_cast<std::uint16_t>(binaryPool[5]) | (static_cast<std::uint16_t>(binaryPool[6]) << 8));
							std::uint16_t uniqueID = static_cast<std::uint16_t>(static_cast<std::uint16_t>(binaryPool[3]) | (static_cast<std::uint16_t>(binaryPool[4]) << 8));
							std::uint16_t presentationObjectID = static_cast<std::uint16_t>(static_cast<std::uint16_t>(binaryPool[12 + numberDesignatorBytes]) | (static_cast<std::uint16_t>(binaryPool[13 + numberDesignatorBytes]) << 8));

							for (std::uint16_t i = 0; i < numberDesignatorBytes; i++)
							{
								designator.push_back(binaryPool[12 + i]);
							}

							if (add_device_property(designator, propertyValue, DDI, presentationObjectID, uniqueID))
							{
								binaryPoolSizeBytes -= expectedSize;
								binaryPool += expectedSize;
							}
							else
							{
								CANStackLogger::error("[DDOP]: Failed adding deserialized device property object. DDOP schema is not valid.");
								retVal = false;
							}
						}
						else
						{
							CANStackLogger::error("[DDOP]: Not enough binary DDOP data left to parse device property object. DDOP schema is not valid");
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
							CANStackLogger::error("[DDOP]: Binary device value presentation object has invalid length.");
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

							if (add_device_value_presentation(designator, offset, scale, binaryPool[13], uniqueID))
							{
								binaryPoolSizeBytes -= expectedSize;
								binaryPool += expectedSize;
							}
							else
							{
								CANStackLogger::error("[DDOP]: Failed adding deserialized device value presentation object. DDOP schema is not valid.");
								retVal = false;
							}
						}
						else
						{
							CANStackLogger::error("[DDOP]: Not enough binary DDOP data left to parse device value presentation object. DDOP schema is not valid");
							retVal = false;
						}
					}
					else
					{
						CANStackLogger::error("[DDOP]: Cannot process an unknown XML namespace from binary DDOP. DDOP schema is invalid.");
						retVal = false;
					}
				}
				else
				{
					retVal = false;
				}

				if (!retVal)
				{
					CANStackLogger::error("[DDOP]: Binary DDOP deserialization aborted.");
					break;
				}
			}
		}
		else
		{
			retVal = false;
			CANStackLogger::error("[DDOP]: Cannot deserialize a DDOP with zero length.");
		}
		return retVal;
	}

	bool DeviceDescriptorObjectPool::generate_binary_object_pool(std::vector<std::uint8_t> &resultantPool)
	{
		bool retVal = true;

		resultantPool.clear();

		if (taskControllerCompatibilityLevel > MAX_TC_VERSION_SUPPORTED)
		{
			CANStackLogger::warn("[DDOP]: A DDOP is being generated for a TC version that is unsupported. This may cause issues.");
		}

		if (resolve_parent_ids_to_objects())
		{
			retVal = true;
			for (auto &currentObject : objectList)
			{
				auto objectBinary = currentObject->get_binary_object();

				if (!objectBinary.empty())
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
			retVal = false;
		}
		return retVal;
	}

	std::shared_ptr<task_controller_object::Object> DeviceDescriptorObjectPool::get_object_by_id(std::uint16_t objectID)
	{
		std::shared_ptr<task_controller_object::Object> retVal;

		for (auto &currentObject : objectList)
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

	std::size_t DeviceDescriptorObjectPool::size() const
	{
		return objectList.size();
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
						CANStackLogger::error("[DDOP]: Object " +
						                      isobus::to_string(static_cast<int>(currentObject->get_object_id())) +
						                      " is an orphan. It's parent is 0xFFFF!");
						retVal = false;
					}

					if (retVal)
					{
						// Process children now that parent has been validated
						for (std::size_t i = 0; i < currentDeviceElement->get_number_child_objects(); i++)
						{
							auto child = get_object_by_id(currentDeviceElement->get_child_object_id(i));
							if (nullptr == child.get())
							{
								CANStackLogger::error("[DDOP]: Object " +
								                      isobus::to_string(static_cast<int>(currentDeviceElement->get_child_object_id(i))) +
								                      " is not found.");
								retVal = false;
								break;
							}
							else if ((task_controller_object::ObjectTypes::DeviceProcessData != child->get_object_type()) &&
							         (task_controller_object::ObjectTypes::DeviceProperty != child->get_object_type()))
							{
								CANStackLogger::error("[DDOP]: Object %u has child %u which is an object type that is not allowed.",
								                      currentDeviceElement->get_child_object_id(i),
								                      child->get_object_id());
								CANStackLogger::error("[DDOP]: A DET object may only have DPD and DPT children.");
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
						auto child = get_object_by_id(currentProcessData->get_device_value_presentation_object_id());
						if (nullptr == child.get())
						{
							CANStackLogger::error("[DDOP]: Object " +
							                      isobus::to_string(static_cast<int>(currentProcessData->get_device_value_presentation_object_id())) +
							                      " is not found.");
							retVal = false;
							break;
						}
						else if (task_controller_object::ObjectTypes::DeviceValuePresentation != child->get_object_type())
						{
							CANStackLogger::error("[DDOP]: Object %u has a child %u with an object type that is not allowed." +
							                        currentProcessData->get_device_value_presentation_object_id(),
							                      child->get_object_id());
							CANStackLogger::error("[DDOP]: A DPD object may only have DVP children.");
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
						auto child = get_object_by_id(currentProperty->get_device_value_presentation_object_id());
						if (nullptr == child.get())
						{
							CANStackLogger::error("[DDOP]: Object " +
							                      isobus::to_string(static_cast<int>(currentProperty->get_device_value_presentation_object_id())) +
							                      " is not found.");
							retVal = false;
							break;
						}
						else if (task_controller_object::ObjectTypes::DeviceValuePresentation != child->get_object_type())
						{
							CANStackLogger::error("[DDOP]: Object %u has a child %u with an object type that is not allowed." +
							                        currentProperty->get_device_value_presentation_object_id(),
							                      child->get_object_id());
							CANStackLogger::error("[DDOP]: A DPT object may only have DVP children.");
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
