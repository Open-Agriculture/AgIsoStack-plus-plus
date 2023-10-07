//================================================================================================
/// @file isobus_virtual_terminal_server_managed_working_set.hpp
///
/// @brief Defines a managed working set for a VT server.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef ISOBUS_VIRTUAL_TERMINAL_MANAGED_WORKING_SET_HPP
#define ISOBUS_VIRTUAL_TERMINAL_MANAGED_WORKING_SET_HPP

#include <array>
#include <map>
#include <mutex>
#include <thread>

#include "isobus/isobus/can_control_function.hpp"
#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"

namespace isobus
{
	/// @brief Defines a managed working set.
	/// @details This class is meant to be used as the basis for a VT server.
	/// It keeps track of one active object pool.
	class VirtualTerminalServerManagedWorkingSet
	{
	public:
		/// @brief Enumerates the states of the processing thread for the object pool
		enum class ObjectPoolProcessingThreadState
		{
			None, ///< Thread has never been started for this working set
			Running, ///< We are currently parsing the object pool
			Success, ///< We have finished parsing the pool successfully and need to respond to the working set
			Fail, ///< The object pool is bad and we need to respond to the working set
			Joined ///< We have sent our response to the working set master and are done parsing
		};

		VirtualTerminalServerManagedWorkingSet();
		VirtualTerminalServerManagedWorkingSet(std::shared_ptr<ControlFunction> associatedControlFunction);
		~VirtualTerminalServerManagedWorkingSet();

		/// @brief Starts a thread to parse the received object pool files
		void start_parsing_thread();

		/// @brief Joins the parsing thread
		void join_parsing_thread();

		bool parse_iop_into_objects(std::uint8_t *iopData, std::uint32_t iopLength);

		/// @brief Returns if any object pools are being managed for this working set master
		/// @returns true if at least 1 object pool has been received for this working set master, otherwise false
		bool get_any_object_pools() const;

		std::shared_ptr<VTObject> get_object_by_id(std::uint16_t objectID);
		std::shared_ptr<VTObject> get_working_set_object();

		ObjectPoolProcessingThreadState get_object_pool_processing_state();
		std::uint16_t get_object_pool_faulting_object_id();

		void add_iop_raw_data(std::vector<std::uint8_t> &dataToAdd);
		std::size_t get_number_iop_files() const;
		std::vector<std::uint8_t> &get_iop_raw_data(std::size_t index);

		/// @brief Returns the control function that is the working set master
		/// @returns The control function that is the working set master
		std::shared_ptr<ControlFunction> get_control_function() const;

		/// @brief Returns the working set maintenance message timestamp
		/// @returns The working set maintenance message timestamp in milliseconds
		std::uint32_t get_working_set_maintenance_message_timestamp_ms() const;

		/// @brief Sets the timestamp for when we sent the maintenance message timestamp
		/// @param[in] value New timestamp value in milliseconds
		void set_working_set_maintenance_message_timestamp_ms(std::uint32_t value);

		/// @brief Saves an event callback handle for the lifetime of this object
		/// which is useful for keeping track of callback lifetimes in a VT server
		void save_callback_handle(std::shared_ptr<void> callbackHandle);

		/// @brief Returns a colour from this working set's current colour table, by index
		/// @param[in] colourIndex The index into the VT's colour table to retrieve
		/// @returns A colour from this working set's current colour table, by index
		VTColourVector get_colour(std::uint8_t colourIndex) const;

	private:
		/// @brief Parses one object in the remaining object pool data
		/// @param[in,out] iopData A pointer to some object pool data
		/// @param[in,out] iopLength The number of bytes remaining in the object pool
		/// @returns true if an object was parsed
		bool parse_next_object(std::uint8_t *&iopData, std::uint32_t &iopLength);

		/// @brief Checks if the object pool contains an object with the supplied object ID
		/// @returns true if an object with the specified ID exists in the object pool
		bool get_object_id_exists(std::uint16_t objectID) const;

		/// @brief Sets the object pool processing state to a new value
		/// @param[in] value The new state of processing the object pool
		void set_object_pool_processing_state(ObjectPoolProcessingThreadState value);

		/// @brief Sets the object ID associated with a faulting object during pool parsing
		/// @param[in] value The object ID to set as the faulting object
		void set_object_pool_faulting_object_id(std::uint16_t value);

		/// @brief The object pool processing thread will execute this function when it runs
		void worker_thread_function();

		VTColourTable workingSetColourTable; ///< This working set's colour table
		std::map<std::uint16_t, std::shared_ptr<VTObject>> vtObjectTree; ///< The C++ object representation (deserialized) of the object pool being managed
		std::vector<std::vector<std::uint8_t>> iopFilesRawData; ///< Raw IOP File data from the client
		std::unique_ptr<std::thread> objectPoolProcessingThread; ///< A thread to process the object pool with, since that can be fairly time consuming.
		std::mutex managedWorkingSetMutex; ///< A mutex to protect the interface of the managed working set
		std::shared_ptr<ControlFunction> workingSetControlFunction; ///< Stores the control function associated with this working set
		std::vector<std::shared_ptr<void>> callbackHandles; ///< A convenient way to associate callback handles to a working set
		ObjectPoolProcessingThreadState processingState; ///< Stores the state of processing the object pool
		std::uint32_t workingSetMaintenanceMessageTimestamp_ms; ///< A timestamp (in ms) to track sending of the maintenance message
		std::uint16_t workingSetID; ///< Stores the object ID of the working set object itself
		std::uint16_t faultingObjectID; ///< Stores the faulting object ID to send to a client when parsing the pool fails
	};
} // namespace isobus

#endif // ISOBUS_VIRTUAL_TERMINAL_MANAGED_WORKING_SET_HPP
