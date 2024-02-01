//================================================================================================
/// @file data_monitor.cpp
///
/// @brief Provides the monitor design pattern for several data types.
/// @author Daan Steenbergen
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#include "isobus/utility/data_monitor.hpp"

#if !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO

using namespace isobus;
void ConcurrentReadingMonitor::writeEntry()
{
	std::unique_lock<std::mutex> lock(mutex);
	// Wait until there are no readers and no writers
	writeCondition.wait(lock, [this] { return !hasWriter && (numberOfReaders == 0); });
	hasWriter = true;
}

void ConcurrentReadingMonitor::writeExit()
{
	hasWriter = false;
	writeCondition.notify_one(); // Only one writer can write at a time, so no need to notify all
	readCondition.notify_all(); // Notify all readers that a writer has finished
}

void ConcurrentReadingMonitor::readEntry()
{
	std::unique_lock<std::mutex> lock(mutex);
	// Wait until there are no writers, there may still be readers
	readCondition.wait(lock, [this] { return !hasWriter; });
	numberOfReaders += 1;
}

void ConcurrentReadingMonitor::readExit()
{
	std::unique_lock<std::mutex> lock(mutex);
	numberOfReaders -= 1;
	if (numberOfReaders == 0)
	{
		writeCondition.notify_one(); // Notify a writer that all readers have finished
	}
}
#endif
