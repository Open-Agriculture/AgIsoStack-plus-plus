// Copyright 2026 Mauricio. Distributed under the MIT license.
#include <gtest/gtest.h>
#include "isobus/hardware_integration/available_can_drivers.hpp"

#ifdef ISOBUS_WINDOWSCANDLELIGHT_AVAILABLE
#include <atomic>
#include <cstring>
#include "candlelight_overlapped.hpp"

namespace
{
	// Real Windows overlapped I/O on a local named pipe, without USB/CAN hardware.
	class CandleLightIOTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			static std::atomic<unsigned> sequence{ 0 };
			const auto name = std::string("\\\\.\\pipe\\AgIsoStackCandleLight-") + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(sequence++);
			server = CreateNamedPipeA(name.c_str(), PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 64, 64, 0, nullptr);
			ASSERT_NE(INVALID_HANDLE_VALUE, server);
			event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
			ASSERT_NE(nullptr, event);
			operation.hEvent = event;
			client = CreateFileA(name.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
			ASSERT_NE(INVALID_HANDLE_VALUE, client);
			if (!ConnectNamedPipe(server, &operation))
			{
				ASSERT_EQ(static_cast<DWORD>(ERROR_PIPE_CONNECTED), GetLastError());
			}
			operation = {};
			operation.hEvent = event;
			ResetEvent(event);
			DWORD count = 0;
			ASSERT_FALSE(ReadFile(server, buffer, sizeof(buffer), &count, &operation));
			ASSERT_EQ(static_cast<DWORD>(ERROR_IO_PENDING), GetLastError());
			pending = true;
		}

		void TearDown() override
		{
			if (pending)
			{
				DWORD ignored = 0;
				isobus::detail::cancel_and_reap(server, operation, ignored);
			}
			if (INVALID_HANDLE_VALUE != client)
				CloseHandle(client);
			if (INVALID_HANDLE_VALUE != server)
				CloseHandle(server);
			if (nullptr != event)
				CloseHandle(event);
		}

		void send_payload()
		{
			DWORD count = 0;
			ASSERT_TRUE(WriteFile(client, payload, sizeof(payload), &count, nullptr));
			ASSERT_EQ(sizeof(payload), count);
		}

		HANDLE server = INVALID_HANDLE_VALUE;
		HANDLE client = INVALID_HANDLE_VALUE;
		HANDLE event = nullptr;
		OVERLAPPED operation{};
		bool pending = false;
		char buffer[24]{};
		const char payload[20] = "CAN frame payload";
	};

	TEST_F(CandleLightIOTest, PendingReadSurvivesPollingTimeouts)
	{
		DWORD count = 0, error = 0;
		EXPECT_EQ(isobus::detail::TransferPollResult::Pending, isobus::detail::poll_overlapped(server, operation, 0, count, error));
		EXPECT_EQ(isobus::detail::TransferPollResult::Pending, isobus::detail::poll_overlapped(server, operation, 0, count, error));
		send_payload();
		ASSERT_EQ(isobus::detail::TransferPollResult::Completed, isobus::detail::poll_overlapped(server, operation, 1000, count, error));
		EXPECT_EQ(sizeof(payload), count);
		EXPECT_EQ(0, std::memcmp(payload, buffer, sizeof(payload)));
	}

	TEST_F(CandleLightIOTest, SuccessfulCompletionWinsCancellationRace)
	{
		send_payload();
		ASSERT_EQ(static_cast<DWORD>(WAIT_OBJECT_0), WaitForSingleObject(event, 1000));
		DWORD count = 0;
		ASSERT_TRUE(isobus::detail::cancel_and_reap(server, operation, count));
		EXPECT_EQ(sizeof(payload), count);
		EXPECT_EQ(0, std::memcmp(payload, buffer, sizeof(payload)));
	}

	TEST_F(CandleLightIOTest, PendingOperationIsReapedBeforeStorageIsReleased)
	{
		DWORD count = 0;
		EXPECT_FALSE(isobus::detail::cancel_and_reap(server, operation, count));
		EXPECT_EQ(static_cast<DWORD>(ERROR_OPERATION_ABORTED), GetLastError());
		EXPECT_EQ(static_cast<DWORD>(WAIT_OBJECT_0), WaitForSingleObject(event, 0));
	}
}
#endif
