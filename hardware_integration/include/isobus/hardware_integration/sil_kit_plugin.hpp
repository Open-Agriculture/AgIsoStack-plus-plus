//================================================================================================
/// @file sil_kit_plugin.hpp
///
/// @brief A CAN driver for Vector SIL Kit virtual bus simulation.
/// @details Connects to a SIL Kit simulation and exchanges CAN frames via a virtual CAN network.
/// @author Matthias Lieb
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef SIL_KIT_PLUGIN_HPP
#define SIL_KIT_PLUGIN_HPP

#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/isobus/can_message_frame.hpp"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>

// Forward declarations to avoid including SIL Kit headers in the public API
namespace SilKit
{
	class IParticipant;
	namespace Services
	{
		namespace Can
		{
			class ICanController;
		} // namespace Can
	} // namespace Services
	namespace Services
	{
		namespace Orchestration
		{
			class ILifecycleService;
		} // namespace Orchestration
	} // namespace Services
} // namespace SilKit

namespace isobus
{
	//================================================================================================
	/// @class SILKitPlugin
	///
	/// @brief A CAN hardware plugin for Vector SIL Kit co-simulation.
	/// @details Connects to a running SIL Kit registry and creates a CAN controller on a named
	/// network. Received frames are queued internally so that the synchronous `read_frame()`
	/// interface required by AgIsoStack can dequeue them one at a time.
	//================================================================================================
	class SILKitPlugin : public CANHardwarePlugin
	{
	public:
		/// @brief Constructor
		/// @param[in] participantName Unique name for this SIL Kit participant
		/// @param[in] networkName Name of the virtual CAN network to join (all controllers
		///            using the same network name can exchange frames)
		/// @param[in] registryUri URI of the SIL Kit registry, e.g. "silkit://localhost:8500"
		explicit SILKitPlugin(const std::string &participantName,
		                      const std::string &networkName = "CAN1",
		                      const std::string &registryUri = "silkit://localhost:8500");

		/// @brief Destructor — disconnects from the SIL Kit simulation
		~SILKitPlugin() override;

		/// @brief Returns the display name of the driver
		/// @returns The name string identifying this SIL Kit participant and network
		std::string get_name() const override;

		/// @brief Returns whether the SIL Kit connection is active and valid
		/// @returns `true` if the connection is active, otherwise `false`
		bool get_is_valid() const override;

		/// @brief Connects to the SIL Kit registry and starts the CAN controller
		void open() override;

		/// @brief Disconnects from the SIL Kit registry
		void close() override;

		/// @brief Returns a frame from the internal receive queue (synchronous).
		/// @param[in, out] canFrame The CAN frame that was read
		/// @returns `true` if a CAN frame was available, otherwise `false`
		bool read_frame(isobus::CANMessageFrame &canFrame) override;

		/// @brief Sends a CAN frame via the SIL Kit virtual bus
		/// @param[in] canFrame The frame to write to the bus
		/// @returns `true` if the frame was sent, otherwise `false`
		bool write_frame(const isobus::CANMessageFrame &canFrame) override;

	private:
		static constexpr std::size_t MAX_QUEUE_SIZE = 1000; ///< Maximum number of frames buffered in the receive queue

		const std::string participantName; ///< Unique name for this SIL Kit participant
		const std::string networkName; ///< Name of the virtual CAN network to join
		const std::string registryUri; ///< URI of the SIL Kit registry

		std::unique_ptr<SilKit::IParticipant> participant; ///< The SIL Kit participant instance
		SilKit::Services::Can::ICanController *canController = nullptr; ///< The CAN controller created by the participant
		SilKit::Services::Orchestration::ILifecycleService *lifecycleService = nullptr; ///< The lifecycle service for this participant

		mutable std::mutex rxMutex; ///< Mutex protecting the receive queue
		std::condition_variable rxCondition; ///< Condition variable to signal new frames in the receive queue
		std::deque<isobus::CANMessageFrame> rxQueue; ///< Internal queue of received CAN frames

		std::atomic_bool running{ false }; ///< Flag indicating whether the SIL Kit connection is active
	};
} // namespace isobus
#endif // SIL_KIT_PLUGIN_HPP
