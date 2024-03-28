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

#endif // THREAD_SYNCHRONIZATION_HPP
