// Copyright 2026 Mauricio. Distributed under the MIT license.
#ifndef CANDLELIGHT_OVERLAPPED_HPP
#define CANDLELIGHT_OVERLAPPED_HPP

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

/// @cond INTERNAL_CANDLELIGHT_TRANSPORT
namespace isobus
{
	namespace detail
	{
		enum class TransferPollResult
		{
			Pending,
			Completed,
			Failed
		};

		// A polling timeout does not cancel or release the operation or its buffer.
		inline TransferPollResult poll_overlapped(HANDLE file, OVERLAPPED &operation, DWORD timeout, DWORD &transferred, DWORD &error)
		{
			const DWORD waitResult = WaitForSingleObject(operation.hEvent, timeout);
			if (WAIT_TIMEOUT == waitResult)
			{
				return TransferPollResult::Pending;
			}
			if (WAIT_OBJECT_0 != waitResult)
			{
				error = (WAIT_FAILED == waitResult) ? GetLastError() : ERROR_INVALID_FUNCTION;
				return TransferPollResult::Failed;
			}
			if (!GetOverlappedResult(file, &operation, &transferred, FALSE))
			{
				error = GetLastError();
				return TransferPollResult::Failed;
			}
			return TransferPollResult::Completed;
		}

		// Cancellation may lose the race to a successful transfer. Always reap it,
		// and return that final result before its OVERLAPPED/buffer can be reused.
		inline bool cancel_and_reap(HANDLE file, OVERLAPPED &operation, DWORD &transferred)
		{
			CancelIoEx(file, &operation);
			return 0 != GetOverlappedResult(file, &operation, &transferred, TRUE);
		}
	}
}
/// @endcond
#endif
