//================================================================================================
/// @file isobus_virtual_terminal_server_managed_working_set.cpp
///
/// @brief Defines a class that manages a VT server's active working sets.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/to_string.hpp"

namespace isobus
{

	VirtualTerminalServerManagedWorkingSet::VirtualTerminalServerManagedWorkingSet() :
	  objectPoolProcessingThread(nullptr),
	  workingSetControlFunction(nullptr),
	  processingState(ObjectPoolProcessingThreadState::None),
	  workingSetMaintenanceMessageTimestamp_ms(0),
	  workingSetID(NULL_OBJECT_ID),
	  faultingObjectID(NULL_OBJECT_ID)
	{
		initialize_colour_table();
		CANStackLogger::info("[WS]: New VT Server Object Created with no associated control function");
	}

	VirtualTerminalServerManagedWorkingSet::VirtualTerminalServerManagedWorkingSet(isobus::ControlFunction *associatedControlFunction) :
	  workingSetID(NULL_OBJECT_ID),
	  objectPoolProcessingThread(nullptr),
	  workingSetControlFunction(associatedControlFunction),
	  processingState(ObjectPoolProcessingThreadState::None)
	{
		initialize_colour_table();
		if (nullptr != associatedControlFunction)
		{
			CANStackLogger::info("[WS]: New VT Server Object Created for CF " + isobus::to_string(static_cast<int>(associatedControlFunction->get_NAME().get_full_name())));
		}
		else
		{
			CANStackLogger::info("[WS]: New VT Server Object Created with no associated control function");
		}
	}

	VirtualTerminalServerManagedWorkingSet::~VirtualTerminalServerManagedWorkingSet()
	{
		for (auto key = vtObjectTree.begin(); key != vtObjectTree.end(); ++key)
		{
			if (nullptr != (*key).second)
			{
				delete (*key).second;
				(*key).second = nullptr;
			}
		}
	}

	void VirtualTerminalServerManagedWorkingSet::start_parsing_thread()
	{
		if (nullptr == objectPoolProcessingThread)
		{
			objectPoolProcessingThread = new std::thread([this]() { worker_thread_function(); });
		}
	}

	void VirtualTerminalServerManagedWorkingSet::join_parsing_thread()
	{
		if ((nullptr != objectPoolProcessingThread) && (objectPoolProcessingThread->joinable()))
		{
			objectPoolProcessingThread->join();
			objectPoolProcessingThread = nullptr;
			set_object_pool_processing_state(ObjectPoolProcessingThreadState::Joined);
		}
	}

	bool VirtualTerminalServerManagedWorkingSet::parse_iop_into_objects(std::uint8_t *iopData, std::uint32_t iopLength)
	{
		uint32_t remainingLength = iopLength;
		std::uint8_t *currentIopPointer = iopData;
		bool retVal = true;

		if (iopLength > 0)
		{
			while (remainingLength > 0)
			{
				if (!parse_next_object(currentIopPointer, remainingLength))
				{
					CANStackLogger::error("[WS]: Parsing object pool failed.");
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

	bool VirtualTerminalServerManagedWorkingSet::get_any_object_pools() const
	{
		return (0 != iopFilesRawData.size());
	}

	VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState VirtualTerminalServerManagedWorkingSet::get_object_pool_processing_state()
	{
		const std::lock_guard<std::mutex> lock(manangedWorkingSetMutex);
		return processingState;
	}

	std::uint16_t VirtualTerminalServerManagedWorkingSet::get_object_pool_faulting_object_id()
	{
		std::lock_guard<std::mutex> lock(manangedWorkingSetMutex);
		return faultingObjectID;
	}

	void VirtualTerminalServerManagedWorkingSet::add_iop_raw_data(std::vector<std::uint8_t> &dataToAdd)
	{
		iopFilesRawData.push_back(dataToAdd);
	}

	std::size_t VirtualTerminalServerManagedWorkingSet::get_number_iop_files() const
	{
		return iopFilesRawData.size();
	}

	std::vector<std::uint8_t> &VirtualTerminalServerManagedWorkingSet::get_iop_raw_data(std::size_t index)
	{
		return iopFilesRawData.at(index);
	}

	isobus::ControlFunction *VirtualTerminalServerManagedWorkingSet::get_control_function() const
	{
		return workingSetControlFunction;
	}

	std::uint32_t VirtualTerminalServerManagedWorkingSet::get_working_set_maintenance_message_timestamp_ms() const
	{
		return workingSetMaintenanceMessageTimestamp_ms;
	}

	void VirtualTerminalServerManagedWorkingSet::set_working_set_maintenance_message_timestamp_ms(std::uint32_t value)
	{
		workingSetMaintenanceMessageTimestamp_ms = value;
	}

	VirtualTerminalServerManagedWorkingSet::VTColourVector VirtualTerminalServerManagedWorkingSet::GetVTColor(std::uint8_t index) const
	{
		return currentColourTable[index];
	}

	bool VirtualTerminalServerManagedWorkingSet::parse_next_object(std::uint8_t *&iopData, std::uint32_t &iopLength)
	{
		bool retVal = false;

		if (iopLength > 3)
		{
			// We at least have object ID and type
			std::uint16_t decodedID = (static_cast<std::uint16_t>(iopData[0]) | (static_cast<std::uint16_t>(iopData[1]) << 8));
			VirtualTerminalObjectType decodedType = static_cast<VirtualTerminalObjectType>(iopData[2]);

			if (!get_object_id_exists(decodedID))
			{
				switch (decodedType)
				{
					case VirtualTerminalObjectType::WorkingSet:
					{
						if (NULL_OBJECT_ID == workingSetID)
						{
							workingSetID = decodedID;
							auto tempObject = new WorkingSet(&vtObjectTree);

							if (iopLength >= tempObject->get_minumum_object_lenth())
							{
								tempObject->set_id(decodedID);
								tempObject->set_background_color(iopData[3]);
								tempObject->set_selectable(iopData[4]);
								tempObject->set_active_mask(static_cast<std::uint16_t>(iopData[5]) | (static_cast<std::uint16_t>(iopData[6]) << 8));

								// Now add child objects
								const std::uint8_t childrenToFollow = iopData[7];
								const std::uint16_t sizeOfChildren = (childrenToFollow * 6); // ID, X, Y 2 bytes each
								const std::uint8_t numberOfMacrosToFollow = iopData[8];
								const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);
								const std::uint8_t numberOfLanguagesToFollow = iopData[9];
								iopLength -= 10; // Subtract the bytes we've processed so far.
								iopData += 10; // Move the pointer

								if (iopLength >= sizeOfChildren)
								{
									for (std::uint_fast8_t i = 0; i < childrenToFollow; i++)
									{
										std::uint16_t childID = (static_cast<std::uint16_t>(iopData[0]) | (static_cast<std::uint16_t>(iopData[1]) << 8));
										std::int16_t childX = static_cast<std::int16_t>(static_cast<std::int16_t>(iopData[2]) | (static_cast<std::int16_t>(iopData[3]) << 8));
										std::int16_t childY = static_cast<std::int16_t>(static_cast<std::int16_t>(iopData[4]) | (static_cast<std::int16_t>(iopData[5]) << 8));
										tempObject->add_child(childID, childX, childY);
										iopLength -= 6;
										iopData += 6;
									}

									// Next, parse macro list

									if (iopLength >= sizeOfMacros)
									{
										for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
										{
											// If the first byte is 255, then more bytes are used! 4.6.22.3
											/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
											iopLength -= 2;
											iopData += 2;
											CANStackLogger::warn("[WS]: Skipped parsing macro reference in working set (todo)");
										}

										// Next, parse language list
										if (iopLength >= (numberOfLanguagesToFollow * 2))
										{
											for (std::uint_fast8_t i = 0; i < numberOfLanguagesToFollow; i++)
											{
												std::string langCode;
												langCode.push_back(static_cast<char>(iopData[0]));
												langCode.push_back(static_cast<char>(iopData[1]));
												iopLength -= 2;
												iopData += 2;
												CANStackLogger::debug("[WS]: IOP Language parsed: " + langCode);
											}
										}
										else
										{
											CANStackLogger::error("[WS]: Not enough IOP data to parse working set language codes for object " + isobus::to_string(static_cast<int>(decodedID)));
										}
										retVal = true;
									}
									else
									{
										CANStackLogger::error("[WS]: Not enough IOP data to parse working set macros for object " + isobus::to_string(static_cast<int>(decodedID)));
									}
								}
								else
								{
									CANStackLogger::error("[WS]: Not enough IOP data to parse working set children for object " + isobus::to_string(static_cast<int>(decodedID)));
								}
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse working set object " + isobus::to_string(static_cast<int>(decodedID)));
							}

							if (retVal)
							{
								vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Multiple working set objects are not allowed in the object pool. Faulting object " + isobus::to_string(static_cast<int>(decodedID)));
						}
					}
					break;

					case VirtualTerminalObjectType::DataMask:
					{
						auto tempObject = new DataMask(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_background_color(iopData[3]);
							tempObject->add_child(static_cast<std::uint16_t>(iopData[4]) | (static_cast<std::uint16_t>(iopData[5]) << 8), 0, 0); // soft key mask
							// Now add child objects
							const std::uint8_t childrenToFollow = iopData[6];
							const std::uint16_t sizeOfChildren = (childrenToFollow * 6); // ID, X, Y 2 bytes each
							const std::uint8_t numberOfMacrosToFollow = iopData[7];
							const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);
							iopLength -= 8; // Subtract the bytes we've processed so far.
							iopData += 8; // Move the pointer

							if (iopLength >= sizeOfChildren)
							{
								for (std::uint_fast8_t i = 0; i < childrenToFollow; i++)
								{
									std::uint16_t childID = (static_cast<std::uint16_t>(iopData[0]) | (static_cast<std::uint16_t>(iopData[1]) << 8));
									std::int16_t childX = static_cast<std::int16_t>(static_cast<std::int16_t>(iopData[2]) | (static_cast<std::int16_t>(iopData[3]) << 8));
									std::int16_t childY = static_cast<std::int16_t>(static_cast<std::int16_t>(iopData[4]) | (static_cast<std::int16_t>(iopData[5]) << 8));
									tempObject->add_child(childID, childX, childY);
									iopLength -= 6;
									iopData += 6;
								}

								// Next, parse macro list

								if (iopLength >= sizeOfMacros)
								{
									for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
									{
										// If the first byte is 255, then more bytes are used! 4.6.22.3
										/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
										iopLength -= 2;
										iopData += 2;
										CANStackLogger::warn("[WS]: Skipped parsing macro reference in data mask (todo)");
									}
									retVal = true;
								}
								else
								{
									CANStackLogger::error("[WS]: Not enough IOP data to parse data mask macros for object " + isobus::to_string(static_cast<int>(decodedID)));
								}
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse data mask children for object " + isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse data mask object for object " + isobus::to_string(static_cast<int>(decodedID)));
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::AlarmMask:
					{
						auto tempObject = new AlarmMask(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_background_color(iopData[3]);
							tempObject->add_child(static_cast<std::uint16_t>(iopData[4]) | (static_cast<std::uint16_t>(iopData[5]) << 8), 0, 0); // soft key mask

							if (iopData[6] <= static_cast<std::uint8_t>(AlarmMask::Priority::Low))
							{
								tempObject->set_mask_priority(static_cast<AlarmMask::Priority>(iopData[6]));

								if (iopData[7] <= static_cast<std::uint8_t>(AlarmMask::AcousticSignal::None))
								{
									// Now add child objects
									const std::uint8_t childrenToFollow = iopData[8];
									const std::uint16_t sizeOfChildren = (childrenToFollow * 6); // ID, X, Y 2 bytes each
									const std::uint8_t numberOfMacrosToFollow = iopData[9];
									const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);
									iopLength -= 10; // Subtract the bytes we've processed so far.
									iopData += 10; // Move the pointer

									if (iopLength >= sizeOfChildren)
									{
										for (std::uint_fast8_t i = 0; i < childrenToFollow; i++)
										{
											std::uint16_t childID = (static_cast<std::uint16_t>(iopData[0]) | (static_cast<std::uint16_t>(iopData[1]) << 8));
											std::int16_t childX = (static_cast<std::int16_t>(iopData[2]) | (static_cast<std::int16_t>(iopData[3]) << 8));
											std::int16_t childY = (static_cast<std::int16_t>(iopData[4]) | (static_cast<std::int16_t>(iopData[5]) << 8));
											tempObject->add_child(childID, childX, childY);
											iopLength -= 6;
											iopData += 6;
										}

										// Next, parse macro list

										if (iopLength >= sizeOfMacros)
										{
											for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
											{
												// If the first byte is 255, then more bytes are used! 4.6.22.3
												/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
												iopLength -= 2;
												iopData += 2;
												CANStackLogger::warn("[WS]: Skipped parsing macro reference in alarm mask (todo)");
											}
											retVal = true;
										}
										else
										{
											CANStackLogger::error("[WS]: Not enough IOP data to parse alarm mask macros for object " + isobus::to_string(static_cast<int>(decodedID)));
										}
									}
									else
									{
										CANStackLogger::error("[WS]: Not enough IOP data to parse alarm mask children for object " + isobus::to_string(static_cast<int>(decodedID)));
									}
								}
								else
								{
									CANStackLogger::error("[WS]: Invalid acoustic signal priority " +
									                      isobus::to_string(static_cast<int>(iopData[7])) +
									                      " specified for alarm mask object " +
									                      isobus::to_string(static_cast<int>(decodedID)));
								}
							}
							else
							{
								CANStackLogger::error("[WS]: Invalid alarm mask priority " +
								                      isobus::to_string(static_cast<int>(iopData[6])) +
								                      " specified for alarm mask object" +
								                      isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse alarm mask object for object " + isobus::to_string(static_cast<int>(decodedID)));
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::Container:
					{
						auto tempObject = new Container(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_width((static_cast<std::uint16_t>(iopData[3]) | (static_cast<std::uint16_t>(iopData[4]) << 8)));
							tempObject->set_height((static_cast<std::uint16_t>(iopData[5]) | (static_cast<std::uint16_t>(iopData[6]) << 8)));
							tempObject->set_hidden(0 != iopData[7]);

							if (iopData[7] > 1)
							{
								CANStackLogger::warn("[WS]: Container " +
								                     isobus::to_string(static_cast<int>(decodedID)) +
								                     " hidden attribute is not a supported value. Assuming that it is hidden.");
							}

							// Now add child objects
							const std::uint8_t childrenToFollow = iopData[8];
							const std::uint16_t sizeOfChildren = (childrenToFollow * 6); // ID, X, Y 2 bytes each
							const std::uint8_t numberOfMacrosToFollow = iopData[9];
							const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);
							iopLength -= 10; // Subtract the bytes we've processed so far.
							iopData += 10; // Move the pointer

							if (iopLength >= sizeOfChildren)
							{
								for (std::uint_fast8_t i = 0; i < childrenToFollow; i++)
								{
									std::uint16_t childID = (static_cast<std::uint16_t>(iopData[0]) | (static_cast<std::uint16_t>(iopData[1]) << 8));
									std::int16_t childX = static_cast<std::int16_t>(static_cast<std::int16_t>(iopData[2]) | (static_cast<std::int16_t>(iopData[3]) << 8));
									std::int16_t childY = static_cast<std::int16_t>(static_cast<std::int16_t>(iopData[4]) | (static_cast<std::int16_t>(iopData[5]) << 8));
									tempObject->add_child(childID, childX, childY);
									iopLength -= 6;
									iopData += 6;
								}

								// Next, parse macro list

								if (iopLength >= sizeOfMacros)
								{
									for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
									{
										// If the first byte is 255, then more bytes are used! 4.6.22.3
										/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
										iopLength -= 2;
										iopData += 2;
										CANStackLogger::warn("[WS]: Skipped parsing macro reference in container object (todo)");
									}
									retVal = true;
								}
								else
								{
									CANStackLogger::error("[WS]: Not enough IOP data to parse container macros for object " + isobus::to_string(static_cast<int>(decodedID)));
								}
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse container children for object " + isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse container object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::WindowMask:
					{
						CANStackLogger::error("[WS]: Window mask not supported yet");
					}
					break;

					case VirtualTerminalObjectType::SoftKeyMask:
					{
						auto tempObject = new SoftKeyMask(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_background_color(iopData[2]);

							// Now add child objects
							const std::uint8_t childrenToFollow = iopData[4];
							const std::uint16_t sizeOfChildren = (childrenToFollow * 2); // ID 2 bytes
							const std::uint8_t numberOfMacrosToFollow = iopData[5];
							const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);
							iopLength -= 6; // Subtract the bytes we've processed so far.
							iopData += 6; // Move the pointer

							if (iopLength >= sizeOfChildren)
							{
								// For soft key masks, no x,y positions are included
								for (std::uint_fast8_t i = 0; i < childrenToFollow; i++)
								{
									std::uint16_t childID = (static_cast<std::uint16_t>(iopData[0]) | (static_cast<std::uint16_t>(iopData[1]) << 8));
									tempObject->add_child(childID, 0, 0);
									iopLength -= 2;
									iopData += 2;
								}

								// Next, parse macro list

								if (iopLength >= sizeOfMacros)
								{
									for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
									{
										// If the first byte is 255, then more bytes are used! 4.6.22.3
										/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
										iopLength -= 2;
										iopData += 2;
										CANStackLogger::warn("[WS]: Skipped parsing macro reference in soft key mask object (todo)");
									}
									retVal = true;
								}
								else
								{
									CANStackLogger::error("[WS]: Not enough IOP data to parse soft key mask macros for object " + isobus::to_string(static_cast<int>(decodedID)));
								}
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse soft key mask children for object " + isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse soft key mask object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::Key:
					{
						auto tempObject = new Key(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_background_color(iopData[3]);
							tempObject->set_key_code(iopData[4]);

							// Now add child objects
							const std::uint8_t childrenToFollow = iopData[5];
							const std::uint16_t sizeOfChildren = (childrenToFollow * 6); // ID, X, Y 2 bytes each
							const std::uint8_t numberOfMacrosToFollow = iopData[6];
							const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);
							iopLength -= 7; // Subtract the bytes we've processed so far.
							iopData += 7; // Move the pointer

							if (iopLength >= sizeOfChildren)
							{
								for (std::uint_fast8_t i = 0; i < childrenToFollow; i++)
								{
									std::uint16_t childID = (static_cast<std::uint16_t>(iopData[0]) | (static_cast<std::uint16_t>(iopData[1]) << 8));
									std::int16_t childX = static_cast<std::int16_t>(static_cast<std::int16_t>(iopData[2]) | (static_cast<std::int16_t>(iopData[3]) << 8));
									std::int16_t childY = static_cast<std::int16_t>(static_cast<std::int16_t>(iopData[4]) | (static_cast<std::int16_t>(iopData[5]) << 8));
									tempObject->add_child(childID, childX, childY);
									iopLength -= 6;
									iopData += 6;
								}

								// Next, parse macro list

								if (iopLength >= sizeOfMacros)
								{
									for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
									{
										// If the first byte is 255, then more bytes are used! 4.6.22.3
										/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
										iopLength -= 2;
										iopData += 2;
										CANStackLogger::warn("[WS]: Skipped parsing macro reference in key object (todo)");
									}
									retVal = true;
								}
								else
								{
									CANStackLogger::error("[WS]: Not enough IOP data to parse macros for key object" + isobus::to_string(static_cast<int>(decodedID)));
								}
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse key children for object" + isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to key object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::Button:
					{
						auto tempObject = new Button(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_width((static_cast<std::uint16_t>(iopData[3]) | (static_cast<std::uint16_t>(iopData[4]) << 8)));
							tempObject->set_height((static_cast<std::uint16_t>(iopData[5]) | (static_cast<std::uint16_t>(iopData[6]) << 8)));
							tempObject->set_background_color(iopData[7]);
							tempObject->set_border_colour(iopData[8]);
							tempObject->set_key_code(iopData[9]);
							tempObject->set_options(iopData[10]);

							// Now add child objects
							const std::uint8_t childrenToFollow = iopData[11];
							const std::uint16_t sizeOfChildren = (childrenToFollow * 6); // ID, X, Y 2 bytes each
							const std::uint8_t numberOfMacrosToFollow = iopData[12];
							const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);
							iopLength -= 13; // Subtract the bytes we've processed so far.
							iopData += 13; // Move the pointer

							if (iopLength >= sizeOfChildren)
							{
								for (std::uint_fast8_t i = 0; i < childrenToFollow; i++)
								{
									std::uint16_t childID = (static_cast<std::uint16_t>(iopData[0]) | (static_cast<std::uint16_t>(iopData[1]) << 8));
									std::int16_t childX = static_cast<std::int16_t>(static_cast<std::int16_t>(iopData[2]) | (static_cast<std::int16_t>(iopData[3]) << 8));
									std::int16_t childY = static_cast<std::int16_t>(static_cast<std::int16_t>(iopData[4]) | (static_cast<std::int16_t>(iopData[5]) << 8));
									tempObject->add_child(childID, childX, childY);
									iopLength -= 6;
									iopData += 6;
								}

								// Next, parse macro list
								if (iopLength >= sizeOfMacros)
								{
									for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
									{
										// If the first byte is 255, then more bytes are used! 4.6.22.3
										/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
										iopLength -= 2;
										iopData += 2;
										CANStackLogger::warn("[WS]: Skipped parsing macro reference in button object (todo)");
									}
									retVal = true;
								}
								else
								{
									CANStackLogger::error("[WS]: Not enough IOP data to parse macros for button object " + isobus::to_string(static_cast<int>(decodedID)));
								}
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse button children for object " + isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse button object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::KeyGroup:
					{
						auto tempObject = new KeyGroup(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_options(iopData[3]);
							tempObject->add_child(static_cast<std::uint16_t>(iopData[4]) | (static_cast<std::uint16_t>(iopData[5]) << 8), 0, 0); // Output string for the object's name/label
							tempObject->add_child(static_cast<std::uint16_t>(iopData[6]) | (static_cast<std::uint16_t>(iopData[7]) << 8), 0, 0); // Key group icon

							// Parse children
							const std::uint8_t numberChildrenToFollow = iopData[8];
							iopLength -= 9;
							iopData += 9;

							const int64_t iopLengthRemaining = (iopLength - (numberChildrenToFollow * 2));

							if (iopLength >= iopLengthRemaining)
							{
								if (numberChildrenToFollow <= KeyGroup::MAX_CHILD_KEYS)
								{
									for (std::uint_fast8_t i = 0; i < numberChildrenToFollow; i++)
									{
										tempObject->add_child(static_cast<std::uint16_t>(iopData[0]) | (static_cast<std::uint16_t>(iopData[1]) << 8), 0, 0);
										iopLength -= 2;
										iopData += 2;
									}

									// Now parse macros
									const std::uint8_t numberOfMacrosToFollow = iopData[0];
									iopData++;
									iopLength--;

									const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);
									if (iopLength >= sizeOfMacros)
									{
										for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
										{
											// If the first byte is 255, then more bytes are used! 4.6.22.3
											/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
											iopLength -= 2;
											iopData += 2;
											CANStackLogger::warn("[WS]: Skipped parsing macro reference in key group object (todo)");
										}
										retVal = true;
									}
									else
									{
										CANStackLogger::error("[WS]: Not enough IOP data to parse macros for key group object " + isobus::to_string(static_cast<int>(decodedID)));
									}
								}
								else
								{
									CANStackLogger::error("[WS]: Key group " + isobus::to_string(static_cast<int>(decodedID)) + " has too many child key objects! Only 4 are permitted.");
								}
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse key group object children");
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse key group object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::InputBoolean:
					{
						auto tempObject = new InputBoolean(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_background_color(iopData[3]);
							tempObject->set_width((static_cast<std::uint16_t>(iopData[4]) | (static_cast<std::uint16_t>(iopData[5]) << 8)));
							tempObject->add_child((static_cast<std::uint16_t>(iopData[6]) | (static_cast<std::uint16_t>(iopData[7]) << 8)), 0, 0); // Child Font Attribute
							tempObject->add_child(static_cast<std::uint16_t>(iopData[8]) | (static_cast<std::uint16_t>(iopData[9]) << 8), 0, 0); // Add variable reference
							tempObject->set_value(iopData[10]);
							tempObject->set_enabled(iopData[11]);

							// No children list to parse

							// Next, parse macro list
							const std::uint8_t numberOfMacrosToFollow = iopData[12];
							const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);
							iopData += 13;
							iopLength -= 13;

							if (iopLength >= sizeOfMacros)
							{
								for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
								{
									// If the first byte is 255, then more bytes are used! 4.6.22.3
									/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
									iopLength -= 2;
									iopData += 2;
									CANStackLogger::warn("[WS]: Skipped parsing macro reference in input boolean object (todo)");
								}
								retVal = true;
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse macros for input boolean object " + isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse input boolean object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::InputString:
					{
						auto tempObject = new InputString(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_width((static_cast<std::uint16_t>(iopData[3]) | (static_cast<std::uint16_t>(iopData[4]) << 8)));
							tempObject->set_height((static_cast<std::uint16_t>(iopData[5]) | (static_cast<std::uint16_t>(iopData[6]) << 8)));
							tempObject->set_background_color(iopData[7]);
							tempObject->add_child((static_cast<std::uint16_t>(iopData[8]) | (static_cast<std::uint16_t>(iopData[9]) << 8)), 0, 0); // Font Attributes
							tempObject->add_child((static_cast<std::uint16_t>(iopData[10]) | (static_cast<std::uint16_t>(iopData[11]) << 8)), 0, 0); // Input Attributes
							tempObject->set_options(iopData[12]);
							tempObject->add_child((static_cast<std::uint16_t>(iopData[13]) | (static_cast<std::uint16_t>(iopData[14]) << 8)), 0, 0); // Number variable
							tempObject->set_justification_bitfield(iopData[15]);

							const std::size_t lengthOfStringObject = iopData[16];
							const int64_t iopLengthRemaining = (iopLength - 17); // Use larger signed int to detect negative rollover

							if (iopLengthRemaining > (lengthOfStringObject + 2)) // +2 is for enabled byte and number of macros to follow
							{
								std::string tempString;
								tempString.reserve(lengthOfStringObject);

								for (std::uint_fast8_t i = 0; i < lengthOfStringObject; i++)
								{
									tempString.push_back(static_cast<char>(iopData[17 + i]));
								}
								tempObject->set_enabled(iopData[17 + lengthOfStringObject]);
								iopData += (17 + lengthOfStringObject);
								iopLength -= (17 + lengthOfStringObject);

								// Next, parse macro list
								const std::uint8_t numberOfMacrosToFollow = iopData[0];

								iopData++;
								iopLength--;

								const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);
								if (iopLength >= sizeOfMacros)
								{
									for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
									{
										// If the first byte is 255, then more bytes are used! 4.6.22.3
										/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
										iopLength -= 2;
										iopData += 2;
										CANStackLogger::warn("[WS]: Skipped parsing macro reference in input boolean string (todo)");
									}
									retVal = true;
								}
								else
								{
									CANStackLogger::error("[WS]: Not enough IOP data to parse macros for input boolean object " + isobus::to_string(static_cast<int>(decodedID)));
								}
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse input string object value");
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse input string object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::InputNumber:
					{
						auto tempObject = new InputNumber(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_width((static_cast<std::uint16_t>(iopData[3]) | (static_cast<std::uint16_t>(iopData[4]) << 8)));
							tempObject->set_height((static_cast<std::uint16_t>(iopData[5]) | (static_cast<std::uint16_t>(iopData[6]) << 8)));
							tempObject->set_background_color(iopData[7]);
							tempObject->add_child((static_cast<std::uint16_t>(iopData[8]) | (static_cast<std::uint16_t>(iopData[9]) << 8)), 0, 0); // Font Attributes
							tempObject->set_options(iopData[10]);
							tempObject->add_child((static_cast<std::uint16_t>(iopData[11]) | (static_cast<std::uint16_t>(iopData[12]) << 8)), 0, 0); // Number variable
							tempObject->set_value(static_cast<std::uint32_t>(iopData[13]) |
							                      (static_cast<std::uint32_t>(iopData[14]) << 8) |
							                      (static_cast<std::uint32_t>(iopData[15]) << 16) |
							                      (static_cast<std::uint32_t>(iopData[16]) << 24));
							tempObject->set_minimum_value(static_cast<std::uint32_t>(iopData[17]) |
							                              (static_cast<std::uint32_t>(iopData[18]) << 8) |
							                              (static_cast<std::uint32_t>(iopData[19]) << 16) |
							                              (static_cast<std::uint32_t>(iopData[20]) << 24));
							tempObject->set_maximum_value(static_cast<std::uint32_t>(iopData[21]) |
							                              (static_cast<std::uint32_t>(iopData[22]) << 8) |
							                              (static_cast<std::uint32_t>(iopData[23]) << 16) |
							                              (static_cast<std::uint32_t>(iopData[24]) << 24));
							tempObject->set_offset(static_cast<std::uint32_t>(iopData[25]) |
							                       (static_cast<std::uint32_t>(iopData[26]) << 8) |
							                       (static_cast<std::uint32_t>(iopData[27]) << 16) |
							                       (static_cast<std::uint32_t>(iopData[28]) << 24));
							float tempFloat = 0;
							std::uint8_t floatBuffer[4] = {
								iopData[29],
								iopData[30],
								iopData[31],
								iopData[32]
							};
							memcpy(&tempFloat, &floatBuffer, 4); // Feels kinda bad...

							tempObject->set_scale(tempFloat);
							tempObject->set_number_of_decimals(iopData[33]);
							tempObject->set_format(0 != iopData[34]);

							if (iopData[34] > 1)
							{
								CANStackLogger::warn("[WS]: Input number " + isobus::to_string(static_cast<int>(decodedID)) + " format byte has undefined value. Setting to exponential format.");
							}

							tempObject->set_justification_bitfield(iopData[35]);
							tempObject->set_options2(iopData[36]);

							// Parse macros
							const std::uint8_t numberOfMacrosToFollow = iopData[37];
							const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);
							iopLength -= 38;
							iopData += 38;

							if (iopLength >= sizeOfMacros)
							{
								for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
								{
									// If the first byte is 255, then more bytes are used! 4.6.22.3
									/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
									iopLength -= 2;
									iopData += 2;
									CANStackLogger::warn("[WS]: Skipped parsing macro reference in input number (todo)");
								}
								retVal = true;
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse macros for input number object " + isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse input number object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::InputList:
					{
						auto tempObject = new InputList(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_width((static_cast<std::uint16_t>(iopData[3]) | (static_cast<std::uint16_t>(iopData[4]) << 8)));
							tempObject->set_height((static_cast<std::uint16_t>(iopData[5]) | (static_cast<std::uint16_t>(iopData[6]) << 8)));
							tempObject->add_child((static_cast<std::uint16_t>(iopData[7]) | (static_cast<std::uint16_t>(iopData[8]) << 8)), 0, 0); // Number variable
							tempObject->set_value(iopData[9]);
							tempObject->set_options(iopData[11]);

							// Parse children
							const std::uint8_t numberOfListItems = iopData[10];
							iopData += 12;
							iopLength -= 12;

							if (iopLength >= (2 * numberOfListItems))
							{
								for (std::uint_fast8_t i = 0; i < numberOfListItems; i++)
								{
									std::uint16_t childID = (static_cast<std::uint16_t>(iopData[0]) | (static_cast<std::uint16_t>(iopData[1]) << 8));
									tempObject->add_child(childID, 0, 0);
									iopLength -= 2;
									iopData += 2;
								}

								// Next, parse macro list
								const std::uint8_t numberOfMacrosToFollow = iopData[0];
								iopData++;
								iopLength--;

								const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);

								if (iopLength >= sizeOfMacros)
								{
									for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
									{
										// If the first byte is 255, then more bytes are used! 4.6.22.3
										/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
										iopLength -= 2;
										iopData += 2;
										CANStackLogger::warn("[WS]: Skipped parsing macro reference in input list object (todo)");
									}
									retVal = true;
								}
								else
								{
									CANStackLogger::error("[WS]: Not enough IOP data to parse macros for input list object " + isobus::to_string(static_cast<int>(decodedID)));
								}
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse children of input list object " + isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse input list object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::OutputString:
					{
						auto tempObject = new OutputString(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_width((static_cast<std::uint16_t>(iopData[3]) | (static_cast<std::uint16_t>(iopData[4]) << 8)));
							tempObject->set_height((static_cast<std::uint16_t>(iopData[5]) | (static_cast<std::uint16_t>(iopData[6]) << 8)));
							tempObject->set_background_color(iopData[7]);
							tempObject->add_child((static_cast<std::uint16_t>(iopData[8]) | (static_cast<std::uint16_t>(iopData[9]) << 8)), 0, 0); // Font Attributes
							tempObject->set_options(iopData[10]);
							tempObject->add_child((static_cast<std::uint16_t>(iopData[11]) | (static_cast<std::uint16_t>(iopData[12]) << 8)), 0, 0); // String Variable
							tempObject->set_justification_bitfield(iopData[13]);

							const std::uint16_t stringLengthToFollow = (static_cast<std::uint16_t>(iopData[14]) | (static_cast<std::uint16_t>(iopData[15]) << 8));
							std::string tempString;
							tempString.reserve(stringLengthToFollow);
							iopData += 16;
							iopLength -= 16;

							if (iopLength >= stringLengthToFollow)
							{
								for (uint_fast8_t i = 0; i < stringLengthToFollow; i++)
								{
									tempString.push_back(static_cast<char>(iopData[0]));
									iopData++;
									iopLength--;
								}
								tempObject->set_value(tempString);

								// Parse macros
								const std::uint8_t numberOfMacrosToFollow = iopData[0];
								iopData++;
								iopLength--;

								const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);

								if (iopLength >= sizeOfMacros)
								{
									for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
									{
										// If the first byte is 255, then more bytes are used! 4.6.22.3
										/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
										iopLength -= 2;
										iopData += 2;
										CANStackLogger::warn("[WS]: Skipped parsing macro reference in output string object (todo)");
									}
									retVal = true;
								}
								else
								{
									CANStackLogger::error("[WS]: Not enough IOP data to parse macros for output string object " + isobus::to_string(static_cast<int>(decodedID)));
								}
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse output string object value");
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse output string object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::OutputNumber:
					{
						auto tempObject = new OutputNumber(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_width((static_cast<std::uint16_t>(iopData[3]) | (static_cast<std::uint16_t>(iopData[4]) << 8)));
							tempObject->set_height((static_cast<std::uint16_t>(iopData[5]) | (static_cast<std::uint16_t>(iopData[6]) << 8)));
							tempObject->set_background_color(iopData[7]);
							tempObject->add_child((static_cast<std::uint16_t>(iopData[8]) | (static_cast<std::uint16_t>(iopData[9]) << 8)), 0, 0); // Font Attributes
							tempObject->set_options(iopData[10]);
							tempObject->add_child((static_cast<std::uint16_t>(iopData[11]) | (static_cast<std::uint16_t>(iopData[12]) << 8)), 0, 0); // Number Variable
							tempObject->set_value(static_cast<std::uint32_t>(iopData[13]) |
							                      (static_cast<std::uint32_t>(iopData[14]) << 8) |
							                      (static_cast<std::uint32_t>(iopData[15]) << 16) |
							                      (static_cast<std::uint32_t>(iopData[16]) << 24));
							tempObject->set_offset(static_cast<std::uint32_t>(iopData[17]) |
							                       (static_cast<std::uint32_t>(iopData[18]) << 8) |
							                       (static_cast<std::uint32_t>(iopData[19]) << 16) |
							                       (static_cast<std::uint32_t>(iopData[20]) << 24));
							float tempFloat = 0;
							std::uint8_t floatBuffer[4] = {
								iopData[21],
								iopData[22],
								iopData[23],
								iopData[24]
							};
							memcpy(&tempFloat, &floatBuffer, 4); // Feels kinda bad...

							tempObject->set_scale(tempFloat);
							tempObject->set_number_of_decimals(iopData[25]);
							tempObject->set_format(0 != iopData[26]);

							if (iopData[26] > 1)
							{
								CANStackLogger::warn("[WS]: Output number " +
								                     isobus::to_string(static_cast<int>(decodedID)) +
								                     " format byte has undefined value. Setting to exponential format.");
							}
							tempObject->set_justification_bitfield(iopData[27]);

							// Parse Macros
							const std::uint8_t numberOfMacrosToFollow = iopData[28];
							const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);
							iopLength -= 29;
							iopData += 29;

							if (iopLength >= sizeOfMacros)
							{
								for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
								{
									// If the first byte is 255, then more bytes are used! 4.6.22.3
									/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
									iopLength -= 2;
									iopData += 2;
									CANStackLogger::warn("[WS]: Skipped parsing macro reference in output number (todo)");
								}
								retVal = true;
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse macros for output number object {}" + isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse output number object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::OutputList:
					{
						auto tempObject = new OutputList(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_width((static_cast<std::uint16_t>(iopData[3]) | (static_cast<std::uint16_t>(iopData[4]) << 8)));
							tempObject->set_height((static_cast<std::uint16_t>(iopData[5]) | (static_cast<std::uint16_t>(iopData[6]) << 8)));
							tempObject->add_child((static_cast<std::uint16_t>(iopData[7]) | (static_cast<std::uint16_t>(iopData[8]) << 8)), 0, 0); // Number Variable
							tempObject->set_value(iopData[9]);

							// Parse children
							const std::uint8_t numberOfListItems = iopData[10];
							const std::uint8_t numberOfMacrosToFollow = iopData[11];
							iopData += 12;
							iopLength -= 12;

							if (iopLength >= (2 * numberOfListItems))
							{
								for (std::uint_fast8_t i = 0; i < numberOfListItems; i++)
								{
									std::uint16_t childID = (static_cast<std::uint16_t>(iopData[0]) | (static_cast<std::uint16_t>(iopData[1]) << 8));
									tempObject->add_child(childID, 0, 0);
									iopLength -= 2;
									iopData += 2;
								}

								const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);

								if (iopLength >= sizeOfMacros)
								{
									for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
									{
										// If the first byte is 255, then more bytes are used! 4.6.22.3
										/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
										iopLength -= 2;
										iopData += 2;
										CANStackLogger::warn("[WS]: Skipped parsing macro reference in output list object (todo)");
									}
									retVal = true;
								}
								else
								{
									CANStackLogger::error("[WS]: Not enough IOP data to parse macros for output list object " + isobus::to_string(static_cast<int>(decodedID)));
								}
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse children for output list object " + isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse output list object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::OutputLine:
					{
						auto tempObject = new OutputLine(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->add_child((static_cast<std::uint16_t>(iopData[3]) | (static_cast<std::uint16_t>(iopData[4]) << 8)), 0, 0); // Line Attributes
							tempObject->set_width((static_cast<std::uint16_t>(iopData[5]) | (static_cast<std::uint16_t>(iopData[6]) << 8)));
							tempObject->set_height((static_cast<std::uint16_t>(iopData[7]) | (static_cast<std::uint16_t>(iopData[8]) << 8)));
							tempObject->set_line_direction(iopData[9]);
							iopData += 10;
							iopLength -= 10;

							// Parse macros
							const std::uint8_t numberOfMacrosToFollow = iopData[0];
							iopData++;
							iopLength--;

							const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);

							if (iopLength >= sizeOfMacros)
							{
								for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
								{
									// If the first byte is 255, then more bytes are used! 4.6.22.3
									/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
									iopLength -= 2;
									iopData += 2;
									CANStackLogger::warn("[WS]: Skipped parsing macro reference in output line object (todo)");
								}
								retVal = true;
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse macros for output line object " + isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse output line object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::OutputRectangle:
					{
						auto tempObject = new OutputRectangle(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->add_child((static_cast<std::uint16_t>(iopData[3]) | (static_cast<std::uint16_t>(iopData[4]) << 8)), 0, 0); // Line Attributes
							tempObject->set_width((static_cast<std::uint16_t>(iopData[5]) | (static_cast<std::uint16_t>(iopData[6]) << 8)));
							tempObject->set_height((static_cast<std::uint16_t>(iopData[7]) | (static_cast<std::uint16_t>(iopData[8]) << 8)));
							tempObject->set_line_suppression_bitfield(iopData[9]);
							tempObject->add_child((static_cast<std::uint16_t>(iopData[10]) | (static_cast<std::uint16_t>(iopData[11]) << 8)), 0, 0); // Fill Attributes
							iopData += 12;
							iopLength -= 12;

							// Parse macros
							const std::uint8_t numberOfMacrosToFollow = iopData[0];
							iopData++;
							iopLength--;

							const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);

							if (iopLength >= sizeOfMacros)
							{
								for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
								{
									// If the first byte is 255, then more bytes are used! 4.6.22.3
									/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
									iopLength -= 2;
									iopData += 2;
									CANStackLogger::warn("[WS]: Skipped parsing macro reference in output rectangle object (todo)");
								}
								retVal = true;
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse macros for output rectangle object " + isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse output rectangle object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::OutputEllipse:
					{
						auto tempObject = new OutputEllipse(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->add_child((static_cast<std::uint16_t>(iopData[3]) | (static_cast<std::uint16_t>(iopData[4]) << 8)), 0, 0); // Line Attributes
							tempObject->set_width((static_cast<std::uint16_t>(iopData[5]) | (static_cast<std::uint16_t>(iopData[6]) << 8)));
							tempObject->set_height((static_cast<std::uint16_t>(iopData[7]) | (static_cast<std::uint16_t>(iopData[8]) << 8)));

							if (iopData[9] <= static_cast<std::uint8_t>(OutputEllipse::EllipseType::ClosedEllipseSection))
							{
								tempObject->set_ellipse_type(static_cast<OutputEllipse::EllipseType>(iopData[9]));
								tempObject->set_start_angle(iopData[10]);
								tempObject->set_end_angle(iopData[11]);
								tempObject->add_child((static_cast<std::uint16_t>(iopData[12]) | (static_cast<std::uint16_t>(iopData[13]) << 8)), 0, 0); // Fill Attributes
								iopData += 14;
								iopLength -= 14;

								// Parse macros
								const std::uint8_t numberOfMacrosToFollow = iopData[0];
								iopData++;
								iopLength--;

								const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);

								if (iopLength >= sizeOfMacros)
								{
									for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
									{
										// If the first byte is 255, then more bytes are used! 4.6.22.3
										/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
										iopLength -= 2;
										iopData += 2;
										CANStackLogger::warn("[WS]: Skipped parsing macro reference in output ellipse object (todo)");
									}
									retVal = true;
								}
								else
								{
									CANStackLogger::error("[WS]: Not enough IOP data to parse macros for output ellipse object " + isobus::to_string(static_cast<int>(decodedID)));
								}
							}
							else
							{
								CANStackLogger::error("[WS]: Output Ellipse type is undefined for object " + isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse output ellipse object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::OutputPolygon:
					{
						auto tempObject = new OutputPolygon(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_width((static_cast<std::uint16_t>(iopData[3]) | (static_cast<std::uint16_t>(iopData[4]) << 8)));
							tempObject->set_height((static_cast<std::uint16_t>(iopData[5]) | (static_cast<std::uint16_t>(iopData[6]) << 8)));
							tempObject->add_child((static_cast<std::uint16_t>(iopData[7]) | (static_cast<std::uint16_t>(iopData[8]) << 8)), 0, 0); // Line Attributes
							tempObject->add_child((static_cast<std::uint16_t>(iopData[9]) | (static_cast<std::uint16_t>(iopData[10]) << 8)), 0, 0); // Fill Attributes

							if (iopData[11] <= 3)
							{
								tempObject->set_type(static_cast<OutputPolygon::PolygonType>(iopData[11]));

								const std::uint8_t numberOfPoints = iopData[12];
								const std::uint8_t numberOfMacrosToFollow = iopData[13];
								iopLength -= 14;
								iopData += 14;

								if (iopLength >= (numberOfPoints * 4))
								{
									for (std::uint_fast8_t i = 0; i < numberOfPoints; i++)
									{
										tempObject->add_point((static_cast<std::uint16_t>(iopData[0]) | (static_cast<std::uint16_t>(iopData[1]) << 8)), (static_cast<std::uint16_t>(iopData[2]) | (static_cast<std::uint16_t>(iopData[3]) << 8)));
										iopLength -= 4;
										iopData += 4;
									}

									const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);

									if (iopLength >= sizeOfMacros)
									{
										for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
										{
											// If the first byte is 255, then more bytes are used! 4.6.22.3
											/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
											iopLength -= 2;
											iopData += 2;
											CANStackLogger::warn("[WS]: Skipped parsing macro reference in output polygon object (todo)");
										}
										retVal = true;
									}
									else
									{
										CANStackLogger::error("[WS]: Not enough IOP data to parse macros for output polygon object " + isobus::to_string(static_cast<int>(decodedID)));
									}
								}
								else
								{
									CANStackLogger::error("[WS]: Not enough IOP data to parse output polygon child points for object " + isobus::to_string(static_cast<int>(decodedID)));
								}
							}
							else
							{
								CANStackLogger::error("[WS]: Polygon type is undefined for object " + isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse output polygon object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::OutputMeter:
					{
						auto tempObject = new OutputMeter(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_width((static_cast<std::uint16_t>(iopData[3]) | (static_cast<std::uint16_t>(iopData[4]) << 8)));
							tempObject->set_needle_colour(iopData[5]);
							tempObject->set_border_colour(iopData[6]);
							tempObject->set_arc_and_tick_colour(iopData[7]);
							tempObject->set_options(iopData[8]);
							tempObject->set_number_of_ticks(iopData[9]);
							tempObject->set_start_angle(iopData[10]);
							tempObject->set_end_angle(iopData[11]);
							tempObject->set_min_value((static_cast<std::uint16_t>(iopData[12]) | (static_cast<std::uint16_t>(iopData[13]) << 8)));
							tempObject->set_max_value((static_cast<std::uint16_t>(iopData[14]) | (static_cast<std::uint16_t>(iopData[15]) << 8)));
							tempObject->add_child((static_cast<std::uint16_t>(iopData[16]) | (static_cast<std::uint16_t>(iopData[17]) << 8)), 0, 0); // Number Variable
							tempObject->set_value((static_cast<std::uint16_t>(iopData[18]) | (static_cast<std::uint16_t>(iopData[19]) << 8)));
							const std::uint8_t numberOfMacrosToFollow = iopData[20];
							const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);
							iopData += 21;
							iopLength -= 21;

							if (iopLength >= sizeOfMacros)
							{
								for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
								{
									// If the first byte is 255, then more bytes are used! 4.6.22.3
									/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
									iopLength -= 2;
									iopData += 2;
									CANStackLogger::warn("[WS]: Skipped parsing macro reference in output meter object (todo)");
								}
								retVal = true;
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse macros for output meter object " + isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse output meter object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::OutputLinearBarGraph:
					{
						auto tempObject = new OutputLinearBarGraph(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_width((static_cast<std::uint16_t>(iopData[3]) | (static_cast<std::uint16_t>(iopData[4]) << 8)));
							tempObject->set_height((static_cast<std::uint16_t>(iopData[5]) | (static_cast<std::uint16_t>(iopData[6]) << 8)));
							tempObject->set_colour(iopData[7]);
							tempObject->set_target_line_colour(iopData[8]);
							tempObject->set_options(iopData[9]);
							tempObject->set_number_of_ticks(iopData[10]);
							tempObject->set_min_value((static_cast<std::uint16_t>(iopData[11]) | (static_cast<std::uint16_t>(iopData[12]) << 8)));
							tempObject->set_max_value((static_cast<std::uint16_t>(iopData[13]) | (static_cast<std::uint16_t>(iopData[14]) << 8)));
							tempObject->add_child((static_cast<std::uint16_t>(iopData[15]) | (static_cast<std::uint16_t>(iopData[16]) << 8)), 0, 0); // Number Variable
							tempObject->set_value((static_cast<std::uint16_t>(iopData[17]) | (static_cast<std::uint16_t>(iopData[18]) << 8)));
							tempObject->set_target_value_reference((static_cast<std::uint16_t>(iopData[19]) | (static_cast<std::uint16_t>(iopData[20]) << 8)));
							tempObject->set_target_value((static_cast<std::uint16_t>(iopData[21]) | (static_cast<std::uint16_t>(iopData[22]) << 8)));
							const std::uint8_t numberOfMacrosToFollow = iopData[23];
							const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);
							iopData += 24;
							iopLength -= 24;

							if (iopLength >= sizeOfMacros)
							{
								for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
								{
									// If the first byte is 255, then more bytes are used! 4.6.22.3
									/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
									iopLength -= 2;
									iopData += 2;
									CANStackLogger::warn("[WS]: Skipped parsing macro reference in output linear bar graph object (todo)");
								}
								retVal = true;
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse macros for output linear bar graph object " + isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse output linear bar graph object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::OutputArchedBarGraph:
					{
						auto tempObject = new OutputArchedBarGraph(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_width((static_cast<std::uint16_t>(iopData[3]) | (static_cast<std::uint16_t>(iopData[4]) << 8)));
							tempObject->set_height((static_cast<std::uint16_t>(iopData[5]) | (static_cast<std::uint16_t>(iopData[6]) << 8)));
							tempObject->set_colour(iopData[7]);
							tempObject->set_target_line_colour(iopData[8]);
							tempObject->set_options(iopData[9]);
							tempObject->set_start_angle(iopData[10]);
							tempObject->set_end_angle(iopData[11]);
							tempObject->set_bar_graph_width((static_cast<std::uint16_t>(iopData[12]) | (static_cast<std::uint16_t>(iopData[13]) << 8)));
							tempObject->set_min_value((static_cast<std::uint16_t>(iopData[14]) | (static_cast<std::uint16_t>(iopData[15]) << 8)));
							tempObject->set_max_value((static_cast<std::uint16_t>(iopData[16]) | (static_cast<std::uint16_t>(iopData[17]) << 8)));
							tempObject->add_child((static_cast<std::uint16_t>(iopData[18]) | (static_cast<std::uint16_t>(iopData[19]) << 8)), 0, 0); // Number Variable
							tempObject->set_value((static_cast<std::uint16_t>(iopData[20]) | (static_cast<std::uint16_t>(iopData[21]) << 8)));
							tempObject->set_target_value_reference((static_cast<std::uint16_t>(iopData[22]) | (static_cast<std::uint16_t>(iopData[23]) << 8)));
							tempObject->set_target_value((static_cast<std::uint16_t>(iopData[24]) | (static_cast<std::uint16_t>(iopData[25]) << 8)));
							const std::uint8_t numberOfMacrosToFollow = iopData[26];
							const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);
							iopData += 27;
							iopLength -= 27;

							if (iopLength >= sizeOfMacros)
							{
								for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
								{
									// If the first byte is 255, then more bytes are used! 4.6.22.3
									/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
									iopLength -= 2;
									iopData += 2;
									CANStackLogger::warn("[WS]: Skipped parsing macro reference in output arched bar graph object (todo)");
								}
								retVal = true;
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse macros for output arched bar graph object " + isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse output arched bar graph object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::GraphicsContext:
					{
						CANStackLogger::error("[WS]: Graphics context not supported yet (todo)");
					}
					break;

					case VirtualTerminalObjectType::Animation:
					{
						CANStackLogger::error("[WS]: Animation not supported yet (todo)");
					}
					break;

					case VirtualTerminalObjectType::PictureGraphic:
					{
						auto tempObject = new PictureGraphic(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_width((static_cast<std::uint16_t>(iopData[3]) | (static_cast<std::uint16_t>(iopData[4]) << 8)));
							tempObject->set_actual_width((static_cast<std::uint16_t>(iopData[5]) | (static_cast<std::uint16_t>(iopData[6]) << 8)));
							tempObject->set_actual_height((static_cast<std::uint16_t>(iopData[7]) | (static_cast<std::uint16_t>(iopData[8]) << 8)));

							if (iopData[9] <= static_cast<std::uint8_t>(PictureGraphic::Format::EightBitColour))
							{
								tempObject->set_format(static_cast<PictureGraphic::Format>(iopData[9]));
								tempObject->set_options(iopData[10]);
								tempObject->set_transparency_colour(iopData[11]);
								tempObject->set_number_of_bytes_in_raw_data(static_cast<std::uint32_t>(iopData[12]) |
								                                            (static_cast<std::uint32_t>(iopData[13]) << 8) |
								                                            (static_cast<std::uint32_t>(iopData[14]) << 16) |
								                                            (static_cast<std::uint32_t>(iopData[15]) << 24));
								const std::uint8_t numberOfMacrosToFollow = iopData[16];
								const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);
								iopData += 17;
								iopLength -= 17;

								if (tempObject->get_option(PictureGraphic::Options::RunLengthEncoded))
								{
									if (0 != (tempObject->get_number_of_bytes_in_raw_data() % 2))
									{
										CANStackLogger::error("[WS]: Picture graphic has RLE but an odd number of data bytes. Object: " + isobus::to_string(static_cast<int>(decodedID)));
									}
									else
									{
										// Decode the RLE
										for (std::uint_fast32_t i = 0; i < (tempObject->get_number_of_bytes_in_raw_data() / 2); i++)
										{
											for (std::size_t j = 0; j < iopData[0]; j++)
											{
												tempObject->add_raw_data(iopData[1]);
											}
											iopData += 2;
											iopLength -= 2;
										}
									}
								}
								else
								{
									tempObject->set_raw_data(iopData, tempObject->get_number_of_bytes_in_raw_data());
									iopData += tempObject->get_number_of_bytes_in_raw_data();
									iopLength -= tempObject->get_number_of_bytes_in_raw_data();
								}

								if (iopLength >= sizeOfMacros)
								{
									for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
									{
										// If the first byte is 255, then more bytes are used! 4.6.22.3
										/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
										iopLength -= 2;
										iopData += 2;
										CANStackLogger::warn("[WS]: Skipped parsing macro reference in picture graphic object (todo)");
									}

									if ((0 != tempObject->get_actual_width()) &&
									    (0 != tempObject->get_actual_height()) &&
									    (tempObject->get_raw_data().size() == (tempObject->get_actual_width() * tempObject->get_actual_height())))
									{
										retVal = true;
										std::vector<float> image_data;
										image_data.resize(4 * (tempObject->get_actual_width() * tempObject->get_actual_height()));

										// Populate the image data from the colour table
										std::vector<std::uint8_t> &pixelData = tempObject->get_raw_data();
										CANStackLogger::debug("[WS]: Compiling texture for object " +
										                      isobus::to_string(static_cast<int>(tempObject->get_id())) +
										                      " using {} bytes of raw data" +
										                      isobus::to_string(static_cast<int>(tempObject->get_raw_data().size())));
										for (std::size_t i = 0; i < (pixelData.size()); i++)
										{
											VTColourVector colour = currentColourTable[pixelData[i]];
											image_data[(i * 4)] = colour.x;
											image_data[(i * 4) + 1] = colour.y;
											image_data[(i * 4) + 2] = colour.z;
											image_data[(i * 4) + 3] = colour.w;
										}
									}
									else
									{
										CANStackLogger::error("[WS]: Picture graphic object has invalid dimensions compared to its data. Object: " + isobus::to_string(static_cast<int>(decodedID)));
									}
								}
								else
								{
									CANStackLogger::error("[WS]: Not enough IOP data to parse macros for picture graphic object " + isobus::to_string(static_cast<int>(decodedID)));
								}
							}
							else
							{
								CANStackLogger::error("[WS]: Picture graphic format is undefined for object " + isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse picture graphic object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::NumberVariable:
					{
						auto tempObject = new NumberVariable(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_value(static_cast<std::uint32_t>(iopData[3]) |
							                      (static_cast<std::uint32_t>(iopData[4]) << 8) |
							                      (static_cast<std::uint32_t>(iopData[5]) << 16) |
							                      (static_cast<std::uint32_t>(iopData[6]) << 24));
							iopLength -= 7;
							iopData += 7;
							retVal = true;
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse number variable object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::StringVariable:
					{
						auto tempObject = new StringVariable(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);

							const std::uint16_t length = (static_cast<std::uint16_t>(iopData[3]) | (static_cast<std::uint16_t>(iopData[4]) << 8));
							iopLength -= 5;
							iopData += 5;

							if (iopLength >= length)
							{
								if ((length >= 2) && (0xFF == iopData[0]) && (0xFE == iopData[1]))
								{
									// This string is UTF-16
									CANStackLogger::error("[WS]: UTF-16 strings are not supported at this time. (todo)");
								}
								else
								{
									// Regular chars
									std::string tempStringValue;
									tempStringValue.reserve(length);

									for (std::uint32_t i = 0; i < length; i++)
									{
										tempStringValue.push_back(static_cast<char>(iopData[0]));
										iopData++;
										iopLength--;
									}
									tempObject->set_value(tempStringValue);
									retVal = true;
								}
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse string variable object raw data");
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse string variable object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::FontAttributes:
					{
						auto tempObject = new FontAttributes(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_colour(iopData[3]);
							tempObject->set_size(static_cast<FontAttributes::FontSize>(iopData[4]));

							if ((iopData[5] <= static_cast<std::uint8_t>(FontAttributes::FontType::ISO8859_7)) &&
							    (iopData[5] != static_cast<std::uint8_t>(FontAttributes::FontType::Reserved_1)) &&
							    (iopData[5] != static_cast<std::uint8_t>(FontAttributes::FontType::Reserved_2)))
							{
								tempObject->set_type(static_cast<FontAttributes::FontType>(iopData[5]));
								tempObject->set_style(iopData[6]);

								const std::uint8_t numberOfMacrosToFollow = iopData[7];
								const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);
								iopData += 8;
								iopLength -= 8;

								if (iopLength >= sizeOfMacros)
								{
									for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
									{
										// If the first byte is 255, then more bytes are used! 4.6.22.3
										/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
										iopLength -= 2;
										iopData += 2;
										CANStackLogger::warn("[WS]: Skipped parsing macro reference in font attributes object (todo)");
									}
									retVal = true;
								}
								else
								{
									CANStackLogger::error("[WS]: Not enough IOP data to parse macros for font attributes object " + isobus::to_string(static_cast<int>(decodedID)));
								}
							}
							else
							{
								CANStackLogger::error("[WS]: Proprietary and reserved fonts are not supported, and will likely never be supported.");
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse font attributes object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::LineAttributes:
					{
						auto tempObject = new LineAttributes(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_background_color(iopData[3]);
							tempObject->set_width(iopData[4]);
							tempObject->set_line_art_bit_pattern(static_cast<std::uint16_t>(iopData[5]) | (static_cast<std::uint16_t>(iopData[6]) << 8));

							const std::uint8_t numberOfMacrosToFollow = iopData[7];
							const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);
							iopData += 8;
							iopLength -= 8;

							if (iopLength >= sizeOfMacros)
							{
								for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
								{
									// If the first byte is 255, then more bytes are used! 4.6.22.3
									/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
									iopLength -= 2;
									iopData += 2;
									CANStackLogger::warn("[WS]: Skipped parsing macro reference in line attributes object (todo)");
								}
								retVal = true;
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse macros for line attributes object " + isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse line attributes object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::FillAttributes:
					{
						auto tempObject = new FillAttributes(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);

							if (iopData[3] <= static_cast<std::uint8_t>(FillAttributes::FillType::FillWithPatternGivenByFillPatternAttribute))
							{
								tempObject->set_type(static_cast<FillAttributes::FillType>(iopData[3]));
								tempObject->set_background_color(iopData[4]);
								tempObject->set_fill_pattern(static_cast<std::uint16_t>(iopData[5]) | (static_cast<std::uint16_t>(iopData[6]) << 8)); // Object ID for a picture graphic

								const std::uint8_t numberOfMacrosToFollow = iopData[7];
								const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);
								iopData += 8;
								iopLength -= 8;

								if (iopLength >= sizeOfMacros)
								{
									for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
									{
										// If the first byte is 255, then more bytes are used! 4.6.22.3
										/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
										iopLength -= 2;
										iopData += 2;
										CANStackLogger::warn("[WS]: Skipped parsing macro reference in fill attributes object (todo)");
									}
									retVal = true;
								}
								else
								{
									CANStackLogger::error("[WS]: Not enough IOP data to parse macros for fill attributes object " + isobus::to_string(static_cast<int>(decodedID)));
								}
							}
							else
							{
								CANStackLogger::error("[WS]: Fill attribute type is undefined for object " + isobus::to_string(static_cast<int>(decodedID)));
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse fill attributes object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::InputAttributes:
					{
						auto tempObject = new InputAttributes(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_validation_type(iopData[3]);
							const std::uint8_t validationStringLength = iopData[4];
							iopData += 5;
							iopLength -= 5;

							if (iopLength >= validationStringLength)
							{
								std::string tempValidationString;
								tempValidationString.reserve(validationStringLength);

								for (std::uint_fast16_t i = 0; i < validationStringLength; i++)
								{
									tempValidationString.push_back(static_cast<char>(iopData[i]));
								}
								iopData += validationStringLength;
								iopLength -= validationStringLength;

								tempObject->set_validation_string(tempValidationString);

								const std::uint8_t numberOfMacrosToFollow = iopData[0];
								const std::uint16_t sizeOfMacros = (numberOfMacrosToFollow * 2);
								iopData++;
								iopLength--;

								if (iopLength >= sizeOfMacros)
								{
									for (std::uint_fast8_t i = 0; i < numberOfMacrosToFollow; i++)
									{
										// If the first byte is 255, then more bytes are used! 4.6.22.3
										/// @todo Parse macro data, check VT version 5 for 16 bit macro IDs
										iopLength -= 2;
										iopData += 2;
										CANStackLogger::warn("[WS]: Skipped parsing macro reference in input attributes object (todo)");
									}
									retVal = true;
								}
								else
								{
									CANStackLogger::error("[WS]: Not enough IOP data to parse macros for input attributes object " + isobus::to_string(static_cast<int>(decodedID)));
								}
							}
							else
							{
								CANStackLogger::error("[WS]: Not enough IOP data to parse input attributes validation string");
							}
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse input attributes object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::ExtendedInputAttributes:
					{
						auto tempObject = new ExtendedInputAttributes(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->set_validation_type(iopData[3]);

							const std::uint8_t numberOfCodePlanesToFollow = iopData[4];
							tempObject->set_number_of_code_planes(numberOfCodePlanesToFollow);
							CANStackLogger::error("[WS]: Extended input attributes not supported yet (todo)");
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse extended input attributes object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::ColourMap:
					{
						auto tempObject = new ColourMap(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse colour map object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::ObjectLabelRefrenceList:
					{
						CANStackLogger::error("[WS]: Object label reference not supported yet (todo)");
					}
					break;

					case VirtualTerminalObjectType::ObjectPointer:
					{
						auto tempObject = new ObjectPointer(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
							tempObject->add_child((static_cast<std::uint16_t>(iopData[3]) | (static_cast<std::uint16_t>(iopData[4]) << 8)), 0, 0);
							iopLength -= 5;
							iopData += 5;
							retVal = true;
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse object pointer object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::ExternalObjectDefinition:
					{
						CANStackLogger::error("[WS]: External object definition not supported yet (todo)");
					}
					break;

					case VirtualTerminalObjectType::ExternalReferenceNAME:
					{
						CANStackLogger::error("[WS]: External reference name not supported yet (todo)");
					}
					break;

					case VirtualTerminalObjectType::ExternalObjectPointer:
					{
						CANStackLogger::error("[WS]: External object pointer not supported yet (todo)");
					}
					break;

					case VirtualTerminalObjectType::Macro:
					{
						auto tempObject = new Macro(&vtObjectTree);

						if (iopLength >= tempObject->get_minumum_object_lenth())
						{
							tempObject->set_id(decodedID);
						}
						else
						{
							CANStackLogger::error("[WS]: Not enough IOP data to parse macro object");
						}

						if (retVal)
						{
							vtObjectTree[tempObject->get_id()] = reinterpret_cast<VTObject *>(tempObject);
						}
					}
					break;

					case VirtualTerminalObjectType::AuxiliaryFunctionType1:
					case VirtualTerminalObjectType::AuxiliaryInputType1:
					case VirtualTerminalObjectType::AuxiliaryFunctionType2:
					case VirtualTerminalObjectType::AuxiliaryInputType2:
					case VirtualTerminalObjectType::AuxiliaryControlDesignatorType2:
					{
						CANStackLogger::error("[WS]: Auxiliary objects not supported yet");
					}
					break;

					default:
					{
						CANStackLogger::error("[WS]: Unsupported Object");
					}
					break;
				}
			}

			if (!retVal)
			{
				set_object_pool_faulting_object_id(decodedID);
			}
		}
		return retVal;
	}

	VTObject *VirtualTerminalServerManagedWorkingSet::get_object_by_id(std::uint16_t objectID)
	{
		return vtObjectTree[objectID];
	}

	VTObject *VirtualTerminalServerManagedWorkingSet::get_working_set_object()
	{
		return get_object_by_id(workingSetID);
	}

	bool VirtualTerminalServerManagedWorkingSet::get_object_id_exists(std::uint16_t objectID) const
	{
		bool retVal;

		if (vtObjectTree.find(objectID) == vtObjectTree.end())
		{
			retVal = false;
		}
		else
		{
			retVal = true;
		}
		return retVal;
	}

	void VirtualTerminalServerManagedWorkingSet::set_object_pool_processing_state(ObjectPoolProcessingThreadState value)
	{
		const std::lock_guard<std::mutex> lock(manangedWorkingSetMutex);
		processingState = value;
	}

	void VirtualTerminalServerManagedWorkingSet::set_object_pool_faulting_object_id(std::uint16_t value)
	{
		const std::lock_guard<std::mutex> lock(manangedWorkingSetMutex);
		faultingObjectID = value;
	}

	void VirtualTerminalServerManagedWorkingSet::initialize_colour_table()
	{
		// The table can be altered at runtime. Init here to VT standard
		currentColourTable[0] = VTColourVector(0.0f, 0.0f, 0.0f, 1.0f); // Black
		currentColourTable[1] = VTColourVector(1.0f, 1.0f, 1.0f, 1.0f); // White
		currentColourTable[2] = VTColourVector(0.0f, (153.0f / 255.0f), 0.0f, 1.0f); // Green
		currentColourTable[3] = VTColourVector(0.0f, (153.0f / 255.0f), (153.0f / 255.0f), 1.0f); // Teal
		currentColourTable[4] = VTColourVector((153.0f / 255.0f), 0.0f, 0.0f, 1.0f); // Maroon
		currentColourTable[5] = VTColourVector((153.0f / 255.0f), 0.0f, (153.0f / 255.0f), 1.0f); // Purple
		currentColourTable[6] = VTColourVector((153.0f / 255.0f), (153.0f / 255.0f), 0.0f, 1.0f); // Olive
		currentColourTable[7] = VTColourVector((204.0f / 255.0f), (204.0f / 255.0f), (204.0f / 255.0f), 1.0f); // Silver
		currentColourTable[8] = VTColourVector((153.0f / 255.0f), (153.0f / 255.0f), (153.0f / 255.0f), 1.0f); // Grey
		currentColourTable[9] = VTColourVector(0.0f, 0.0f, 1.0f, 1.0f); // Blue
		currentColourTable[10] = VTColourVector(0.0f, 1.0f, 0.0f, 1.0f); // Lime
		currentColourTable[11] = VTColourVector(0.0f, 1.0f, 1.0f, 1.0f); // Cyan
		currentColourTable[12] = VTColourVector(1.0f, 0.0f, 0.0f, 1.0f); // Red
		currentColourTable[13] = VTColourVector(1.0f, 0.0f, 1.0f, 1.0f); // Magenta
		currentColourTable[14] = VTColourVector(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
		currentColourTable[15] = VTColourVector(0.0f, 0.0f, (153.0f / 255.0f), 1.0f); // Navy

		// This section of the table increases with a pattern
		for (std::uint8_t i = 16; i <= 231; i++)
		{
			std::uint8_t index = i - 16;

			std::uint32_t redCounter = (index / 36);
			std::uint32_t greenCounter = ((index / 6) % 6);
			std::uint32_t blueCounter = (index % 6);

			currentColourTable[i] = VTColourVector((51.0f * (redCounter) / 255.0f), ((51.0f * (greenCounter)) / 255.0f), ((51.0f * blueCounter) / 255.0f), 1.0f);
		}

		// The rest are proprietary. Init to white for now.
		for (std::uint16_t i = 232; i < VT_COLOUR_TABLE_SIZE; i++)
		{
			currentColourTable[i] = VTColourVector(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

	void VirtualTerminalServerManagedWorkingSet::worker_thread_function()
	{
		if (0 != iopFilesRawData.size())
		{
			bool lSuccess = true;

			set_object_pool_processing_state(ObjectPoolProcessingThreadState::Running);
			CANStackLogger::info("[WS]: Beginning parsing of object pool. This pool has " +
			                     isobus::to_string(static_cast<int>(iopFilesRawData.size())) +
			                     " IOP components.");
			for (std::size_t i = 0; i < iopFilesRawData.size(); i++)
			{
				if (!parse_iop_into_objects(iopFilesRawData[i].data(), iopFilesRawData[i].size()))
				{
					lSuccess = false;
					break;
				}
			}

			if (lSuccess)
			{
				CANStackLogger::info("[WS]: Object pool sucessfully parsed.");
				set_object_pool_processing_state(ObjectPoolProcessingThreadState::Success);
			}
			else
			{
				CANStackLogger::error("[WS]: Object pool failed to be parsed.");
				set_object_pool_processing_state(ObjectPoolProcessingThreadState::Fail);
			}
		}
		else
		{
			CANStackLogger::error("[WS]: Object pool failed to be parsed.");
			set_object_pool_processing_state(ObjectPoolProcessingThreadState::Fail);
		}
	}

} // namespace isobus