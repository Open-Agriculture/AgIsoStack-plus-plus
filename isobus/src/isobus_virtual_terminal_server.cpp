//================================================================================================
/// @file isobus_virtual_terminal_server.cpp
///
/// @brief Implements portions of an abstract VT server.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/isobus_virtual_terminal_server.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_message.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"

namespace isobus
{
	VirtualTerminalServer::VirtualTerminalServer(std::shared_ptr<InternalControlFunction> controlFunctionToUse) :
	  languageCommandInterface(controlFunctionToUse, true),
	  serverInternalControlFunction(controlFunctionToUse)
	{
	}

	VirtualTerminalServer ::~VirtualTerminalServer()
	{
		if (initialized)
		{
			CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
			                                                                                          process_rx_message,
			                                                                                          this);
		}
	}

	void VirtualTerminalServer::initialize()
	{
		if (!initialized)
		{
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
			                                                                                       process_rx_message,
			                                                                                       this);
		}
	}

	bool VirtualTerminalServer::get_initialized() const
	{
		return initialized;
	}

	std::shared_ptr<VirtualTerminalServerManagedWorkingSet> VirtualTerminalServer::get_active_working_set() const
	{
		return activeWorkingSet;
	}

	VirtualTerminalBase::GraphicMode VirtualTerminalServer::get_graphic_mode() const
	{
		return VirtualTerminalBase::GraphicMode::TwoHundredFiftySixColour;
	}

	std::uint8_t VirtualTerminalServer::get_powerup_time() const
	{
		return 0xFF;
	}

	std::uint8_t VirtualTerminalServer::get_supported_small_fonts_bitfield() const
	{
		return 0xFF;
	}

	std::uint8_t VirtualTerminalServer::get_supported_large_fonts_bitfield() const
	{
		return 0xFF;
	}

	EventDispatcher<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, std::uint16_t, std::uint16_t> &VirtualTerminalServer::get_on_change_active_mask_event_dispatcher()
	{
		return onChangeActiveMaskEventDispatcher;
	}

	EventDispatcher<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, std::uint16_t, bool> &VirtualTerminalServer::get_on_hide_show_object_event_dispatcher()
	{
		return onHideShowObjectEventDispatcher;
	}

	EventDispatcher<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, std::uint16_t, bool> &VirtualTerminalServer::get_on_enable_disable_object_event_dispatcher()
	{
		return onEnableDisableObjectEventDispatcher;
	}

	EventDispatcher<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, std::uint16_t, std::uint32_t> &VirtualTerminalServer::get_on_change_numeric_value_event_dispatcher()
	{
		return onChangeNumericValueEventDispatcher;
	}

	EventDispatcher<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, std::uint16_t, std::uint16_t, std::int8_t, std::int8_t> &VirtualTerminalServer::get_on_change_child_location_event_dispatcher()
	{
		return onChangeChildLocationEventDispatcher;
	}

	LanguageCommandInterface &VirtualTerminalServer::get_language_command_interface()
	{
		return languageCommandInterface;
	}

	bool VirtualTerminalServer::check_if_source_is_managed(const CANMessage &message)
	{
		// Check if we're managing this CF
		bool retVal = false;

		// This is the static callback for the instance.
		// See if we need to set up a new managed working set.
		for (auto cf : managedWorkingSetList)
		{
			if (cf->get_control_function() == message.get_source_control_function())
			{
				// Found a match
				retVal = true;
				break;
			}
		}

		if (!retVal)
		{
			if ((message.get_data()[0] == static_cast<std::uint8_t>(Function::WorkingSetMaintenanceMessage)) &&
			    (message.get_data()[1] & 0x01)) // Init bit is set
			{
				// This CF is probably trying to initiate communication with us.
				managedWorkingSetList.emplace_back(std::move(std::make_shared<VirtualTerminalServerManagedWorkingSet>(message.get_source_control_function())));
				auto &data = message.get_data();

				CANStackLogger::info("[VT Server]: Client %u initiated working set maintenance messages with version %u", managedWorkingSetList.back()->get_control_function()->get_address(), data[2]);
				if (data[2] > static_cast<std::uint8_t>(get_version()))
				{
					CANStackLogger::warn("[VT Server]: Client %u version %u is not supported", managedWorkingSetList.back()->get_control_function()->get_address(), data[2]);
				}
				managedWorkingSetList.back()->set_working_set_maintenance_message_timestamp_ms(SystemTiming::get_timestamp_ms());
			}
			else
			{
				// Whomever this is has probably timed out. Send them a NACK
				CANStackLogger::warn("[VT Server]: Received a non-status message from a client at address %u, but they are not connected to this VT.", message.get_identifier().get_source_address());
				send_acknowledgement(AcknowledgementType::Negative, static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal), serverInternalControlFunction, message.get_source_control_function());
			}
		}
		return retVal;
	}

	void VirtualTerminalServer::process_rx_message(const CANMessage &message, void *parent)
	{
		auto parentServer = static_cast<VirtualTerminalServer *>(parent);
		if ((nullptr != message.get_source_control_function()) &&
		    (nullptr != parentServer) &&
		    (CAN_DATA_LENGTH <= message.get_data_length()) &&
		    (parentServer->check_if_source_is_managed(message)))
		{
			for (auto cf : parentServer->managedWorkingSetList)
			{
				if (cf->get_control_function() == message.get_source_control_function())
				{
					auto &data = message.get_data();

					switch (message.get_identifier().get_parameter_group_number())
					{
						case static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal):
						{
							switch (data[0])
							{
								case static_cast<std::uint32_t>(Function::ObjectPoolTransferMessage):
								{
									auto tempPool = data; // Make a copy of the data (ouch)
									tempPool.erase(tempPool.begin()); // Strip off the mux byte (double ouch, good thing this is rare)
									CANStackLogger::info("[VT Server]: An ecu at address %u transferred &u bytes of object pool data to us.", message.get_identifier().get_source_address(), static_cast<std::uint32_t>(tempPool.size()));
									cf->add_iop_raw_data(tempPool);
								}
								break;

								case static_cast<std::uint32_t>(Function::GetMemoryMessage):
								{
									std::uint32_t requiredMemory = (data[2] | (static_cast<std::uint32_t>(data[3]) << 8) | (static_cast<std::uint32_t>(data[4]) << 16) | (static_cast<std::uint32_t>(data[5]) << 24));
									bool isEnoughMemory = parentServer->get_is_enough_memory(requiredMemory);
									CANStackLogger::info("[VT Server]: An ecu requested %u bytes of memory.", requiredMemory);

									if (!isEnoughMemory)
									{
										CANStackLogger::warn("[VT Server]: Callback indicated there is NOT enough memory.", requiredMemory);
									}
									else
									{
										CANStackLogger::debug("[VT Server]: Callback indicated there may be enough memory, but since there is overhead associated to object storage it is impossible to be sure.", requiredMemory);
									}

									std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };
									buffer[0] = static_cast<std::uint8_t>(Function::GetMemoryMessage);
									buffer[1] = static_cast<std::uint8_t>(parentServer->get_version());
									buffer[2] = static_cast<std::uint8_t>(!isEnoughMemory);
									buffer[3] = 0xFF; // Reserved
									buffer[4] = 0xFF; // Reserved
									buffer[5] = 0xFF; // Reserved
									buffer[6] = 0xFF; // Reserved
									buffer[7] = 0xFF; // Reserved
									CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
									                                               buffer.data(),
									                                               CAN_DATA_LENGTH,
									                                               parentServer->serverInternalControlFunction,
									                                               message.get_source_control_function(),
									                                               CANIdentifier::PriorityLowest7);
								}
								break;

								case static_cast<std::uint32_t>(Function::GetNumberOfSoftKeysMessage):
								{
									std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };
									buffer[0] = static_cast<std::uint8_t>(Function::GetNumberOfSoftKeysMessage);
									buffer[1] = parentServer->get_number_of_navigation_soft_keys(); // No navigation softkeys
									buffer[2] = 0xFF; // Reserved
									buffer[3] = 0xFF; // Reserved
									buffer[4] = parentServer->get_soft_key_descriptor_x_pixel_width(); // Pixel width of X softkey descriptor
									buffer[5] = parentServer->get_soft_key_descriptor_y_pixel_width(); // Pixel width of Y softkey descriptor
									buffer[6] = parentServer->get_number_of_possible_virtual_soft_keys_in_soft_key_mask(); // Number of possible virtual Soft Keys in a Soft Key Mask
									buffer[7] = parentServer->get_number_of_physical_soft_keys(); // No physical softkeys

									CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
									                                               buffer.data(),
									                                               CAN_DATA_LENGTH,
									                                               parentServer->serverInternalControlFunction,
									                                               message.get_source_control_function(),
									                                               CANIdentifier::PriorityLowest7);
								}
								break;

								case static_cast<std::uint32_t>(Function::GetTextFontDataMessage):
								{
									std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };
									buffer[0] = static_cast<std::uint8_t>(Function::GetTextFontDataMessage);
									buffer[1] = 0xFF; // Reserved
									buffer[2] = 0xFF; // Reserved
									buffer[3] = 0xFF; // Reserved
									buffer[4] = 0xFF; // Reserved
									buffer[5] = parentServer->get_supported_small_fonts_bitfield(); // Say we support all small fonts
									buffer[6] = parentServer->get_supported_large_fonts_bitfield(); // Say we support all large fonts
									buffer[7] = 0x8F; // Support normal, bold, italic, proportional
									CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
									                                               buffer.data(),
									                                               CAN_DATA_LENGTH,
									                                               parentServer->serverInternalControlFunction,
									                                               message.get_source_control_function(),
									                                               CANIdentifier::PriorityLowest7);
								}
								break;

								case static_cast<std::uint32_t>(Function::GetHardwareMessage):
								{
									std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };
									buffer[0] = static_cast<std::uint8_t>(Function::GetHardwareMessage);
									buffer[1] = parentServer->get_powerup_time();
									buffer[2] = static_cast<std::uint8_t>(parentServer->get_graphic_mode()); // 256 Colour Mode by default
									buffer[3] = 0x0F; // Support pointing event message
									buffer[4] = (parentServer->get_data_mask_area_size_x_pixels() & 0xFF); // X Pixels LSB
									buffer[5] = (parentServer->get_data_mask_area_size_x_pixels() >> 8); // X Pixels MSB
									buffer[6] = (parentServer->get_data_mask_area_size_y_pixels() & 0xFF); // Y Pixels LSB
									buffer[7] = (parentServer->get_data_mask_area_size_y_pixels() >> 8); // Y Pixels MSB
									CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
									                                               buffer.data(),
									                                               CAN_DATA_LENGTH,
									                                               parentServer->serverInternalControlFunction,
									                                               message.get_source_control_function(),
									                                               CANIdentifier::PriorityLowest7);
								}
								break;

								case static_cast<std::uint32_t>(Function::GetSupportedWidecharsMessage):
								{
									std::vector<std::uint8_t> wideCharRangeArray;
									std::uint8_t numberOfRanges = 0;
									std::uint8_t codePlane = data.at(1);
									std::uint16_t firstWideCharInInquiryRange = static_cast<std::uint16_t>(data.at(2)) | (static_cast<std::uint16_t>(data.at(3)) << 8);
									std::uint16_t lastWideCharInInquiryRange = static_cast<std::uint16_t>(data.at(4)) | (static_cast<std::uint16_t>(data.at(5)) << 8);
									auto errorCode = parentServer->get_supported_wide_chars(codePlane, firstWideCharInInquiryRange, lastWideCharInInquiryRange, numberOfRanges, wideCharRangeArray);

									std::vector<std::uint8_t> buffer;
									buffer.push_back(static_cast<std::uint8_t>(Function::GetSupportedWidecharsMessage));
									buffer.push_back(codePlane);
									buffer.push_back(static_cast<std::uint8_t>(firstWideCharInInquiryRange & 0xFF));
									buffer.push_back(static_cast<std::uint8_t>((firstWideCharInInquiryRange >> 8) & 0xFF));
									buffer.push_back(static_cast<std::uint8_t>(lastWideCharInInquiryRange & 0xFF));
									buffer.push_back(static_cast<std::uint8_t>((lastWideCharInInquiryRange >> 8) & 0xFF));
									buffer.push_back(static_cast<std::uint8_t>(errorCode));
									buffer.push_back(numberOfRanges);

									for (const auto &range : wideCharRangeArray)
									{
										buffer.push_back(range);
									}
									CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
									                                               buffer.data(),
									                                               static_cast<std::uint32_t>(buffer.size()),
									                                               parentServer->serverInternalControlFunction,
									                                               message.get_source_control_function(),
									                                               CANIdentifier::PriorityLowest7);
								}
								break;

								case static_cast<std::uint32_t>(Function::GetVersionsMessage):
								{
									auto versions = parentServer->get_versions(message.get_source_control_function()->get_NAME());

									std::vector<std::uint8_t> buffer;
									buffer.push_back(static_cast<std::uint32_t>(Function::GetVersionsResponse));

									CANStackLogger::debug("[VT Server]: Client %u requests stored versions", message.get_source_control_function()->get_address());
									if (0 != (versions.size() % 7))
									{
										CANStackLogger::error("[VT Server]: get_versions returned illegal version lengths!");
									}

									buffer.push_back(static_cast<std::uint8_t>(versions.size() / 7));

									for (const auto &versionByte : versions)
									{
										buffer.push_back(versionByte);
									}

									while (buffer.size() < CAN_DATA_LENGTH)
									{
										buffer.push_back(0xFF);
									}
									CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
									                                               buffer.data(),
									                                               static_cast<std::uint32_t>(buffer.size()),
									                                               parentServer->serverInternalControlFunction,
									                                               message.get_source_control_function(),
									                                               CANIdentifier::PriorityLowest7);
								}
								break;

								case static_cast<std::uint32_t>(Function::LoadVersionCommand):
								{
									constexpr std::uint8_t VERSION_LABEL_LENGTH = 7;
									std::uint8_t errorCodes = 0x01; // Version label incorrect
									std::vector<std::uint8_t> versionLabel;

									versionLabel.reserve(VERSION_LABEL_LENGTH);

									for (std::uint_fast8_t i = 0; i < VERSION_LABEL_LENGTH; i++)
									{
										versionLabel.push_back(data[i + 1]);
									}

									auto loadedVersion = parentServer->load_version(versionLabel, message.get_source_control_function()->get_NAME());
									if (!loadedVersion.empty())
									{
										cf->add_iop_raw_data(loadedVersion);
										errorCodes = 0;
									}

									if (cf->get_any_object_pools())
									{
										cf->start_parsing_thread();
										CANStackLogger::debug("[VT Server]: Starting parsing thread for loaded pool data.");
									}
									std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };
									buffer[0] = static_cast<std::uint8_t>(Function::LoadVersionCommand);
									buffer[1] = 0xFF; // Reserved
									buffer[2] = 0xFF; // Reserved
									buffer[3] = 0xFF; // Reserved
									buffer[4] = 0xFF; // Reserved
									buffer[5] = errorCodes;
									buffer[6] = 0xFF; // Reserved
									buffer[7] = 0xFF; // Reserved
									CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
									                                               buffer.data(),
									                                               CAN_DATA_LENGTH,
									                                               parentServer->serverInternalControlFunction,
									                                               message.get_source_control_function(),
									                                               CANIdentifier::PriorityLowest7);
								}
								break;

								case static_cast<std::uint32_t>(Function::StoreVersionCommand):
								{
									if (cf->get_any_object_pools())
									{
										constexpr std::uint8_t VERSION_LABEL_LENGTH = 7;
										std::string cfName = std::to_string(cf->get_control_function()->get_NAME().get_full_name());
										std::vector<std::uint8_t> versionLabel;
										bool allPoolsSaved = true;
										versionLabel.reserve(VERSION_LABEL_LENGTH);

										for (std::uint_fast8_t i = 0; i < VERSION_LABEL_LENGTH; i++)
										{
											versionLabel.push_back(static_cast<char>(data[i + 1]));
										}

										for (std::size_t i = 0; i < cf->get_number_iop_files(); i++)
										{
											bool didSave = parentServer->save_version(cf->get_iop_raw_data(i), versionLabel, message.get_source_control_function()->get_NAME());

											if (didSave)
											{
												CANStackLogger::info("[VT Server]: Object pool %u for NAME %s was stored", i, cfName);
											}
											else
											{
												CANStackLogger::warn("[VT Server]: Object pool %u for NAME %s could not be stored.", i, cfName);
												allPoolsSaved = false;
												break;
											}
										}

										std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };
										buffer[0] = static_cast<std::uint8_t>(Function::StoreVersionCommand);
										buffer[1] = 0xFF; // Reserved
										buffer[2] = 0xFF; // Reserved
										buffer[3] = 0xFF; // Reserved
										buffer[4] = 0xFF; // Reserved
										if (allPoolsSaved)
										{
											buffer[5] = 0; // No error
										}
										else
										{
											buffer[5] = 0x04; // Any other error
										}
										buffer[6] = 0xFF; // Reserved
										buffer[7] = 0xFF; // Reserved
										CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
										                                               buffer.data(),
										                                               CAN_DATA_LENGTH,
										                                               parentServer->serverInternalControlFunction,
										                                               message.get_source_control_function(),
										                                               CANIdentifier::PriorityLowest7);
									}
									else
									{
										// Whomever this is is being bad, send them a NACK
										parentServer->send_acknowledgement(AcknowledgementType::Negative, static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal), parentServer->serverInternalControlFunction, cf->get_control_function());
									}
								}
								break;

								case static_cast<std::uint32_t>(Function::EndOfObjectPoolMessage):
								{
									if (cf->get_any_object_pools())
									{
										cf->start_parsing_thread();
									}
									else
									{
										CANStackLogger::warn("[VT Server]: End of object pool message ignored - no object pools are loaded for the source control function");
									}
								}
								break;

								case static_cast<std::uint32_t>(Function::WorkingSetMaintenanceMessage):
								{
									if (0 != cf->get_working_set_maintenance_message_timestamp_ms())
									{
										cf->set_working_set_maintenance_message_timestamp_ms(SystemTiming::get_timestamp_ms());
									}
								}
								break;

								case static_cast<std::uint32_t>(Function::ChangeNumericValueCommand):
								{
									std::uint32_t value = (static_cast<std::uint32_t>(data[4]) | (static_cast<std::uint32_t>(data[5]) << 8) | (static_cast<std::uint32_t>(data[6]) << 16) | (static_cast<std::uint32_t>(data[7]) << 24));
									auto objectId = static_cast<std::uint16_t>(static_cast<std::uint16_t>(data[1]) | (static_cast<std::uint16_t>(data[2]) << 8));
									auto lTargetObject = cf->get_object_by_id(objectId);

									if (nullptr != lTargetObject)
									{
										switch (lTargetObject->get_object_type())
										{
											case VirtualTerminalObjectType::InputBoolean:
											{
												std::static_pointer_cast<InputBoolean>(lTargetObject)->set_value(value);
												parentServer->onChangeNumericValueEventDispatcher.call(cf, objectId, value);
												parentServer->send_change_numeric_value_response(objectId, 0, value, cf->get_control_function());
											}
											break;

											case VirtualTerminalObjectType::InputNumber:
											{
												std::static_pointer_cast<InputNumber>(lTargetObject)->set_value(value);
												parentServer->onChangeNumericValueEventDispatcher.call(cf, objectId, value);
												parentServer->send_change_numeric_value_response(objectId, 0, value, cf->get_control_function());
											}
											break;

											case VirtualTerminalObjectType::InputList:
											{
												std::static_pointer_cast<InputList>(lTargetObject)->set_value(value);
												parentServer->onChangeNumericValueEventDispatcher.call(cf, objectId, value);
												parentServer->send_change_numeric_value_response(objectId, 0, value, cf->get_control_function());
											}
											break;

											case VirtualTerminalObjectType::OutputNumber:
											{
												std::static_pointer_cast<OutputNumber>(lTargetObject)->set_value(value);
												parentServer->onChangeNumericValueEventDispatcher.call(cf, objectId, value);
												parentServer->send_change_numeric_value_response(objectId, 0, value, cf->get_control_function());
											}
											break;

											case VirtualTerminalObjectType::OutputList:
											{
												std::static_pointer_cast<OutputList>(lTargetObject)->set_value(value);
												parentServer->onChangeNumericValueEventDispatcher.call(cf, objectId, value);
												parentServer->send_change_numeric_value_response(objectId, 0, value, cf->get_control_function());
											}
											break;

											case VirtualTerminalObjectType::OutputMeter:
											{
												std::static_pointer_cast<OutputMeter>(lTargetObject)->set_value(value);
												parentServer->onChangeNumericValueEventDispatcher.call(cf, objectId, value);
												parentServer->send_change_numeric_value_response(objectId, 0, value, cf->get_control_function());
											}
											break;

											case VirtualTerminalObjectType::OutputLinearBarGraph:
											{
												std::static_pointer_cast<OutputLinearBarGraph>(lTargetObject)->set_value(value);
												parentServer->onChangeNumericValueEventDispatcher.call(cf, objectId, value);
												parentServer->send_change_numeric_value_response(objectId, 0, value, cf->get_control_function());
											}
											break;

											case VirtualTerminalObjectType::OutputArchedBarGraph:
											{
												std::static_pointer_cast<OutputArchedBarGraph>(lTargetObject)->set_value(value);
												parentServer->onChangeNumericValueEventDispatcher.call(cf, objectId, value);
												parentServer->send_change_numeric_value_response(objectId, 0, value, cf->get_control_function());
											}
											break;

											case VirtualTerminalObjectType::NumberVariable:
											{
												std::static_pointer_cast<NumberVariable>(lTargetObject)->set_value(value);
												parentServer->onChangeNumericValueEventDispatcher.call(cf, objectId, value);
												parentServer->send_change_numeric_value_response(objectId, 0, value, cf->get_control_function());
											}
											break;

											case VirtualTerminalObjectType::ObjectPointer:
											{
												std::static_pointer_cast<ObjectPointer>(lTargetObject)->pop_child();
												std::static_pointer_cast<ObjectPointer>(lTargetObject)->add_child(value, 0, 0);
												parentServer->onChangeNumericValueEventDispatcher.call(cf, objectId, value);
												parentServer->send_change_numeric_value_response(objectId, 0, value, cf->get_control_function());
											}
											break;

											case VirtualTerminalObjectType::ExternalObjectPointer:
											{
												std::uint16_t externalReferenceNAMEObjectIdD = (static_cast<std::uint16_t>(data[4]) | (static_cast<std::uint16_t>(data[5]) << 8));
												std::uint16_t referencedObjectID = (static_cast<std::uint16_t>(data[6]) | (static_cast<std::uint16_t>(data[7]) << 8));
												std::static_pointer_cast<ExternalObjectPointer>(lTargetObject)->set_external_reference_name_id(externalReferenceNAMEObjectIdD);
												std::static_pointer_cast<ExternalObjectPointer>(lTargetObject)->set_external_object_id(referencedObjectID);
												parentServer->send_change_numeric_value_response(objectId, 0, value, cf->get_control_function());
												// Todo: event dispatcher
											}
											break;

											case VirtualTerminalObjectType::Animation:
											{
												//Todo std::static_pointer_cast<Animation>(lTargetObject)->set_value(value);
												// parentServer->onChangeNumericValueEventDispatcher.call(objectId, value);
												parentServer->send_change_numeric_value_response(objectId, (1 << static_cast<std::uint8_t>(ChangeNumericValueErrorBit::AnyOtherError)), value, cf->get_control_function());
											}
											break;

											default:
											{
												parentServer->send_change_numeric_value_response(objectId, (1 << static_cast<std::uint8_t>(ChangeNumericValueErrorBit::InvalidObjectID)), value, cf->get_control_function());
											}
											break;
										}
									}
									else
									{
										parentServer->send_change_numeric_value_response(objectId, (1 << static_cast<std::uint8_t>(ChangeNumericValueErrorBit::InvalidObjectID)), value, cf->get_control_function());
									}
								}
								break;

								case static_cast<std::uint32_t>(Function::HideShowObjectCommand):
								{
									std::uint16_t objectId = static_cast<std::uint16_t>(static_cast<std::uint16_t>(data[1]) | (static_cast<std::uint16_t>(data[2]) << 8));
									auto lTargetObject = cf->get_object_by_id(objectId);

									if ((nullptr != lTargetObject) && (VirtualTerminalObjectType::Container == lTargetObject->get_object_type()))
									{
										std::static_pointer_cast<Container>(lTargetObject)->set_hidden(0 != data[3]);
										parentServer->send_hide_show_object_response(objectId, 0, (0 != data[3]), cf->get_control_function());
										parentServer->onHideShowObjectEventDispatcher.call(cf, objectId, (0 != data[3]));
									}
									else
									{
										parentServer->send_hide_show_object_response(objectId, (1 << static_cast<std::uint8_t>(HideShowObjectErrorBit::InvalidObjectID)), (0 != data[3]), cf->get_control_function());
									}
								}
								break;

								case static_cast<std::uint32_t>(Function::EnableDisableObjectCommand):
								{
									std::uint16_t objectId = static_cast<std::uint16_t>(static_cast<std::uint16_t>(data[1]) | (static_cast<std::uint16_t>(data[2]) << 8));
									auto lTargetObject = cf->get_object_by_id(objectId);

									if (nullptr != lTargetObject)
									{
										if (data[3] <= 1)
										{
											switch (lTargetObject->get_object_type())
											{
												case VirtualTerminalObjectType::InputBoolean:
												{
													std::static_pointer_cast<InputBoolean>(lTargetObject)->set_enabled((0 != data[3]));
													parentServer->send_enable_disable_object_response(objectId, 0, (0 != data[3]), cf->get_control_function());
													parentServer->onEnableDisableObjectEventDispatcher.call(cf, objectId, (0 != data[3]));
												}
												break;

												case VirtualTerminalObjectType::InputList:
												{
													std::static_pointer_cast<InputList>(lTargetObject)->set_option(InputList::Options::Enabled, (0 != data[3]));
													parentServer->send_enable_disable_object_response(objectId, 0, (0 != data[3]), cf->get_control_function());
													parentServer->onEnableDisableObjectEventDispatcher.call(cf, objectId, (0 != data[3]));
												}
												break;

												case VirtualTerminalObjectType::InputString:
												{
													std::static_pointer_cast<InputString>(lTargetObject)->set_enabled((0 != data[3]));
													parentServer->send_enable_disable_object_response(objectId, 0, (0 != data[3]), cf->get_control_function());
													parentServer->onEnableDisableObjectEventDispatcher.call(cf, objectId, (0 != data[3]));
												}
												break;

												case VirtualTerminalObjectType::InputNumber:
												{
													std::static_pointer_cast<InputNumber>(lTargetObject)->set_option2(InputNumber::Options2::Enabled, (0 != data[3]));
													parentServer->send_enable_disable_object_response(objectId, 0, (0 != data[3]), cf->get_control_function());
													parentServer->onEnableDisableObjectEventDispatcher.call(cf, objectId, (0 != data[3]));
												}
												break;

												case VirtualTerminalObjectType::Button:
												{
													std::static_pointer_cast<Button>(lTargetObject)->set_option(Button::Options::Disabled, (0 == data[3]));
													parentServer->send_enable_disable_object_response(objectId, 0, (0 != data[3]), cf->get_control_function());
													parentServer->onEnableDisableObjectEventDispatcher.call(cf, objectId, (0 != data[3]));
												}
												break;

												default:
												{
													parentServer->send_enable_disable_object_response(objectId, (1 << static_cast<std::uint8_t>(EnableDisableObjectErrorBit::InvalidObjectID)), (0 != data[3]), cf->get_control_function());
												}
												break;
											}
										}
										else
										{
											parentServer->send_enable_disable_object_response(objectId, (1 << static_cast<std::uint8_t>(EnableDisableObjectErrorBit::InvalidEnableDisableCommandValue)), (0 != data[3]), cf->get_control_function());
										}
									}
									else
									{
										parentServer->send_enable_disable_object_response(objectId, (1 << static_cast<std::uint8_t>(EnableDisableObjectErrorBit::InvalidObjectID)), (0 != data[3]), cf->get_control_function());
									}
								}
								break;

								case static_cast<std::uint32_t>(Function::ChangeChildLocationCommand):
								{
									auto parentObjectId = static_cast<std::uint16_t>(static_cast<std::uint16_t>(data[1]) | (static_cast<std::uint16_t>(data[2]) << 8));
									auto objectID = static_cast<std::uint16_t>(static_cast<std::uint16_t>(data[3]) | (static_cast<std::uint16_t>(data[4]) << 8));
									auto parentObject = cf->get_object_by_id(parentObjectId);

									if (nullptr != parentObject)
									{
										auto lTargetObject = cf->get_object_by_id(objectID);

										if (nullptr != lTargetObject)
										{
											std::int8_t xRelativeChange = static_cast<std::int8_t>(static_cast<std::int16_t>(data[5]) - 127);
											std::int8_t yRelativeChange = static_cast<std::int8_t>(static_cast<std::int16_t>(data[6]) - 127);
											bool anyObjectMatched = anyObjectMatched = parentObject->offset_all_children_x_with_id(objectID, xRelativeChange, yRelativeChange);

											parentServer->onChangeChildLocationEventDispatcher.call(cf, parentObjectId, objectID, xRelativeChange, yRelativeChange);
											parentServer->send_change_child_location_response(parentObjectId, objectID, anyObjectMatched ? 0 : (1 << static_cast<std::uint8_t>(ChangeChildLocationErrorBit::TargetObjectDoesNotExistOrIsNotApplicable)), cf->get_control_function());
										}
										else
										{
											parentServer->send_change_child_location_response(parentObjectId, objectID, (1 << static_cast<std::uint8_t>(ChangeChildLocationErrorBit::TargetObjectDoesNotExistOrIsNotApplicable)), cf->get_control_function());
										}
									}
									else
									{
										parentServer->send_change_child_location_response(parentObjectId, objectID, (1 << static_cast<std::uint8_t>(ChangeChildLocationErrorBit::ParentObjectDoesntExistOrIsNotAParentOfSpecifiedObject)), cf->get_control_function());
									}
								}
								break;

								case static_cast<std::uint32_t>(Function::ChangeActiveMaskCommand):
								{
									auto workingSetObjectId = static_cast<std::uint16_t>(static_cast<std::uint16_t>(data[1]) | (static_cast<std::uint16_t>(data[2]) << 8));
									auto newActiveMaskObjectId = static_cast<std::uint16_t>(static_cast<std::uint16_t>(data[3]) | (static_cast<std::uint16_t>(data[4]) << 8));
									auto workingSetObject = cf->get_object_by_id(workingSetObjectId);

									if (nullptr != workingSetObject)
									{
										if (nullptr != cf->get_object_by_id(newActiveMaskObjectId))
										{
											std::static_pointer_cast<WorkingSet>(workingSetObject)->set_active_mask(newActiveMaskObjectId);
											parentServer->send_change_active_mask_response(workingSetObjectId, 0, cf->get_control_function());
											parentServer->onChangeActiveMaskEventDispatcher.call(cf, workingSetObjectId, newActiveMaskObjectId);
										}
										else
										{
											parentServer->send_change_active_mask_response(workingSetObjectId, (1 << static_cast<std::uint8_t>(ChangeActiveMaskErrorBit::InvalidMaskObjectID)), cf->get_control_function());
										}
									}
									else
									{
										parentServer->send_change_active_mask_response(workingSetObjectId, (1 << static_cast<std::uint8_t>(ChangeActiveMaskErrorBit::InvalidWorkingSetObjectID)), cf->get_control_function());
									}
								}
								break;

								default:
									break;
							}
						}
						break;

						default:
							break;
					}
					break;
				}
			}
		}
	}

	bool VirtualTerminalServer::send_acknowledgement(AcknowledgementType type, std::uint32_t parameterGroupNumber, std::shared_ptr<InternalControlFunction> source, std::shared_ptr<ControlFunction> destination)
	{
		bool retVal = false;

		if ((nullptr != source) && (nullptr != destination))
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer;

			buffer[0] = static_cast<std::uint8_t>(type);
			buffer[1] = 0xFF;
			buffer[2] = 0xFF;
			buffer[3] = 0xFF;
			buffer[4] = destination->get_address();
			buffer[5] = static_cast<std::uint8_t>(parameterGroupNumber & 0xFF);
			buffer[6] = static_cast<std::uint8_t>((parameterGroupNumber >> 8) & 0xFF);
			buffer[7] = static_cast<std::uint8_t>((parameterGroupNumber >> 16) & 0xFF);

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::Acknowledge),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        source,
			                                                        nullptr,
			                                                        CANIdentifier::PriorityLowest7);
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_change_active_mask_response(std::uint16_t newMaskObjectID, std::uint8_t errorBitfield, std::shared_ptr<ControlFunction> destination)
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
				static_cast<std::uint8_t>(Function::ChangeActiveMaskCommand),
				static_cast<std::uint8_t>(newMaskObjectID & 0xFF),
				static_cast<std::uint8_t>((newMaskObjectID >> 8) & 0xFF),
				errorBitfield,
				0xFF,
				0xFF,
				0xFF,
				0xFF
			};

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        CANIdentifier::PriorityLowest7);
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_change_child_location_response(std::uint16_t parentObjectID, std::uint16_t objectID, std::uint8_t errorBitfield, std::shared_ptr<ControlFunction> destination)
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer;

			buffer[0] = static_cast<std::uint8_t>(Function::ChangeChildLocationCommand);
			buffer[1] = static_cast<std::uint8_t>(parentObjectID & 0xFF);
			buffer[2] = static_cast<std::uint8_t>(parentObjectID >> 8);
			buffer[3] = static_cast<std::uint8_t>(objectID & 0xFF);
			buffer[4] = static_cast<std::uint8_t>(objectID >> 8);
			buffer[5] = errorBitfield;
			buffer[6] = 0xFF;
			buffer[7] = 0xFF;

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        CANIdentifier::PriorityLowest7);
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_button_activation_message(KeyActivationCode activationCode, std::uint16_t objectId, std::uint16_t parentObjectId, std::uint8_t keyNumber, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer;

			buffer[0] = static_cast<std::uint8_t>(Function::ButtonActivationMessage);
			buffer[1] = static_cast<std::uint8_t>(activationCode);
			buffer[2] = static_cast<std::uint8_t>(objectId & 0xFF);
			buffer[3] = static_cast<std::uint8_t>(objectId >> 8);
			buffer[4] = static_cast<std::uint8_t>(parentObjectId & 0xFF);
			buffer[5] = static_cast<std::uint8_t>(parentObjectId >> 8);
			buffer[6] = keyNumber;
			buffer[7] = 0xFF; // Reserved TODO: TAN

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        CANIdentifier::PriorityLowest7);
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_change_numeric_value_message(std::uint16_t objectId, std::uint32_t value, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {

				static_cast<std::uint8_t>(Function::VTChangeNumericValueMessage),
				static_cast<std::uint8_t>(objectId & 0xFF),
				static_cast<std::uint8_t>((objectId >> 8) & 0xFF),
				0xFF, // TODO: TAN, version 6
				static_cast<std::uint8_t>(value & 0xFF),
				static_cast<std::uint8_t>((value >> 8) & 0xFF),
				static_cast<std::uint8_t>((value >> 16) & 0xFF),
				static_cast<std::uint8_t>((value >> 24) & 0xFF)
			};

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        CANIdentifier::PriorityLowest7);
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_select_input_object_message(std::uint16_t objectId, bool isObjectSelected, bool isObjectOpenForInput, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {

				static_cast<std::uint8_t>(Function::VTSelectInputObjectMessage),
				static_cast<std::uint8_t>(objectId & 0xFF),
				static_cast<std::uint8_t>((objectId >> 8) & 0xFF),
				static_cast<std::uint8_t>(isObjectSelected),
				static_cast<std::uint8_t>(isObjectOpenForInput),
				0xFF,
				0xFF,
				0xFF // Reserved TODO: TAN
			};

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        CANIdentifier::PriorityLowest7);
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_soft_key_activation_message(KeyActivationCode activationCode, std::uint16_t objectId, std::uint16_t parentObjectId, std::uint8_t keyNumber, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {

				static_cast<std::uint8_t>(Function::SoftKeyActivationMessage),
				static_cast<std::uint8_t>(activationCode),
				static_cast<std::uint8_t>(objectId & 0xFF),
				static_cast<std::uint8_t>(objectId >> 8),
				static_cast<std::uint8_t>(parentObjectId & 0xFF),
				static_cast<std::uint8_t>(parentObjectId >> 8),
				keyNumber,
				0xFF // Reserved TODO: TAN
			};

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        CANIdentifier::PriorityLowest7);
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_change_numeric_value_response(std::uint16_t objectID, std::uint8_t errorBitfield, std::uint32_t value, std::shared_ptr<ControlFunction> destination)
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer;

			buffer[0] = static_cast<std::uint8_t>(Function::ChangeNumericValueCommand);
			buffer[1] = static_cast<std::uint8_t>(objectID & 0xFF);
			buffer[2] = static_cast<std::uint8_t>(objectID >> 8);
			buffer[3] = errorBitfield;
			buffer[4] = static_cast<std::uint8_t>(value & 0xFF);
			buffer[5] = static_cast<std::uint8_t>(value >> 8);
			buffer[6] = static_cast<std::uint8_t>(value >> 16);
			buffer[7] = static_cast<std::uint8_t>(value >> 24);

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        CANIdentifier::PriorityLowest7);
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_enable_disable_object_response(std::uint16_t objectID, std::uint8_t errorBitfield, bool value, std::shared_ptr<ControlFunction> destination)
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer;

			buffer[0] = static_cast<std::uint8_t>(Function::EnableDisableObjectCommand);
			buffer[1] = static_cast<std::uint8_t>(objectID & 0xFF);
			buffer[2] = static_cast<std::uint8_t>(objectID >> 8);
			buffer[3] = value;
			buffer[4] = errorBitfield;
			buffer[5] = 0xFF;
			buffer[6] = 0xFF;
			buffer[7] = 0xFF;

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        CANIdentifier::PriorityLowest7);
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_end_of_object_pool_response(bool success,
	                                                             std::uint16_t parentIDOfFaultingObject,
	                                                             std::uint16_t faultingObjectID,
	                                                             std::uint8_t errorCodes,
	                                                             std::shared_ptr<ControlFunction> destination)
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };

		buffer[0] = static_cast<std::uint8_t>(Function::EndOfObjectPoolMessage);
		buffer[1] = (success ? 0x00 : 0x01); // Error in object pool is 0x01, no error is 0x00
		buffer[2] = (parentIDOfFaultingObject & 0xFF);
		buffer[3] = (parentIDOfFaultingObject >> 8);
		buffer[4] = (faultingObjectID & 0xFF);
		buffer[5] = (faultingObjectID >> 8);
		buffer[6] = errorCodes;
		buffer[7] = 0xFF; // Reserved

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      serverInternalControlFunction,
		                                                      destination,
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalServer::send_hide_show_object_response(std::uint16_t objectID, std::uint8_t errorBitfield, bool value, std::shared_ptr<ControlFunction> destination)
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };

		buffer[0] = static_cast<std::uint8_t>(Function::HideShowObjectCommand);
		buffer[1] = (objectID & 0xFF);
		buffer[2] = ((objectID >> 8) & 0xFF);
		buffer[3] = static_cast<std::uint8_t>(value);
		buffer[4] = errorBitfield;
		buffer[5] = 0xFF; // Reserved
		buffer[6] = 0xFF; // Reserved
		buffer[7] = 0xFF; // Reserved

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      serverInternalControlFunction,
		                                                      destination,
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalServer::send_status_message()
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };

		buffer[0] = static_cast<std::uint8_t>(Function::VTStatusMessage);
		buffer[1] = activeWorkingSetMasterAddress;
		buffer[2] = (activeWorkingSetDataMaskObjectID & 0xFF);
		buffer[3] = ((activeWorkingSetDataMaskObjectID >> 8) & 0xFF);
		buffer[4] = (activeWorkingSetSoftkeyMaskObjectID & 0xFF);
		buffer[5] = ((activeWorkingSetSoftkeyMaskObjectID >> 8) & 0xFF);
		buffer[6] = busyCodesBitfield;
		buffer[7] = currentCommandFunctionCode;
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      serverInternalControlFunction,
		                                                      nullptr,
		                                                      CANIdentifier::PriorityLowest7);
	}

	void VirtualTerminalServer::update()
	{
		if ((isobus::SystemTiming::time_expired_ms(statusMessageTimestamp_ms, 1000)) &&
		    (send_status_message()))
		{
			statusMessageTimestamp_ms = isobus::SystemTiming::get_timestamp_ms();
		}

		for (auto &ws : managedWorkingSetList)
		{
			if (VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Success == ws->get_object_pool_processing_state())
			{
				ws->join_parsing_thread();
				send_end_of_object_pool_response(true, NULL_OBJECT_ID, NULL_OBJECT_ID, 0, ws->get_control_function());
				if (isobus::NULL_CAN_ADDRESS == activeWorkingSetMasterAddress)
				{
					activeWorkingSetMasterAddress = ws->get_control_function()->get_address();
					activeWorkingSetDataMaskObjectID = std::static_pointer_cast<WorkingSet>(ws->get_working_set_object())->get_active_mask();
				}
			}
			else if (VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Fail == ws->get_object_pool_processing_state())
			{
				ws->join_parsing_thread();
				///  @todo Get the parent object ID of the faulting object
				send_end_of_object_pool_response(true, NULL_OBJECT_ID, ws->get_object_pool_faulting_object_id(), 0, ws->get_control_function());
			}
		}
	}
}
