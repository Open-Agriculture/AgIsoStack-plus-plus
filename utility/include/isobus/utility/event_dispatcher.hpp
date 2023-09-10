//================================================================================================
/// @file event_dispatcher.hpp
///
/// @brief An object to represent a dispatcher that can invoke callbacks in a thread-safe manner.
/// @author Daan Steenbergen
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef EVENT_DISPATCHER_HPP
#define EVENT_DISPATCHER_HPP

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

#if !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
#include <mutex>
#endif

namespace isobus
{
	//================================================================================================
	/// @class EventDispatcher
	///
	/// @brief A dispatcher that notifies listeners when an event is invoked.
	//================================================================================================
	template<typename... E>
	class EventDispatcher
	{
	public:
		/// @brief Register a callback to be invoked when the event is invoked.
		/// @param callback The callback to register.
		/// @return A shared pointer to the callback.
		std::shared_ptr<std::function<void(const E &...)>> add_listener(const std::function<void(const E &...)> &callback)
		{
#if !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
			std::lock_guard<std::mutex> lock(callbacksMutex);
#endif
			auto shared = std::make_shared<std::function<void(const E &...)>>(callback);
			callbacks.push_back(shared);
			return shared;
		}

		/// @brief Register a callback to be invoked when the event is invoked.
		/// @param callback The callback to register.
		/// @param context The context object to pass through to the callback.
		/// @return A shared pointer to the contextless callback.
		template<typename C>
		std::shared_ptr<std::function<void(const E &...)>> add_listener(const std::function<void(const E &..., std::shared_ptr<C>)> &callback, std::weak_ptr<C> context)
		{
			std::function<void(const E &...)> callbackWrapper = [callback, context](const E &...args) {
				if (auto contextPtr = context.lock())
				{
					callback(args..., contextPtr);
				}
			};
			return add_listener(callbackWrapper);
		}

		/// @brief Register an unsafe callback to be invoked when the event is invoked.
		/// @param callback The callback to register.
		/// @param context The context object to pass through to the callback.
		/// @return A shared pointer to the contextless callback.
		template<typename C>
		std::shared_ptr<std::function<void(const E &...)>> add_unsafe_listener(const std::function<void(const E &..., std::weak_ptr<C>)> &callback, std::weak_ptr<C> context)
		{
			std::function<void(const E &...)> callbackWrapper = [callback, context](const E &...args) {
				callback(args..., context);
			};
			return add_listener(callbackWrapper);
		}

		/// @brief Get the number of listeners registered to this event.
		/// @return The number of listeners
		std::size_t get_listener_count()
		{
#if !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
			std::lock_guard<std::mutex> lock(callbacksMutex);
#endif
			return callbacks.size();
		}

		/// @brief Remove expired listeners from the dispatcher
		void remove_expired_listeners()
		{
			auto removeResult = std::remove_if(callbacks.begin(), callbacks.end(), [](std::weak_ptr<std::function<void(const E &...)>> &callback) {
				return callback.expired();
			});
			callbacks.erase(removeResult, callbacks.end());
		}

		/// @brief Call and event with context that is moved using move semantics to notify all listeners.
		/// @param args The event context to notify listeners with.
		/// @return True if the event was successfully invoked, false otherwise.
		void invoke(E &&...args)
		{
#if !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
			std::lock_guard<std::mutex> lock(callbacksMutex);
#endif
			remove_expired_listeners();

			std::for_each(callbacks.begin(), callbacks.end(), [&args...](std::weak_ptr<std::function<void(const E &...)>> &callback) {
				if (auto callbackPtr = callback.lock())
				{
					(*callbackPtr)(std::forward<E>(args)...);
				}
			});
		}

		/// @brief Call an event with existing context to notify all listeners.
		/// @param args The event context to notify listeners with.
		/// @return True if the event was successfully invoked, false otherwise.
		void call(const E &...args)
		{
#if !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
			std::lock_guard<std::mutex> lock(callbacksMutex);
#endif
			remove_expired_listeners();

			std::for_each(callbacks.begin(), callbacks.end(), [&args...](std::weak_ptr<std::function<void(const E &...)>> &callback) {
				if (auto callbackPtr = callback.lock())
				{
					(*callbackPtr)(args...);
				}
			});
		}

	private:
		std::vector<std::weak_ptr<std::function<void(const E &...)>>> callbacks; ///< The callbacks to invoke
#if !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
		std::mutex callbacksMutex; ///< The mutex to protect the callbacks
#endif
	};
} // namespace isobus

#endif // EVENT_DISPATCHER_HPP
