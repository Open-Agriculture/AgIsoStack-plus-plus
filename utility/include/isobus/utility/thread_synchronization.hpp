//================================================================================================
/// @file thread_synchronization.hpp
///
/// @brief A single header file to automatically include the correct thread synchronization
/// @author Daan Steenbergen
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#ifndef THREAD_SYNCHRONIZATION_HPP
#define THREAD_SYNCHRONIZATION_HPP

#if defined CAN_STACK_DISABLE_THREADS || defined ARDUINO

namespace isobus
{
	/// @brief A dummy mutex class when treading is disabled.
	class Mutex
	{
	};
	/// @brief A dummy recursive mutex class when treading is disabled.
	class RecursiveMutex
	{
	};
}
/// @brief Disabled LOCK_GUARD macro since threads are disabled.
#define LOCK_GUARD(type, x)

#elif defined USE_CMSIS_RTOS2_THREADING

#include "cmsis_os.h"

namespace isobus
{
	/// @brief A wrapper around a CMSIS RTOS 2 mutex.
	/// @details See definition at https://www.keil.com/pack/doc/CMSIS/RTOS2/html/group__CMSIS__RTOS.html
	class Mutex
	{
	public:
		/// @brief Constructor for the CMSIS RTOS2 mutex wrapper
		Mutex() :
		  handle(nullptr)
		{
		}

		/// @brief Locks the mutex. Part of BasicLockable requirements.
		void lock()
		{
			if (ready())
			{
				osStatus_t osRetVal = osMutexAcquire(handle, osWaitForever);

				if (osOK != osRetVal)
				{
					while (true)
					{
						// If your code is stuck in here, that means you did something
						// very wrong, like recursively locked this mutex, or tried to
						// lock the mutex before the OS was initialized, or called this in
						// an interrupt service routine.
						// osRetVal may contain more information.
					}
				}
			}
		}

		/// @brief Attempts to the mutex, and doesn't wait if it's not available.
		/// @returns true if the mutex was successfully locked, false otherwise.
		bool try_lock()
		{
			bool retVal = false;

			if (ready())
			{
				osStatus_t osRetVal = osMutexAcquire(handle, 0);
				retVal = (osOK == osRetVal);
			}
			return retVal;
		}

		/// @brief Unlocks the mutex. Part of BasicLockable requirements.
		void unlock()
		{
			if (nullptr != handle)
			{
				osStatus_t osRetVal = osMutexRelease(handle);

				if (osOK != osRetVal)
				{
					while (true)
					{
						// If your code is stuck in here, that means you
						// either tried to release a mutex which is owned by a different thread,
						// or the release failed due to some other OS reason.
						// osRetVal may contain more information.
					}
				}
			}
			else
			{
				while (true)
				{
					// If your code is stuck in here, it's because you tried to unlock a
					// mutex which doesn't exist. Don't do that.
					// osRetVal may contain more information.
				}
			}
		}

	protected:
		/// @brief Checks if the mutex is ready to be used. Initializes the mutex if it's not.
		/// @returns true if the mutex is ready to be used, false otherwise.
		virtual bool ready()
		{
			if (nullptr == handle)
			{
				const osMutexAttr_t attributes = {
					nullptr,
#ifdef osCMSIS_FreeRTOS // FreeRTOS doesn't support robust mutexes
					osMutexPrioInherit,
#else
					osMutexPrioInherit | osMutexRobust,
#endif
					nullptr,
					0
				};
				handle = osMutexNew(&attributes);
			}
			return nullptr != handle;
		}
		osMutexId_t handle; ///< Mutex ID for reference by other functions or NULL in case of error or not yet initialized
	};

	/// @brief A wrapper around a CMSIS RTOS 2 recursive mutex.
	/// @details See definition at https://www.keil.com/pack/doc/CMSIS/RTOS2/html/group__CMSIS__RTOS.html
	class RecursiveMutex : public Mutex
	{
	protected:
		/// @brief Checks if the mutex is ready to be used.
		/// Initializes the mutex if it's not.
		/// @returns true if the mutex is ready to be used, false otherwise.
		bool ready() override
		{
			if (nullptr == handle)
			{
				const osMutexAttr_t attributes = {
					nullptr,
#ifdef osCMSIS_FreeRTOS // FreeRTOS doesn't support robust mutexes
					osMutexPrioInherit | osMutexRecursive,
#else
					osMutexPrioInherit | osMutexRobust | osMutexRecursive,
#endif
					nullptr,
					0
				};
				handle = osMutexNew(&attributes);
			}
			return nullptr != handle;
		}
	};

	template<class T>
	/// @brief A class to automatically lock and unlock a mutex when the scope ends.
	/// Meant for systems with no support for std::lock_guard.
	class LockGuard
	{
	public:
		/// @brief Constructor for the LockGuard class.
		/// @param mutex The mutex to lock.
		/// @details Locks the mutex when the scope starts.
		/// Unlocks the mutex when the scope ends.
		LockGuard(T *mutex) :
		  lockable(mutex)
		{
			lockable->lock();
		}

		/// @brief Destructor for the LockGuard class.
		/// @details Unlocks the mutex when the scope ends.
		~LockGuard()
		{
			lockable->unlock();
		}

	private:
		T *lockable; ///< The mutex to lock and unlock.
	};
} // namespace isobus

namespace std
{
	using mutex = isobus::Mutex;
	using recursive_mutex = isobus::RecursiveMutex;
} // namespace std

#define LOCK_GUARD(type, x) const LockGuard<type> x##Lock(&x)

#else

#include <mutex>
namespace isobus
{
	using Mutex = std::mutex;
	using RecursiveMutex = std::recursive_mutex;
}
/// @brief A macro to automatically lock a mutex and unlock it when the scope ends.
/// @param type The type of the mutex.
/// @param x The mutex to lock.
#define LOCK_GUARD(type, x) const std::lock_guard<type> x##Lock(x)

#endif

#include <atomic>
#include <vector>

/// @brief A template class for a lock free queue.
/// @tparam T The item type for the queue.
template<typename T>
class LockFreeQueue
{
public:
	/// @brief Constructor for the lock free queue.
	explicit LockFreeQueue(std::size_t size) :
	  buffer(size), capacity(size)
	{
		// Validate the size of the queue, if assertion is disabled, set the size to 1.
		assert(size > 0 && "The size of the queue must be greater than 0.");
		if (size == 0)
		{
			size = 1;
		}
	}

	/// @brief Push an item to the queue.
	/// @param item The item to push to the queue.
	/// @return True if the item was pushed to the queue, false if the queue is full.
	bool push(const T &item)
	{
		const auto currentWriteIndex = writeIndex.load(std::memory_order_relaxed);
		const auto nextWriteIndex = nextIndex(currentWriteIndex);

		if (nextWriteIndex == readIndex.load(std::memory_order_acquire))
		{
			// The buffer is full.
			return false;
		}

		buffer[currentWriteIndex] = item;
		writeIndex.store(nextWriteIndex, std::memory_order_release);
		return true;
	}

	/// @brief Peek at the next item in the queue.
	/// @param item The item to peek at in the queue.
	/// @return True if the item was peeked at in the queue, false if the queue is empty.
	bool peek(T &item)
	{
		const auto currentReadIndex = readIndex.load(std::memory_order_relaxed);
		if (currentReadIndex == writeIndex.load(std::memory_order_acquire))
		{
			// The buffer is empty.
			return false;
		}

		item = buffer[currentReadIndex];
		return true;
	}

	/// @brief Pop an item from the queue.
	/// @return True if the item was popped from the queue, false if the queue is empty.
	bool pop()
	{
		const auto currentReadIndex = readIndex.load(std::memory_order_relaxed);
		if (currentReadIndex == writeIndex.load(std::memory_order_acquire))
		{
			// The buffer is empty.
			return false;
		}

		readIndex.store(nextIndex(currentReadIndex), std::memory_order_release);
		return true;
	}

	/// @brief Check if the queue is full.
	/// @return True if the queue is full, false if the queue is not full.
	bool is_full() const
	{
		return nextIndex(writeIndex.load(std::memory_order_acquire)) == readIndex.load(std::memory_order_acquire);
	}

	/// @brief Clear the queue.
	void clear()
	{
		// Simply move the read index to the write index.
		readIndex.store(writeIndex.load(std::memory_order_acquire), std::memory_order_release);
	}

private:
	std::vector<T> buffer; ///< The buffer for the circular buffer.
	std::atomic<std::size_t> readIndex = { 0 }; ///< The read index for the circular buffer.
	std::atomic<std::size_t> writeIndex = { 0 }; ///< The write index for the circular buffer.
	const std::size_t capacity; ///< The capacity of the circular buffer.

	/// @brief Get the next index in the circular buffer.
	/// @param current The current index.
	/// @return The next index in the circular buffer.
	std::size_t nextIndex(std::size_t current) const
	{
		return (current + 1) % capacity;
	}
};

#endif // THREAD_SYNCHRONIZATION_HPP
