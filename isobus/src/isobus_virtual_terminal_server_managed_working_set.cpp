//================================================================================================
/// @file isobus_virtual_terminal_server_managed_working_set.cpp
///
/// @brief Defines a class that manages a VT server's active working sets.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#if !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/to_string.hpp"

#include <algorithm>
#include <cstring>
#include <map>
#include <utility>
#include <vector>

namespace isobus
{
	/// @brief Parses an object pool into an object tree of its own, which holds only the objects of this run
	class VirtualTerminalServerManagedWorkingSet::ObjectPoolParser : public VirtualTerminalWorkingSetBase
	{
	public:
		/// @brief Constructs the parser
		/// @param[in] existingObjects The objects of the current object tree that parsing needs to check against,
		/// which are the working set object and any Working Set Special Controls object
		/// @param[in] existingWorkingSetID The object ID of the working set object in the current object tree
		/// @param[in] existingObjectTypes The type of each object in the current object tree
		ObjectPoolParser(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &existingObjects,
		                 std::uint16_t existingWorkingSetID,
		                 std::map<std::uint16_t, VirtualTerminalObjectType> existingObjectTypes) :
		  existingObjects(existingObjects),
		  existingObjectTypes(std::move(existingObjectTypes))
		{
			vtObjectTree = existingObjects;
			workingSetID = existingWorkingSetID;
		}

		/// @brief Checks if an object is one that parsing replaced or added, not one copied from the current object tree
		/// @param[in] objectID The object ID to check
		/// @param[in] object The object in the parser's object tree
		/// @returns true if parsing replaced or added the object
		bool is_parsed_object(std::uint16_t objectID, const std::shared_ptr<VTObject> &object) const
		{
			const auto existing = existingObjects.find(objectID);
			return (existingObjects.end() == existing) || (existing->second != object);
		}

		/// @brief Checks that no parsed object replaces an object of another type in the current object tree
		/// @details A replaced object shall be of the same type (ISO 11783-6:2018 4.6.10.3). The parser's own object tree
		/// only holds the objects of this run, so add_or_replace_object() can't check this against the current pool.
		/// The lowest such object ID is set as the faulting object.
		/// @returns true if no parsed object replaces an object of another type
		bool check_replaced_object_types()
		{
			for (const auto &object : get_object_tree())
			{
				const auto existing = existingObjectTypes.find(object.first);
				if ((nullptr != object.second) &&
				    (existingObjectTypes.end() != existing) &&
				    (existing->second != object.second->get_object_type()))
				{
					LOG_ERROR("[WS]: Object %u can't replace the object of another type with the same ID", object.first);
					set_object_pool_faulting_object_id(object.first);
					return false;
				}
			}
			return true;
		}

		/// @brief Returns the object ID of the working set object, after parsing
		/// @returns The object ID of the working set object, or NULL_OBJECT_ID
		std::uint16_t get_working_set_id() const
		{
			return workingSetID;
		}

		const std::map<std::uint16_t, std::shared_ptr<VTObject>> existingObjects; ///< The objects copied from the current object tree
		const std::map<std::uint16_t, VirtualTerminalObjectType> existingObjectTypes; ///< The type of each object in the current object tree
		std::vector<std::pair<std::size_t, std::uint32_t>> parsedTransfers; ///< The index of each transfer parsed by this run, and the length of its objects without the padding after the last one
	};

	/// @brief Finds the lowest object ID among the objects that list an object as a child
	/// @param[in] objectTree The objects to search
	/// @param[in] childID The object ID of the child
	/// @param[in] skip Objects whose ID is in this tree are not searched, or nullptr to search all
	/// @returns The object ID of the parent, or NULL_OBJECT_ID
	static std::uint16_t find_parent_object_id(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectTree,
	                                           std::uint16_t childID,
	                                           const std::map<std::uint16_t, std::shared_ptr<VTObject>> *skip)
	{
		for (const auto &object : objectTree)
		{
			if ((nullptr == object.second) || ((nullptr != skip) && (skip->end() != skip->find(object.first))))
			{
				continue;
			}

			for (std::uint16_t i = 0; i < object.second->get_number_children(); i++)
			{
				if (childID == object.second->get_child_id(i))
				{
					return object.first;
				}
			}
		}
		return NULL_OBJECT_ID;
	}

	VirtualTerminalServerManagedWorkingSet::VirtualTerminalServerManagedWorkingSet()
	{
		LOG_INFO("[WS]: New VT Server Object Created with no associated control function");
	}

	VirtualTerminalServerManagedWorkingSet::VirtualTerminalServerManagedWorkingSet(std::shared_ptr<ControlFunction> associatedControlFunction) :
	  workingSetControlFunction(associatedControlFunction)
	{
		if (nullptr != associatedControlFunction)
		{
			LOG_INFO("[WS]: New VT Server Object Created for CF " + to_string(static_cast<int>(associatedControlFunction->get_NAME().get_full_name())));
		}
		else
		{
			LOG_INFO("[WS]: New VT Server Object Created with no associated control function");
		}
	}

	VirtualTerminalServerManagedWorkingSet::~VirtualTerminalServerManagedWorkingSet()
	{
		try
		{
			join_parsing_thread();
		}
		catch (...)
		{
			// A destructor must not throw, and joining can fail with a system error
			LOG_ERROR("[WS]: The object pool parsing thread could not be joined.");
		}
	}

	void VirtualTerminalServerManagedWorkingSet::start_parsing_thread(const std::function<void()> &onParsingEnded)
	{
		if (nullptr == objectPoolProcessingThread)
		{
			// Parsing checks a later Working Set object against the current one, and allows only one Working Set Special Controls object
			// A parsed object must not replace an object of another type, so the parser gets the type of every current object
			std::map<std::uint16_t, std::shared_ptr<VTObject>> existingObjects;
			std::map<std::uint16_t, VirtualTerminalObjectType> existingObjectTypes;
			for (const auto &object : vtObjectTree)
			{
				if (nullptr != object.second)
				{
					existingObjectTypes.emplace_hint(existingObjectTypes.end(), object.first, object.second->get_object_type());
					if ((workingSetID == object.first) || (VirtualTerminalObjectType::WorkingSetSpecialControls == object.second->get_object_type()))
					{
						existingObjects.insert(object);
					}
				}
			}

			objectPoolParser.reset(new ObjectPoolParser(existingObjects, workingSetID, std::move(existingObjectTypes)));
			set_object_pool_processing_state(ObjectPoolProcessingThreadState::Running);
			objectPoolProcessingThread.reset(new std::thread([this, onParsingEnded]() {
				worker_thread_function();
				if (nullptr != onParsingEnded)
				{
					onParsingEnded();
				}
			}));
		}
	}

	void VirtualTerminalServerManagedWorkingSet::join_parsing_thread()
	{
		if ((nullptr != objectPoolProcessingThread) && (objectPoolProcessingThread->joinable()))
		{
			objectPoolProcessingThread->join();
			objectPoolProcessingThread = nullptr;

			if ((nullptr != objectPoolParser) && (ObjectPoolProcessingThreadState::Success == get_object_pool_processing_state()))
			{
				take_over_parsed_objects();
			}
			else if (nullptr != objectPoolParser)
			{
				set_fault_from_parser();
			}
			objectPoolParser.reset();
			set_object_pool_processing_state(ObjectPoolProcessingThreadState::Joined);
		}
	}

	void VirtualTerminalServerManagedWorkingSet::take_over_parsed_objects()
	{
		// Merged into the current object tree, so objects added to it while the thread ran stay.
		// The parsed objects are in ID order, so replacedObjects stays sorted.
		replacedObjects.clear();
		for (const auto &object : objectPoolParser->get_object_tree())
		{
			if (objectPoolParser->is_parsed_object(object.first, object.second))
			{
				if (vtObjectTree.end() != vtObjectTree.find(object.first))
				{
					replacedObjects.push_back(object.first);
				}
				vtObjectTree[object.first] = object.second;
			}
		}
		workingSetID = objectPoolParser->get_working_set_id();

		for (const auto &transfer : objectPoolParser->parsedTransfers)
		{
			// Load Version parses the stored transfers joined together, where this padding would read as objects
			iopFilesRawData[transfer.first].data.resize(transfer.second);
			iopFilesRawData[transfer.first].processed = true;
		}
	}

	void VirtualTerminalServerManagedWorkingSet::set_fault_from_parser()
	{
		// A failed parse leaves the object tree as it was, so it replaced nothing
		replacedObjects.clear();

		// The parent can be a new object or one of the current pool, and a new object replaces a current one with the same ID
		const auto &parsedObjects = objectPoolParser->get_object_tree();
		const std::uint16_t faultingID = objectPoolParser->get_object_pool_faulting_object_id();
		std::uint16_t parentID = NULL_OBJECT_ID;

		if (NULL_OBJECT_ID != faultingID)
		{
			parentID = std::min(find_parent_object_id(parsedObjects, faultingID, nullptr),
			                    find_parent_object_id(vtObjectTree, faultingID, &parsedObjects));
		}
		set_object_pool_fault(faultingID, parentID);
	}

	bool VirtualTerminalServerManagedWorkingSet::get_any_object_pools() const
	{
		return (!iopFilesRawData.empty());
	}

	VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState VirtualTerminalServerManagedWorkingSet::get_object_pool_processing_state()
	{
		const std::lock_guard<std::mutex> lock(managedWorkingSetMutex);
		return processingState;
	}

	std::shared_ptr<ControlFunction> VirtualTerminalServerManagedWorkingSet::get_control_function() const
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

	std::uint8_t VirtualTerminalServerManagedWorkingSet::get_working_set_version() const
	{
		return workingSetVersion;
	}

	void VirtualTerminalServerManagedWorkingSet::set_working_set_version(std::uint8_t value)
	{
		workingSetVersion = value;
	}

	void VirtualTerminalServerManagedWorkingSet::save_callback_handle(isobus::EventCallbackHandle callbackHandle)
	{
		callbackHandles.push_back(callbackHandle);
	}

	void VirtualTerminalServerManagedWorkingSet::clear_callback_handles()
	{
		callbackHandles.clear();
	}

	bool VirtualTerminalServerManagedWorkingSet::get_was_object_pool_loaded_from_non_volatile_memory() const
	{
		return wasLoadedFromNonVolatileMemory;
	}

	bool VirtualTerminalServerManagedWorkingSet::get_was_object_pool_loaded_with_extended_version_label() const
	{
		return wasLoadedWithExtendedVersionLabel;
	}

	void VirtualTerminalServerManagedWorkingSet::set_was_object_pool_loaded_from_non_volatile_memory(bool value, bool extendedVersionLabel, CANLibBadge<VirtualTerminalServer>)
	{
		wasLoadedFromNonVolatileMemory = value;
		wasLoadedWithExtendedVersionLabel = extendedVersionLabel;
	}

	void VirtualTerminalServerManagedWorkingSet::clear_object_pool_data(CANLibBadge<VirtualTerminalServer>)
	{
		vtObjectTree.clear();
		iopFilesRawData.clear();
		transferredIopSize = 0;
		workingSetID = NULL_OBJECT_ID;
		focusedObject.store(NULL_OBJECT_ID);
		openForInputObject.store(NULL_OBJECT_ID);
		set_object_pool_faulting_object_id(NULL_OBJECT_ID);
		set_object_pool_processing_state(ObjectPoolProcessingThreadState::None);
		objectPoolTransferDropped = false;
	}

	void VirtualTerminalServerManagedWorkingSet::set_object_pool_transfer_dropped()
	{
		objectPoolTransferDropped = true;
	}

	bool VirtualTerminalServerManagedWorkingSet::get_object_pool_transfer_dropped() const
	{
		return objectPoolTransferDropped;
	}

	void VirtualTerminalServerManagedWorkingSet::set_object_focus(std::uint16_t objectID)
	{
		focusedObject.store(objectID);
	}

	std::uint16_t VirtualTerminalServerManagedWorkingSet::get_object_focus() const
	{
		return focusedObject.load();
	}

	void VirtualTerminalServerManagedWorkingSet::set_object_open_for_input(std::uint16_t objectID)
	{
		openForInputObject.store(objectID);
	}

	std::uint16_t VirtualTerminalServerManagedWorkingSet::get_object_open_for_input() const
	{
		return openForInputObject.load();
	}

	bool VirtualTerminalServerManagedWorkingSet::was_object_replaced(std::uint16_t objectID) const
	{
		return std::binary_search(replacedObjects.begin(), replacedObjects.end(), objectID);
	}

	void VirtualTerminalServerManagedWorkingSet::set_auxiliary_input_maintenance_timestamp_ms(std::uint32_t value)
	{
		auxiliaryInputMaintenanceMessageTimestamp_ms = value;
	}

	std::uint32_t VirtualTerminalServerManagedWorkingSet::get_auxiliary_input_maintenance_timestamp_ms() const
	{
		return auxiliaryInputMaintenanceMessageTimestamp_ms;
	}

	void VirtualTerminalServerManagedWorkingSet::request_deletion()
	{
		workingSetDeletionRequested = true;
	}

	bool VirtualTerminalServerManagedWorkingSet::is_deletion_requested() const
	{
		return workingSetDeletionRequested;
	}

	void VirtualTerminalServerManagedWorkingSet::set_iop_size(std::uint32_t newIopSize)
	{
		const std::lock_guard<std::mutex> lock(managedWorkingSetMutex);
		iopSize = newIopSize;
	}

	float VirtualTerminalServerManagedWorkingSet::iop_load_percentage() const
	{
		if (processingState != ObjectPoolProcessingThreadState::None || transferredIopSize > iopSize)
		{
			return 100.0f;
		}

		if (0 == iopSize)
		{
			return 0.0f;
		}

		// if IOP transfer is not completed check if there is an ongoing IOP transfer to us
		auto sessions = CANNetworkManager::CANNetwork.get_active_transport_protocol_sessions(0);
		auto currentTransferredIopSize = transferredIopSize;
		for (const auto &session : sessions)
		{
			if (session->get_source()->get_address() == get_control_function()->get_address() &&
			    (static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal) == session->get_parameter_group_number()) &&
			    (session->get_data().size() >= 1) &&
			    (session->get_data().get_byte(0) == 0x11)) // ObjectPoolTransferMessage
			{
				currentTransferredIopSize += session->get_total_bytes_transferred();
			}
		}

		if (currentTransferredIopSize > iopSize)
		{
			return 100.0f;
		}
		return (static_cast<float>(currentTransferredIopSize) / static_cast<float>(iopSize)) * 100.0f;
	}

	void VirtualTerminalServerManagedWorkingSet::set_object_pool_processing_state(ObjectPoolProcessingThreadState value)
	{
		const std::lock_guard<std::mutex> lock(managedWorkingSetMutex);
		processingState = value;
	}

	void VirtualTerminalServerManagedWorkingSet::worker_thread_function()
	{
		if (!iopFilesRawData.empty())
		{
			bool lSuccess = true;

			LOG_INFO("[WS]: Beginning parsing of object pool. This pool has " +
			         isobus::to_string(static_cast<int>(iopFilesRawData.size())) +
			         " IOP components.");
			for (std::size_t i = 0; i < iopFilesRawData.size(); i++)
			{
				if (iopFilesRawData[i].processed)
				{
					continue;
				}

				std::uint32_t objectsLength = 0;
				if (!objectPoolParser->parse_iop_into_objects(iopFilesRawData[i].data.data(), static_cast<std::uint32_t>(iopFilesRawData[i].data.size()), &objectsLength))
				{
					lSuccess = false;
					break;
				}
				// Applied in join_parsing_thread(), as the server can read the transfers while this thread runs
				objectPoolParser->parsedTransfers.emplace_back(i, objectsLength);
			}

			if (lSuccess)
			{
				lSuccess = objectPoolParser->check_replaced_object_types();
			}

			if (lSuccess)
			{
				LOG_INFO("[WS]: Object pool successfully parsed.");
				set_object_pool_processing_state(ObjectPoolProcessingThreadState::Success);
			}
			else
			{
				LOG_ERROR("[WS]: Object pool failed to be parsed.");
				set_object_pool_processing_state(ObjectPoolProcessingThreadState::Fail);
			}
		}
		else
		{
			LOG_ERROR("[WS]: Object pool failed to be parsed.");
			set_object_pool_processing_state(ObjectPoolProcessingThreadState::Fail);
		}
	}

	bool VirtualTerminalServerManagedWorkingSet::is_object_pool_transfer_in_progress() const
	{
		return iop_load_percentage() != 0.0f;
	}

} // namespace isobus
#endif // !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
