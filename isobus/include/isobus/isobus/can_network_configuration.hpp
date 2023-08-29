//================================================================================================
/// @file can_network_configuration.hpp
///
/// @brief This is a class for changing stack settings.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_NETWORK_CONFIGURATION_HPP
#define CAN_NETWORK_CONFIGURATION_HPP

#include <cstdint>

namespace isobus
{
	//================================================================================================
	/// @class CANNetworkConfiguration
	///
	/// @brief A class that defines stack-wide configuration data. You can set the values in there
	/// to suit your specific memory constraints.
	//================================================================================================
	class CANNetworkConfiguration
	{
	public:
		/// @brief The constructor for the configuration object
		CANNetworkConfiguration() = default;

		/// @brief The destructor for the configuration object
		~CANNetworkConfiguration() = default;

		/// @brief Configures the max number of concurrent TP sessions to provide a RAM limit for TP sessions
		/// @param[in] value The max allowable number of TP sessions
		void set_max_number_transport_protocol_sessions(std::uint32_t value);

		/// @brief Returns the max number of concurrent TP sessions
		/// @returns The max number of concurrent TP sessions
		std::uint32_t get_max_number_transport_protocol_sessions() const;

		/// @brief Sets the minimum time to wait between sending BAM frames
		/// @details The acceptable range as defined by ISO-11783 is 10 to 200 ms.
		/// This is a minumum time, so if you set it to some value, like 10 ms, the
		/// stack will attempt to transmit it as close to that time as it can, but it is
		/// not possible to 100% ensure it.
		/// @param[in] value The minimum time to wait between sending BAM frames
		void set_minimum_time_between_transport_protocol_bam_frames(std::uint32_t value);

		/// @brief Returns the minimum time to wait between sending BAM frames
		/// @returns The minimum time to wait between sending BAM frames
		std::uint32_t get_minimum_time_between_transport_protocol_bam_frames() const;

		/// @brief Sets the max number of data frames the stack will use when
		/// in an ETP session, between EDPO phases. The default is 255,
		/// but decreasing it may reduce bus load at the expense of transfer time.
		/// @param[in] numberFrames The max number of data frames to use
		void set_max_number_of_etp_frames_per_edpo(std::uint8_t numberFrames);

		/// @brief Returns the max number of data frames the stack will use when
		/// in an ETP session, between EDPO phases. The default is 255,
		/// but decreasing it may reduce bus load at the expense of transfer time.
		/// @returns The number of data frames the stack will use when sending ETP messages between EDPOs
		std::uint8_t get_max_number_of_etp_frames_per_edpo() const;

		/// @brief Sets the max number of data frames the stack will send from each
		/// transport layer protocol, per update. The default is 255,
		/// but decreasing it may reduce bus load at the expense of transfer time.
		/// @param[in] numberFrames The max number of frames to use
		void set_max_number_of_network_manager_protocol_frames_per_update(std::uint8_t numberFrames);

		/// @brief Returns the max number of data frames the stack will send from each
		/// transport layer protocol, per update. The default is 255,
		/// but decreasing it may reduce bus load at the expense of transfer time.
		/// @returns The max number of frames to use in transport protocols in each network manager update
		std::uint8_t get_max_number_of_network_manager_protocol_frames_per_update() const;

	private:
		static constexpr std::uint8_t DEFAULT_BAM_PACKET_DELAY_TIME_MS = 50; ///< The default time between BAM frames, as defined by J1939

		std::uint32_t maxNumberTransportProtocolSessions = 4; ///< The max number of TP sessions allowed
		std::uint32_t minimumTimeBetweenTransportProtocolBAMFrames = DEFAULT_BAM_PACKET_DELAY_TIME_MS; ///< The configurable time between BAM frames
		std::uint8_t extendedTransportProtocolMaxNumberOfFramesPerEDPO = 0xFF; ///< Used to control throttling of ETP sessions.
		std::uint8_t networkManagerMaxFramesToSendPerUpdate = 0xFF; ///< Used to control the max number of transport layer frames added to the driver queue per network manager update
	};
} // namespace isobus

#endif // CAN_NETWORK_CONFIGURATION_HPP
