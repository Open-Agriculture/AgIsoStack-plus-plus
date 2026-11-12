//================================================================================================
/// @file isobus_device_descriptor_object_pool_xml.cpp
///
/// @brief Implements the ISO XML export of a Task Controller DDOP.
/// @details This is a separate file from the rest of the DDOP, so that a program that only uploads
/// the binary DDOP doesn't link it. The export uses a C++ stream, and linking any stream brings in
/// the standard library's stream and locale code, which costs more than 100 KB on a microcontroller.
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/isobus_device_descriptor_object_pool.hpp"

#include "isobus/isobus/can_stack_logger.hpp"

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

namespace isobus
{
	/// @brief Returns the length of the well-formed UTF-8 sequence starting at an index of a string.
	/// @details The byte ranges are those of the Unicode Standard, table 3-7, which also rejects
	/// overlong forms and surrogates.
	/// @param[in] value The string to check
	/// @param[in] index The index of the first byte of the sequence
	/// @returns The number of bytes in the sequence (1-4), or 0 if the bytes are not well-formed UTF-8
	static std::size_t get_utf8_sequence_length(const std::string &value, std::size_t index)
	{
		const auto leadByte = static_cast<std::uint8_t>(value[index]);
		std::size_t retVal = 0;
		std::uint8_t minimumSecondByte = 0x80;
		std::uint8_t maximumSecondByte = 0xBF;

		if (leadByte < 0x80)
		{
			retVal = 1;
		}
		else if ((leadByte >= 0xC2) && (leadByte <= 0xDF))
		{
			retVal = 2;
		}
		else if ((leadByte >= 0xE0) && (leadByte <= 0xEF))
		{
			retVal = 3;
			if (0xE0 == leadByte)
			{
				minimumSecondByte = 0xA0;
			}
			else if (0xED == leadByte)
			{
				maximumSecondByte = 0x9F;
			}
		}
		else if ((leadByte >= 0xF0) && (leadByte <= 0xF4))
		{
			retVal = 4;
			if (0xF0 == leadByte)
			{
				minimumSecondByte = 0x90;
			}
			else if (0xF4 == leadByte)
			{
				maximumSecondByte = 0x8F;
			}
		}

		if (retVal > (value.size() - index))
		{
			retVal = 0;
		}

		for (std::size_t i = 1; i < retVal; i++)
		{
			const auto continuationByte = static_cast<std::uint8_t>(value[index + i]);
			const std::uint8_t minimum = (1 == i) ? minimumSecondByte : static_cast<std::uint8_t>(0x80);
			const std::uint8_t maximum = (1 == i) ? maximumSecondByte : static_cast<std::uint8_t>(0xBF);

			if ((continuationByte < minimum) || (continuationByte > maximum))
			{
				retVal = 0;
			}
		}
		return retVal;
	}

	/// @brief Converts a DDOP string into text that is safe to place inside an XML 1.0 attribute value.
	/// @details Binary DDOPs pad strings with NULs and can cut a designator in the middle of a UTF-8
	/// sequence, and XML 1.0 forbids NUL even as a character reference. So the string ends at its first NUL,
	/// markup characters are escaped, and bytes that are not well-formed UTF-8, control characters other than
	/// tab, line feed and carriage return, U+FFFE and U+FFFF are dropped.
	/// @param[in] value The string to convert
	/// @returns The escaped string
	static std::string to_xml_attribute_value(std::string value)
	{
		value = value.substr(0, value.find('\0'));
		std::string retVal;
		std::size_t index = 0;

		while (index < value.size())
		{
			const std::size_t sequenceLength = get_utf8_sequence_length(value, index);

			if (1 == sequenceLength)
			{
				switch (value[index])
				{
					case '&':
						retVal += "&amp;";
						break;
					case '<':
						retVal += "&lt;";
						break;
					case '>':
						retVal += "&gt;";
						break;
					case '"':
						retVal += "&quot;";
						break;
					case '\'':
						retVal += "&apos;";
						break;
					// Escaped so that attribute value normalization doesn't turn them into spaces
					case '\t':
						retVal += "&#9;";
						break;
					case '\n':
						retVal += "&#10;";
						break;
					case '\r':
						retVal += "&#13;";
						break;
					default:
						if (value[index] >= ' ')
						{
							retVal += value[index];
						}
						break;
				}
			}
			else if (3 == sequenceLength)
			{
				const bool isNonCharacter = ((0xEF == static_cast<std::uint8_t>(value[index])) &&
				                             (0xBF == static_cast<std::uint8_t>(value[index + 1])) &&
				                             (0xBE <= static_cast<std::uint8_t>(value[index + 2])));

				if (!isNonCharacter)
				{
					retVal.append(value, index, sequenceLength);
				}
			}
			else if (0 != sequenceLength)
			{
				retVal.append(value, index, sequenceLength);
			}
			index += ((0 == sequenceLength) ? 1 : sequenceLength);
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
					xmlOutput << "\" B=\"" << to_xml_attribute_value(rootDevice->get_designator());
					xmlOutput << "\" C=\"" << to_xml_attribute_value(rootDevice->get_software_version());
					xmlOutput << "\" D=\"" << std::uppercase << std::hex << std::setfill('0') << std::setw(16) << static_cast<unsigned long long int>(rootDevice->get_iso_name());
					xmlOutput.copyfmt(initialStreamFormat);
					xmlOutput << "\" E=\"" << to_xml_attribute_value(rootDevice->get_serial_number());
					xmlOutput << "\" F=\"";

					auto lStructureLabel = rootDevice->get_structure_label();
					lStructureLabel.resize(task_controller_object::DeviceObject::MAX_STRUCTURE_AND_LOCALIZATION_LABEL_LENGTH, ' ');
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
							xmlOutput << "\" D=\"" << to_xml_attribute_value(deviceElement->get_designator());
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
							xmlOutput << "\" E=\"" << to_xml_attribute_value(deviceProcessData->get_designator());
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
							xmlOutput << "\" D=\"" << to_xml_attribute_value(deviceProperty->get_designator());
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
							xmlOutput << "\" E=\"" << to_xml_attribute_value(deviceValuePresentation->get_designator());
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
} // namespace isobus
