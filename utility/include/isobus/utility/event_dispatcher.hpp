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

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

namespace isobus
{
	//================================================================================================
	/// @class EventDispatcher
	///
	/// @brief A dispatcher that notifies listeners when an event is invoked.
	//================================================================================================
	template<typename E>
	class EventDispatcher
	{
	public:
		/// @brief Register a callback to be invoked when the event is invoked.
		/// @param callback The callback to register.
		/// @return A shared pointer to the callback.
		std::shared_ptr<std::function<void(const E &)>> add_listener(std::function<void(const E &)> &callback)
		{
			auto shared = std::make_shared<std::function<void(const E &)>>(callback);
			callbacks.push_back(shared);
			return shared;
		}

		/// @brief Get the number of listeners registered to this event.
		/// @return The number of listeners
		std::size_t get_listener_count() const
		{
			return callbacks.size();
		}

		/// @brief Invokes an event and notify all listeners.
		/// @param args The arguments to pass to the listeners.
		/// @return True if the event was successfully invoked, false otherwise.
		bool invoke(E &&args)
		{
			// Remove all callbacks that are gone, only if we are not dispatching.
			if (0 == concurrent_invokes_count)
			{
				for (auto it = callbacks.begin(); it != callbacks.end();)
				{
					if (it->expired())
					{
						it = callbacks.erase(it);
					}
					else
					{
						++it;
					}
				}
			}

			concurrent_invokes_count++;
			try
			{
				std::size_t current = 0;
				while (current < callbacks.size())
				{
					if (auto callback = callbacks[current++].lock())
					{
						(*callback)(args);
					}
				}
				concurrent_invokes_count--;
			}
			catch (...)
			{
				concurrent_invokes_count--;
				return false;
			}
			return true;
		}

	private:
		std::vector<std::weak_ptr<std::function<void(const E &)>>> callbacks; ///< The callbacks to invoke
		std::atomic<std::uint32_t> concurrent_invokes_count = { 0 }; ///< The number of concurrent invoke calls in progress
	};
} // namespace isobus

#endif // EVENT_DISPATCHER_HPP
