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

#include <cstring>
#include <utility>
#include <vector>

namespace isobus
{
	/// @brief Parses an object pool into an object tree of its own
	class VirtualTerminalServerManagedWorkingSet::ObjectPoolParser : public VirtualTerminalWorkingSetBase
	{
	public:
		/// @brief Starts from a copy of an existing object tree, so that a later pool transfer can replace and add objects
		/// @param[in] objectTree The object tree to start from
		/// @param[in] existingWorkingSetID The object ID of the working set object in that tree
		ObjectPoolParser(const std::map<std::uint16_t, std::shared_ptr<VTObject>> &objectTree, std::uint16_t existingWorkingSetID)
		{
			vtObjectTree = objectTree;
			workingSetID = existingWorkingSetID;
		}

		/// @brief Moves the parsed object tree out of the parser
		/// @returns The parsed object tree
		std::map<std::uint16_t, std::shared_ptr<VTObject>> take_object_tree()
		{
			return std::move(vtObjectTree);
		}

		/// @brief Returns the object ID of the working set object in the parsed object tree
		/// @returns The object ID of the working set object, or NULL_OBJECT_ID
		std::uint16_t get_working_set_id() const
		{
			return workingSetID;
		}
	};

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
		join_parsing_thread();
	}

	void VirtualTerminalServerManagedWorkingSet::start_parsing_thread()
	{
		if (nullptr == objectPoolProcessingThread)
		{
			objectPoolParser.reset(new ObjectPoolParser(vtObjectTree, workingSetID));
			set_object_pool_processing_state(ObjectPoolProcessingThreadState::Running);
			objectPoolProcessingThread.reset(new std::thread([this]() { worker_thread_function(); }));
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
				vtObjectTree = objectPoolParser->take_object_tree();
				workingSetID = objectPoolParser->get_working_set_id();
			}
			objectPoolParser.reset();
			set_object_pool_processing_state(ObjectPoolProcessingThreadState::Joined);
		}
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
		focusedObject = NULL_OBJECT_ID;
		set_object_pool_faulting_object_id(NULL_OBJECT_ID);
		set_object_pool_processing_state(ObjectPoolProcessingThreadState::None);
	}

	void VirtualTerminalServerManagedWorkingSet::set_object_focus(std::uint16_t objectID)
	{
		focusedObject = objectID;
	}

	std::uint16_t VirtualTerminalServerManagedWorkingSet::get_object_focus() const
	{
		return focusedObject;
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
			// The length of the objects in each transfer parsed by this run, without the padding after the last one
			std::vector<std::pair<std::size_t, std::uint32_t>> parsedTransfers;

			for (std::size_t i = 0; i < iopFilesRawData.size(); i++)
			{
				if (iopFilesRawData[i].processed)
				{
					continue;
				}

				std::uint32_t objectsLength = 0;
				if (!objectPoolParser->parse_iop_into_objects(iopFilesRawData[i].data.data(), static_cast<std::uint32_t>(iopFilesRawData[i].data.size()), &objectsLength))
				{
					set_object_pool_fault(objectPoolParser->get_object_pool_faulting_object_id(), objectPoolParser->get_object_pool_faulting_parent_object_id());
					lSuccess = false;
					break;
				}
				parsedTransfers.emplace_back(i, objectsLength);
			}

			if (lSuccess)
			{
				// Only now, as after a failure none of the objects of this run are taken over
				for (const auto &transfer : parsedTransfers)
				{
					// Load Version parses the stored transfers joined together, where this padding would read as objects
					iopFilesRawData[transfer.first].data.resize(transfer.second);
					iopFilesRawData[transfer.first].processed = true;
				}
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
