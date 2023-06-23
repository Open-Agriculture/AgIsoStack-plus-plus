#include <gtest/gtest.h>

#include "isobus/utility/event_dispatcher.hpp"

#include <chrono>
#include <memory>
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
		dispatcher.invoke(true);
		EXPECT_EQ(dispatcher.get_listener_count(), 1);
	}

	// Invoke is required to automatically remove expired listeners.
	dispatcher.invoke(true);
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

	dispatcher.invoke(true);
	ASSERT_EQ(count, 1);

	dispatcher.invoke(true);
	ASSERT_EQ(count, 2);
}

TEST(EVENT_DISPATCHER_TESTS, MultipleArguments)
{
	EventDispatcher<bool, int, float> dispatcher;

	int count = 0;
	std::function<void(const bool &, const int &, const float &)> callback = [&count](bool value, int value2, float value3) {
		ASSERT_TRUE(value);
		ASSERT_EQ(value2, 42);
		ASSERT_EQ(value3, 3.14f);
		count += 1;
	};
	auto listener = dispatcher.add_listener(callback);

	dispatcher.invoke(true, 42, 3.14f);
	ASSERT_EQ(count, 1);

	dispatcher.invoke(true, 42, 3.14f);
	ASSERT_EQ(count, 2);
}

TEST(EVENT_DISPATCHER_TESTS, InvokeContextEvent)
{
	EventDispatcher<bool> dispatcher;

	int count = 0;
	std::function<void(const bool &, std::shared_ptr<int>)> callback = [&count](bool value, std::shared_ptr<int> context) {
		ASSERT_TRUE(value);
		ASSERT_EQ(*context, 42);
		count += 1;
	};
	auto context = std::make_shared<int>(42);
	auto listener = dispatcher.add_listener<int>(callback, context);

	dispatcher.invoke(true);
	ASSERT_EQ(count, 1);

	dispatcher.invoke(true);
	ASSERT_EQ(count, 2);

	context = nullptr;

	dispatcher.invoke(true);
	ASSERT_EQ(count, 2);
}

TEST(EVENT_DISPATCHER_TESTS, InvokeUnsafeContextEvent)
{
	EventDispatcher<bool> dispatcher;

	int count = 0;
	std::function<void(const bool &, std::weak_ptr<int>)> callback = [&count](bool value, std::weak_ptr<int> context) {
		if (count == 0)
		{
			ASSERT_FALSE(context.expired());
			ASSERT_EQ(*context.lock(), 42);
		}
		else
		{
			ASSERT_TRUE(context.expired());
		}
		ASSERT_TRUE(value);
		count += 1;
	};

	auto context = std::make_shared<int>(42);
	auto listener = dispatcher.add_unsafe_listener<int>(callback, context);

	dispatcher.invoke(true);
	ASSERT_EQ(count, 1);

	context = nullptr;

	dispatcher.invoke(true);
	ASSERT_EQ(count, 2);
}

TEST(EVENT_DISPATCHER_TESTS, CallEvent)
{
	EventDispatcher<bool> dispatcher;

	int count = 0;
	std::function<void(const bool &)> callback = [&count](bool value) {
		ASSERT_TRUE(value);
		count += 1;
	};
	auto listener = dispatcher.add_listener(callback);

	bool lvalue = true;
	dispatcher.call(lvalue);
	ASSERT_EQ(count, 1);

	dispatcher.call(lvalue);
	ASSERT_EQ(count, 2);
}