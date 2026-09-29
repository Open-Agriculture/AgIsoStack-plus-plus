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

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <type_traits>
#include <utility>

#if defined CAN_STACK_DISABLE_THREADS || defined ARDUINO
#include <queue>

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
	/// @brief A no-op thread wrapper when threading is disabled.
	class Thread
	{
	public:
		template<typename Function>
		explicit Thread(Function &&)
		{
		}
		bool joinable() const
		{
			return false;
		}
		bool is_current_thread() const
		{
			return false;
		}
		void join() {}
	};
}
/// @brief Disabled LOCK_GUARD macro since threads are disabled.
#define LOCK_GUARD(type, x)

/// @brief A template class for a queue, since threads are disabled this is a simple queue.
/// @tparam T The item type for the queue.
template<typename T>
class LockFreeQueue
{
public:
	/// @brief Constructor for the lock free queue.
	explicit LockFreeQueue(std::size_t) {}

	/// @brief Push an item to the queue.
	/// @param item The item to push to the queue.
	/// @return Simply returns true, since this version of the queue is not limited in size.
	bool push(const T &item)
	{
		queue.push(item);
		return true;
	}

	/// @brief Peek at the next item in the queue.
	/// @param item The item to peek at in the queue.
	/// @return True if the item was peeked at in the queue, false if the queue is empty.
	bool peek(T &item)
	{
		if (queue.empty())
		{
			return false;
		}

		item = queue.front();
		return true;
	}

	/// @brief Pop an item from the queue.
	/// @return True if the item was popped from the queue, false if the queue is empty.
	bool pop()
	{
		if (queue.empty())
		{
			return false;
		}

		queue.pop();
		return true;
	}

	/// @brief Check if the queue is full.
	/// @return Always returns false, since this version of the queue is not limited in size.
	bool is_full() const
	{
		return false;
	}

	/// @brief Clear the queue.
	void clear()
	{
		queue = {};
	}

private:
	std::queue<T> queue; ///< The queue
};

#else

#include <atomic>
#include <cassert>
#include <vector>

#if defined USE_CMSIS_RTOS2_THREADING
#include <functional>
#if defined __ZEPHYR__
#include <zephyr/portability/cmsis_os2.h>
#else
#include "cmsis_os2.h"
#endif
#else
#include <condition_variable>
#include <mutex>
#include <thread>
#endif

namespace isobus
{
#if defined USE_CMSIS_RTOS2_THREADING
	/// @brief Creates an RTOS object on first use, allowing static wrapper construction.
	template<typename Id>
	class CMSISHandle
	{
	public:
		Id get(Id (*create)()) const
		{
			auto state = status.load(std::memory_order_acquire);
			if (2U == state)
			{
				return handle;
			}
			if (3U == state)
			{
				return nullptr;
			}
			unsigned int uninitialized = 0U;
			if (status.compare_exchange_strong(uninitialized, 1U, std::memory_order_acq_rel))
			{
				handle = create();
				status.store((nullptr != handle) ? 2U : 3U, std::memory_order_release);
				return handle;
			}

			state = uninitialized;
			while (1U == state)
			{
				osDelay(1U);
				state = status.load(std::memory_order_acquire);
			}
			return (2U == state) ? handle : nullptr;
		}

		Id existing() const
		{
			return (2U == status.load(std::memory_order_acquire)) ? handle : nullptr;
		}

	private:
		mutable std::atomic<unsigned int> status = { 0U }; ///< 0: new, 1: creating, 2: ready, 3: failed
		mutable Id handle = nullptr;
	};

	template<bool Recursive>
	class CMSISMutex
	{
	public:
		CMSISMutex() = default;

		~CMSISMutex()
		{
			if (nullptr != mutex.existing())
			{
				osMutexDelete(mutex.existing());
			}
		}

		CMSISMutex(const CMSISMutex &) = delete;
		CMSISMutex &operator=(const CMSISMutex &) = delete;

		void lock()
		{
			const auto handle = mutex.get(create);
			if ((nullptr == handle) || (osOK != osMutexAcquire(handle, osWaitForever)))
			{
				std::abort();
			}
		}

		bool try_lock()
		{
			const auto handle = mutex.get(create);
			return (nullptr != handle) && (osOK == osMutexAcquire(handle, 0U));
		}

		void unlock()
		{
			const auto handle = mutex.existing();
			if ((nullptr == handle) || (osOK != osMutexRelease(handle)))
			{
				std::abort();
			}
		}

	private:
		static osMutexId_t create()
		{
			osMutexAttr_t attributes = {};
			attributes.attr_bits = osMutexPrioInherit | (Recursive ? osMutexRecursive : 0U);
			return osMutexNew(&attributes);
		}

		CMSISHandle<osMutexId_t> mutex;
	};

	using Mutex = CMSISMutex<false>;
	using RecursiveMutex = CMSISMutex<true>;

	template<typename MutexType>
	class LockGuard
	{
	public:
		explicit LockGuard(MutexType &mutex) :
		  mutex(mutex)
		{
			this->mutex.lock();
		}

		~LockGuard()
		{
			mutex.unlock();
		}

		LockGuard(const LockGuard &) = delete;
		LockGuard &operator=(const LockGuard &) = delete;

	private:
		MutexType &mutex;
	};

	template<typename MutexType>
	class UniqueLock
	{
	public:
		explicit UniqueLock(MutexType &mutex) :
		  mutex(&mutex),
		  ownsLock(true)
		{
			mutex.lock();
		}

		~UniqueLock()
		{
			if (ownsLock)
			{
				mutex->unlock();
			}
		}

		UniqueLock(const UniqueLock &) = delete;
		UniqueLock &operator=(const UniqueLock &) = delete;

		void lock()
		{
			if (!ownsLock)
			{
				mutex->lock();
				ownsLock = true;
			}
		}

		void unlock()
		{
			if (ownsLock)
			{
				mutex->unlock();
				ownsLock = false;
			}
		}

	private:
		MutexType *mutex;
		bool ownsLock;
	};

	class Thread
	{
	public:
		template<typename Function>
		explicit Thread(Function &&function) :
		  function(std::forward<Function>(function))
		{
			osThreadAttr_t attributes = {};
			attributes.attr_bits = osThreadJoinable;
			thread = osThreadNew(run, this, &attributes);
			if (nullptr == thread)
			{
				std::abort();
			}
		}

		~Thread()
		{
			if (joinable())
			{
				join();
			}
		}

		Thread(const Thread &) = delete;
		Thread &operator=(const Thread &) = delete;

		bool joinable() const
		{
			return nullptr != thread;
		}

		bool is_current_thread() const
		{
			return (nullptr != thread) && (thread == osThreadGetId());
		}

		void join()
		{
			if (!joinable() || is_current_thread())
			{
				std::abort();
			}
			const auto result = osThreadJoin(thread);
			// Zephyr reports osErrorResource if the joinable thread already exited.
			if ((osOK != result) && !((osErrorResource == result) && (osThreadTerminated == osThreadGetState(thread))))
			{
				std::abort();
			}
			thread = nullptr;
		}

	private:
		static void run(void *argument)
		{
			auto *self = static_cast<Thread *>(argument);
			self->function();
		}

		std::function<void()> function;
		osThreadId_t thread = nullptr;
	};

	inline std::uint32_t cmsis_ticks_for(const std::chrono::milliseconds &duration)
	{
		if (duration <= duration.zero())
		{
			return 0U;
		}
		const std::uint64_t frequency = osKernelGetTickFreq();
		if (0U == frequency)
		{
			std::abort();
		}
		const std::uint64_t milliseconds = static_cast<std::uint64_t>(duration.count());
		const std::uint64_t maximum = osWaitForever - 1U;
		if (milliseconds >= (maximum * 1000U) / frequency)
		{
			return static_cast<std::uint32_t>(maximum);
		}
		return static_cast<std::uint32_t>((milliseconds * frequency + 999U) / 1000U);
	}

	/// @brief CMSIS event flag wait for one waiting thread per instance.
	/// @note notify_all() wakes that one waiter; sharing this instance among waiters is unsupported.
	class ConditionVariable
	{
	public:
		ConditionVariable() = default;
		~ConditionVariable()
		{
			if (nullptr != eventFlags.existing())
			{
				osEventFlagsDelete(eventFlags.existing());
			}
		}

		ConditionVariable(const ConditionVariable &) = delete;
		ConditionVariable &operator=(const ConditionVariable &) = delete;

		template<typename MutexType>
		void wait(UniqueLock<MutexType> &lock)
		{
			wait(lock, osWaitForever);
		}

		template<typename MutexType>
		void wait_for(UniqueLock<MutexType> &lock, const std::chrono::milliseconds &duration)
		{
			wait(lock, cmsis_ticks_for(duration));
		}

		void notify_one()
		{
			const auto handle = eventFlags.get(create);
			if ((nullptr == handle) || (0U != (osEventFlagsSet(handle, 1U) & osFlagsError)))
			{
				std::abort();
			}
		}

		void notify_all()
		{
			// Current stack users have one waiter per condition variable.
			notify_one();
		}

	private:
		static osEventFlagsId_t create()
		{
			return osEventFlagsNew(nullptr);
		}

		template<typename MutexType>
		void wait(UniqueLock<MutexType> &lock, std::uint32_t timeout)
		{
			const auto handle = eventFlags.get(create);
			if (nullptr == handle)
			{
				std::abort();
			}
			lock.unlock();
			const auto result = osEventFlagsWait(handle, 1U, osFlagsWaitAny, timeout);
			const bool timedOut = (osFlagsErrorTimeout == result) || ((0U == timeout) && (osFlagsErrorResource == result));
			if ((0U != (result & osFlagsError)) && !timedOut)
			{
				std::abort();
			}
			lock.lock();
		}

		CMSISHandle<osEventFlagsId_t> eventFlags;
	};

	inline void sleep_for(const std::chrono::milliseconds &duration)
	{
		if (duration <= duration.zero())
		{
			return;
		}
		if (osOK != osDelay(cmsis_ticks_for(duration)))
		{
			std::abort();
		}
	}

	inline void yield()
	{
		if (osOK != osThreadYield())
		{
			std::abort();
		}
	}
#else
	using Mutex = std::mutex;
	using RecursiveMutex = std::recursive_mutex;
	template<typename MutexType>
	using LockGuard = std::lock_guard<MutexType>;
	template<typename MutexType>
	using UniqueLock = std::unique_lock<MutexType>;
	class Thread
	{
	public:
		template<typename Function, typename = typename std::enable_if<!std::is_same<typename std::decay<Function>::type, Thread>::value>::type>
		explicit Thread(Function &&function) :
		  thread(std::forward<Function>(function))
		{
		}

		bool joinable() const
		{
			return thread.joinable();
		}

		bool is_current_thread() const
		{
			return joinable() && (thread.get_id() == std::this_thread::get_id());
		}

		void join()
		{
			if (joinable() && !is_current_thread())
			{
				thread.join();
			}
		}

	private:
		std::thread thread;
	};

	using ConditionVariable = std::condition_variable;

	inline void sleep_for(const std::chrono::milliseconds &duration)
	{
		std::this_thread::sleep_for(duration);
	}

	inline void yield()
	{
		std::this_thread::yield();
	}
#endif
}
/// @brief A macro to automatically lock a mutex and unlock it when the scope ends.
/// @param type The type of the mutex.
/// @param x The mutex to lock.
#define LOCK_GUARD(type, x) const isobus::LockGuard<type> x##Lock(x)

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

#endif

#include <queue>
template<typename T>
class UnsafeQueue
{
public:
	using value_type = T;

	template<typename U, typename = typename std::enable_if<std::is_convertible<U, value_type>::value>::type>
	void push(U &&item)
	{
		queue.push(std::forward<U>(item));
	}

	bool pop(value_type *item)
	{
		if (queue.empty())
		{
			return false;
		}
		*item = std::move(queue.front());
		queue.pop();
		return true;
	}

	void clear()
	{
		queue = {};
	}

private:
	std::queue<value_type> queue;
};

#if defined CAN_STACK_DISABLE_THREADS || defined ARDUINO
template<typename T>
using Queue = UnsafeQueue<T>;
#else

template<typename T>
class SafeQueue : private UnsafeQueue<T>
{
	using Q = UnsafeQueue<T>;

public:
	using value_type = T;

	template<typename U, typename = typename std::enable_if<std::is_convertible<U, value_type>::value>::type>
	void push(U &&item)
	{
		isobus::LockGuard<isobus::Mutex> lock(mtx);
		Q::push(std::forward<U>(item));
	}

	bool pop(value_type *item)
	{
		isobus::LockGuard<isobus::Mutex> lock(mtx);
		return Q::pop(item);
	}

	void clear()
	{
		isobus::LockGuard<isobus::Mutex> lock(mtx);
		Q::clear();
	}

private:
	isobus::Mutex mtx;
};
template<typename T>
using Queue = SafeQueue<T>;
#endif

#endif // THREAD_SYNCHRONIZATION_HPP
