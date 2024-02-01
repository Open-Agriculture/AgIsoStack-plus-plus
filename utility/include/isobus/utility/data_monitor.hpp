//================================================================================================
/// @file data_monitor.hpp
///
/// @brief Provides the monitor design pattern to allow for thread-safe synchronization of data
/// @author Daan Steenbergen
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#ifndef DATA_MONITOR_HPP
#define DATA_MONITOR_HPP

#if defined CAN_STACK_DISABLE_THREADS || defined ARDUINO
/// @brief Disabled READ_GUARD macro since threads are disabled.
#define READ_GUARD(type, x)
/// @brief Disabled WRITE_GUARD macro since threads are disabled.
#define WRITE_GUARD(type, x)
namespace isobus
{
	/// @brief A dummy class to allow for the use of the monitor design pattern without threads.
	class ConcurrentReadingMonitor
	{
	};
}
#else
#include <condition_variable>
#include <cstdint>
#include <mutex>

#define READ_GUARD(type, x) ReadGuard<type> x##Guard(x)
#define WRITE_GUARD(type, x) WriteGuard<type> x##Guard(x)

namespace isobus
{
	/// @brief A RAII-style mechanism to read data from a monitor in a scoped block.
	/// @tparam T The monitor type (requires a readEntry() and readExit() function).
	template<typename T>
	class ReadGuard
	{
	public:
		/// @brief Constructs a read guard for the given monitor.
		/// @param monitor The monitor to read from.
		explicit ReadGuard(T &monitor) :
		  monitor(monitor)
		{
			monitor.readEntry();
		}

		/// @brief Destructs the read guard.
		~ReadGuard()
		{
			monitor.readExit();
		}

		ReadGuard(const ReadGuard &) = delete;
		ReadGuard &operator=(const ReadGuard &) = delete;
		ReadGuard(ReadGuard &&) = delete;
		ReadGuard &operator=(ReadGuard &&) = delete;

	private:
		T &monitor; ///< The monitor to read from.
	};

	/// @brief A RAII-style mechanism to write data to a monitor in a scoped block.
	/// @tparam T The monitor type (requires a writeEntry() and writeExit() function).
	template<typename T>
	class WriteGuard
	{
	public:
		/// @brief Constructs a write guard for the given monitor.
		/// @param monitor The monitor to write to.
		explicit WriteGuard(T &monitor) :
		  monitor(monitor)
		{
			monitor.writeEntry();
		}

		/// @brief Destructs the write guard.
		~WriteGuard()
		{
			monitor.writeExit();
		}

		WriteGuard(const WriteGuard &) = delete;
		WriteGuard &operator=(const WriteGuard &) = delete;
		WriteGuard(WriteGuard &&) = delete;
		WriteGuard &operator=(WriteGuard &&) = delete;

	private:
		T &monitor; ///< The monitor to write to.
	};

	/// @brief A monitor to allow for multiple concurrent readers and a single writer.
	/// Only used when support for threads is enabled.
	/// @note Only the writer is allowed to make changes to the underlying data.
	class ConcurrentReadingMonitor
	{
	public:
		/// @brief Enters a write block.
		void writeEntry();

		/// @brief Exits a write block.
		void writeExit();

		/// @brief Enters a read block.
		void readEntry();

		/// @brief Exits a read block.
		void readExit();

	private:
		std::size_t numberOfReaders = 0; ///< The number of readers.
		bool hasWriter = false; ///< Whether there is a writer.

		std::condition_variable readCondition; ///< The condition variable for readers.
		std::condition_variable writeCondition; ///< The condition variable for the writer.
		std::mutex mutex; /// The mutex to use with the condition variables.
	};
}
#endif

#endif // DATA_SPAN_HPP
