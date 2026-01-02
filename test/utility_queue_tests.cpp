#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <iostream>
#include <isobus/utility/thread_synchronization.hpp>
#include <random>
#include <thread>
#include <vector>
class QUEUE_TESTS : public ::testing::Test
{
protected:
	static constexpr std::size_t QUEUE_SIZE = 500;
	Queue<int> queue{ QUEUE_SIZE };
};

TEST_F(QUEUE_TESTS, MultipleProducersMultipleConsumersStressTest)
{
	const int NUM_PRODUCERS = 16;
	const int NUM_CONSUMERS = 4;
	const int ITEMS_PER_PRODUCER = 100000; // Reduced for faster testing
	const int TOTAL_ITEMS = NUM_PRODUCERS * ITEMS_PER_PRODUCER;

	std::atomic<int> produced_count{ 0 };
	std::atomic<int> consumed_count{ 0 };
	std::vector<std::atomic<bool>> producer_done(NUM_PRODUCERS);
	for (auto &done : producer_done)
		done = false;
	std::vector<std::thread> producers;

	// Producers
	for (int p = 0; p < NUM_PRODUCERS; ++p)
	{
		producers.emplace_back([this, ITEMS_PER_PRODUCER, &produced_count, &producer_done, p]() {
			for (int i = 0; i < ITEMS_PER_PRODUCER; ++i)
			{
				while (!queue.push(i))
				{
					// Spin if queue is full
					std::this_thread::yield();
				}
				produced_count++;
			}
			producer_done[p] = true; // Mark this producer as done
		});
	}

	// Multiple Consumers
	std::vector<std::thread> consumers;
	for (int c = 0; c < NUM_CONSUMERS; ++c)
	{
		consumers.emplace_back([this, &consumed_count, &producer_done]() {
			while (true)
			{
				bool all_producers_done = true;
				for (const auto &done : producer_done)
				{
					if (!done.load())
					{
						all_producers_done = false;
						break;
					}
				}

				if (queue.pop())
				{
					consumed_count++;
				}
				else if (all_producers_done)
				{
					break; // All producers done and queue is empty
				}
				else
				{
					std::this_thread::yield();
				}
			}
		});
	}

	// Wait for producers
	for (auto &t : producers)
	{
		t.join();
	}

	// Wait for consumers
	for (auto &t : consumers)
	{
		t.join();
	}

	// Check if all items were produced
	EXPECT_EQ(produced_count.load(), TOTAL_ITEMS);
	// Due to race conditions, consumed_count might be less than produced if data is overwritten
	std::cout << "Produced: " << produced_count.load() << ", Consumed: " << consumed_count.load() << std::endl;

	// This assertion may fail due to race conditions
	EXPECT_EQ(consumed_count.load(), TOTAL_ITEMS);
}

// Test all methods of the queue
TEST_F(QUEUE_TESTS, QueueAPIMethodsTest)
{
	// Test 1: Basic push/pop operations
	EXPECT_TRUE(queue.push(1));
	EXPECT_TRUE(queue.push(2));
	EXPECT_TRUE(queue.push(3));

	EXPECT_EQ(queue.size(), 3);
	EXPECT_FALSE(queue.is_empty());

	// Test 2: peek method
	int peek_value = 0;
	EXPECT_TRUE(queue.peek(peek_value));
	EXPECT_EQ(peek_value, 1); // Should be first item

	// Test 3: pop() without parameter
	EXPECT_TRUE(queue.pop());
	EXPECT_EQ(queue.size(), 2);

	// Test 4: pop(value_type*) method
	int popped_value1 = 0;
	EXPECT_TRUE(queue.pop(&popped_value1));
	EXPECT_EQ(popped_value1, 2);
	EXPECT_EQ(queue.size(), 1);

	// Test 5: pop(value_type&) method
	int popped_value2 = 0;
	EXPECT_TRUE(queue.pop(popped_value2));
	EXPECT_EQ(popped_value2, 3);
	EXPECT_EQ(queue.size(), 0);

	// Test 6: Empty queue checks
	EXPECT_TRUE(queue.is_empty());
	int temp = 0;
	EXPECT_FALSE(queue.peek(temp));
	EXPECT_FALSE(queue.pop());
	EXPECT_FALSE(queue.pop(&temp));
	EXPECT_FALSE(queue.pop(temp));

	// Test 7: Clear method
	EXPECT_TRUE(queue.push(10));
	EXPECT_TRUE(queue.push(20));
	EXPECT_EQ(queue.size(), 2);

	queue.clear();
	EXPECT_EQ(queue.size(), 0);
	EXPECT_TRUE(queue.is_empty());

	// Test 8: Move semantics
	int moved_value = 42;
	EXPECT_TRUE(queue.push(std::move(moved_value)));
	int result = 0;
	EXPECT_TRUE(queue.pop(result));
	EXPECT_EQ(result, 42);
}
