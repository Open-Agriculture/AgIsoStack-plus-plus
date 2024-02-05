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

#include "isobus/utility/event_dispatcher.hpp"

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

#else

#include <mutex>
namespace isobus
{
	using Mutex = std::mutex;
	using RecursiveMutex = std::recursive_mutex;
};
/// @brief A macro to automatically lock a mutex and unlock it when the scope ends.
/// @param type The type of the mutex.
/// @param x The mutex to lock.
#define LOCK_GUARD(type, x) const std::lock_guard<type> x##Lock(x)

#endif

#endif // THREAD_SYNCHRONIZATION_HPP
