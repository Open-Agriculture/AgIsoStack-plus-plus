#include <gtest/gtest.h>

#include "isobus/utility/event_dispatcher.hpp"

#include <chrono>
#include <memory>
#include <thread>

using namespace isobus;

// Test fixture for Event Dispatcher
template<typename... Args>
class EventDispatcherTest : public ::testing::Test
{
protected:
	EventDispatcher<Args...> dispatcher;
};

// Define a test suite for a specific callback signature
using EventManagerBool = EventDispatcherTest<bool>;
using EventManagerBoolIntFloat = EventDispatcherTest<bool, int, float>;

TEST_F(EventManagerBool, AddRemoveListener)
{
	std::function<void(const bool &)> callback = [](bool) {};

	auto listener = dispatcher.add_listener(callback);
	EXPECT_EQ(dispatcher.get_listener_count(), 1);
	auto listener2 = dispatcher.add_listener(callback);
	EXPECT_EQ(dispatcher.get_listener_count(), 2);
	dispatcher.remove_listener(listener);
	EXPECT_EQ(dispatcher.get_listener_count(), 1);
	dispatcher.remove_listener(listener2);
	EXPECT_EQ(dispatcher.get_listener_count(), 0);
}

TEST_F(EventManagerBool, InvokeEvent)
{
	int count = 0;
	std::function<void(const bool &)> callback = [&count](bool value) {
		ASSERT_TRUE(value);
		count += 1;
	};
	dispatcher.add_listener(callback);

	dispatcher.invoke(true);
	ASSERT_EQ(count, 1);

	dispatcher.invoke(true);
	ASSERT_EQ(count, 2);
}

TEST_F(EventManagerBoolIntFloat, MultipleArguments)
{
	int count = 0;
	std::function<void(const bool &, const int &, const float &)> callback = [&count](bool value, int value2, float value3) {
		ASSERT_TRUE(value);
		ASSERT_EQ(value2, 42);
		ASSERT_EQ(value3, 3.14f);
		count += 1;
	};
	dispatcher.add_listener(callback);

	dispatcher.invoke(true, 42, 3.14f);
	ASSERT_EQ(count, 1);

	dispatcher.invoke(true, 42, 3.14f);
	ASSERT_EQ(count, 2);
}

TEST_F(EventManagerBool, InvokeContextEvent)
{
	int count = 0;
	std::function<void(const bool &, std::shared_ptr<int>)> callback = [&count](bool value, std::shared_ptr<int> context) {
		ASSERT_TRUE(value);
		ASSERT_EQ(*context, 42);
		count += 1;
	};
	auto context = std::make_shared<int>(42);
	dispatcher.add_listener<int>(callback, context);

	dispatcher.invoke(true);
	ASSERT_EQ(count, 1);

	dispatcher.invoke(true);
	ASSERT_EQ(count, 2);

	context = nullptr;

	dispatcher.invoke(true);
	ASSERT_EQ(count, 2);
}

TEST_F(EventManagerBool, InvokeUnsafeContextEvent)
{
	int count = 0;
	std::function<void(const bool &, int *)> callback = [&count](bool value, int *context) {
		ASSERT_NE(context, nullptr);
		ASSERT_EQ(*context, 42);
		ASSERT_TRUE(value);
		count += 1;
	};

	int *context_ptr = new int(42);
	dispatcher.add_unsafe_listener<int>(callback, context_ptr);

	dispatcher.invoke(true);
	ASSERT_EQ(count, 1);
}

TEST_F(EventManagerBool, CallEvent)
{
	int count = 0;
	std::function<void(const bool &)> callback = [&count](bool value) {
		ASSERT_TRUE(value);
		count += 1;
	};
	dispatcher.add_listener(callback);

	bool lvalue = true;
	dispatcher.call(lvalue);
	ASSERT_EQ(count, 1);

	dispatcher.call(lvalue);
	ASSERT_EQ(count, 2);
}

// Test adding a callback from within another callback
TEST_F(EventManagerBool, AddCallbackWithinCallback)
{
	int initialCallbackExecuted = 0;
	int addedCallbackExecuted = 0;

	dispatcher.add_listener([&](bool) {
		initialCallbackExecuted++;
		// Attempt to add a new callback during the execution of this callback
		dispatcher.add_listener([&](bool) {
			addedCallbackExecuted++;
		});
	});

	// Execute callbacks for the first time; should only execute the initial callback
	dispatcher.invoke(true);
	EXPECT_EQ(initialCallbackExecuted, 1);
	EXPECT_EQ(addedCallbackExecuted, 0); // The added callback should not execute this time

	// Execute callbacks for the second time; both the initial and the newly added callback should execute
	dispatcher.invoke(true);
	EXPECT_EQ(initialCallbackExecuted, 2);
	EXPECT_EQ(addedCallbackExecuted, 1); // The added callback should execute this time
}

// Test removing a callback from within another callback
TEST_F(EventManagerBool, RemoveCallbackWithinCallback)
{
	int callbackToBeRemovedExecuted = 0;

	// Add a callback that will be removed
	auto callbackId = dispatcher.add_listener([&](bool) {
		callbackToBeRemovedExecuted++;
	});

	// Add another callback that removes the first one during its execution
	dispatcher.add_listener([&](bool) {
		dispatcher.remove_listener(callbackId);
	});

	// Execute callbacks for the first time; both callbacks should execute
	dispatcher.invoke(true);
	EXPECT_EQ(callbackToBeRemovedExecuted, 1);

	// Execute callbacks for the second time; the first callback should not execute as it was removed
	dispatcher.invoke(true);
	EXPECT_EQ(callbackToBeRemovedExecuted, 1); // Ensure the removed callback did not execute again
}
