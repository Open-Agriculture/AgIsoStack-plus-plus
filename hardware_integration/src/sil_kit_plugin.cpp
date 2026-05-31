//================================================================================================
/// @file sil_kit_plugin.cpp
///
/// @brief Implementation of the SIL Kit CAN hardware plugin.
/// @author Matthias Lieb
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================

#include "isobus/hardware_integration/sil_kit_plugin.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

#include <silkit/SilKit.hpp>
#include <silkit/services/can/all.hpp>
#include <silkit/services/orchestration/all.hpp>

#include <chrono>
#include <future>

namespace isobus
{
	SILKitPlugin::SILKitPlugin(const std::string &participantName,
	                           const std::string &networkName,
	                           const std::string &registryUri) :
	  participantName(participantName),
	  networkName(networkName),
	  registryUri(registryUri)
	{
	}

	SILKitPlugin::~SILKitPlugin()
	{
		close();
	}

	std::string SILKitPlugin::get_name() const
	{
		return "SIL Kit: " + participantName + " on " + networkName;
	}

	bool SILKitPlugin::get_is_valid() const
	{
		return running.load();
	}

	void SILKitPlugin::open()
	{
		if (running.load())
		{
			return;
		}

		try
		{
			auto config = SilKit::Config::ParticipantConfigurationFromString("{}");
			participant = SilKit::CreateParticipant(config, participantName, registryUri);

			canController = participant->CreateCanController("AgIsoStackCAN", networkName);

			// Register frame receive handler — queues incoming frames for read_frame()
			canController->AddFrameHandler(
			  [this](SilKit::Services::Can::ICanController *,
			         const SilKit::Services::Can::CanFrameEvent &event) {
				  if (!running.load())
				  {
					  return;
				  }

				  CANMessageFrame frame;
				  frame.identifier = event.frame.canId;
				  frame.isExtendedFrame = (event.frame.flags & static_cast<SilKit::Services::Can::CanFrameFlagMask>(SilKit::Services::Can::CanFrameFlag::Ide)) != 0;

				  if (event.frame.dataField.size() > 8)
				  {
					  LOG_DEBUG("[SILKit] Received frame with data length %zu > 8, truncating to 8",
					            event.frame.dataField.size());
				  }

				  frame.dataLength = static_cast<std::uint8_t>(
				    event.frame.dataField.size() > 8 ? 8 : event.frame.dataField.size());
				  for (std::uint8_t i = 0; i < frame.dataLength; ++i)
				  {
					  frame.data[i] = event.frame.dataField[i];
				  }

				  frame.timestamp_us = static_cast<std::uint64_t>(
				    std::chrono::duration_cast<std::chrono::microseconds>(event.timestamp).count());

				  {
					  std::lock_guard<std::mutex> lock(rxMutex);
					  if (rxQueue.size() < MAX_QUEUE_SIZE)
					  {
						  rxQueue.push_back(frame);
					  }
					  else
					  {
						  LOG_WARNING("[SILKit] Receive queue full, dropping frame with canId=0x%08X", frame.identifier);
					  }
				  }
				  rxCondition.notify_one();
			  });

			// Monitor transmit acknowledgements for diagnostics
			canController->AddFrameTransmitHandler(
			  [](SilKit::Services::Can::ICanController *,
			     const SilKit::Services::Can::CanFrameTransmitEvent &event) {
				  if (SilKit::Services::Can::CanTransmitStatus::Transmitted != event.status)
				  {
					  LOG_WARNING("[SILKit] Frame transmit status: %d for canId=0x%08X",
					              static_cast<int>(event.status),
					              event.canId);
				  }
			  });

			canController->Start();

			// Create lifecycle in autonomous mode — frames are only delivered once
			// the lifecycle reaches the Running state.
			lifecycleService = participant->CreateLifecycleService(
			  { SilKit::Services::Orchestration::OperationMode::Autonomous });

			std::promise<void> runningPromise;
			auto runningFuture = runningPromise.get_future();
			lifecycleService->SetCommunicationReadyHandler([&runningPromise]() {
				runningPromise.set_value();
			});

			lifecycleService->StartLifecycle();

			// Wait for the lifecycle to reach at least CommunicationReady before
			// allowing frames to be sent, otherwise SendFrame silently drops them.
			if (runningFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready)
			{
				running.store(true);
				LOG_INFO("[SILKit] Connected participant '%s' to network '%s' via %s",
				         participantName.c_str(),
				         networkName.c_str(),
				         registryUri.c_str());
			}
			else
			{
				LOG_ERROR("[SILKit] Lifecycle did not reach CommunicationReady within 5 s");
				lifecycleService->Stop("Timeout");
				participant.reset();
				canController = nullptr;
				lifecycleService = nullptr;
			}
		}
		catch (const std::exception &e)
		{
			LOG_ERROR("[SILKit] Failed to connect: %s", e.what());
			participant.reset();
			canController = nullptr;
			running.store(false);
			lifecycleService = nullptr;
		}
	}

	void SILKitPlugin::close()
	{
		if (!running.load())
		{
			return;
		}

		running.store(false);
		rxCondition.notify_all();

		try
		{
			if (nullptr != canController)
			{
				canController->Stop();
			}
			if (nullptr != lifecycleService)
			{
				lifecycleService->Stop("Shutdown");
				// Optionally wait for lifecycle to reach Stopped state
			}
		}
		catch (const std::exception &e)
		{
			LOG_WARNING("[SILKit] Error during shutdown: %s", e.what());
		}

		canController = nullptr;
		lifecycleService = nullptr;
		participant.reset();

		{
			std::lock_guard<std::mutex> lock(rxMutex);
			rxQueue.clear();
		}
		LOG_INFO("[SILKit] Disconnected participant '%s' from network '%s'", participantName.c_str(), networkName.c_str());
	}

	bool SILKitPlugin::read_frame(isobus::CANMessageFrame &canFrame)
	{
		std::unique_lock<std::mutex> lock(rxMutex);
		if (rxQueue.empty())
		{
			rxCondition.wait_for(lock, std::chrono::seconds(1), [this]() {
				return !rxQueue.empty() || !running.load();
			});
		}

		if (rxQueue.empty())
		{
			return false;
		}

		canFrame = rxQueue.front();
		rxQueue.pop_front();
		return true;
	}

	bool SILKitPlugin::write_frame(const isobus::CANMessageFrame &canFrame)
	{
		if (!running.load() || nullptr == canController)
		{
			return false;
		}

		try
		{
			SilKit::Services::Can::CanFrame silkitFrame{};
			silkitFrame.canId = canFrame.identifier;
			silkitFrame.flags = canFrame.isExtendedFrame
			  ? static_cast<SilKit::Services::Can::CanFrameFlagMask>(SilKit::Services::Can::CanFrameFlag::Ide)
			  : 0;
			silkitFrame.dlc = canFrame.dataLength;
			silkitFrame.dataField = SilKit::Util::Span<const uint8_t>(canFrame.data, canFrame.dataLength);

			canController->SendFrame(silkitFrame);
			LOG_DEBUG("[SILKit] Sent frame: canId=0x%08X, dlc=%u, ext=%d",
			          canFrame.identifier,
			          canFrame.dataLength,
			          canFrame.isExtendedFrame ? 1 : 0);
			return true;
		}
		catch (const std::exception &e)
		{
			LOG_ERROR("[SILKit] SendFrame failed: %s", e.what());
			return false;
		}
	}
} // namespace isobus
