#include <gtest/gtest.h>

#include "isobus/utility/event_dispatcher.hpp"

#include <chrono>
#include <thread>

using namespace isobus;

TEST(EVENT_DISPATCHER_TESTS, AddRemoveListener)
{
	EventDispatcher<bool> dispatcher;

	std::function<void(const bool &)> callback = [](bool) {};

	// Use different scopes to test the lifetime of the listeners.
	{
		auto listener = dispatcher.add_listener(callback);
		EXPECT_EQ(dispatcher.get_listener_count(), 1);
		{
			auto listener2 = dispatcher.add_listener(callback);
			EXPECT_EQ(dispatcher.get_listener_count(), 2);
		}
		EXPECT_EQ(dispatcher.get_listener_count(), 2);

		// Invoke is required to automatically remove expired listeners.
		dispatcher.invoke({ true });
		EXPECT_EQ(dispatcher.get_listener_count(), 1);
	}

	// Invoke is required to automatically remove expired listeners.
	dispatcher.invoke({ true });
	EXPECT_EQ(dispatcher.get_listener_count(), 0);
}

TEST(EVENT_DISPATCHER_TESTS, InvokeEvent)
{
	EventDispatcher<bool> dispatcher;

	int count = 0;
	std::function<void(const bool &)> callback = [&count](bool value) {
		ASSERT_TRUE(value);
		count += 1;
	};
	auto listener = dispatcher.add_listener(callback);

	dispatcher.invoke({ true });
	ASSERT_EQ(count, 1);

	dispatcher.invoke({ true });
	ASSERT_EQ(count, 2);
}