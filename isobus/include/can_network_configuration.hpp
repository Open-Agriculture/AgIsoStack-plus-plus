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
		CANNetworkConfiguration();

		/// @brief The destructor for the configuration object
		~CANNetworkConfiguration();

		/// @brief Configures the max number of concurrent TP sessions to provide a RAM limit for TP sessions
		/// @param[in] value The max allowable number of TP sessions
		static void set_max_number_transport_protcol_sessions(std::uint32_t value);

		/// @brief Returns the max number of concurrent TP sessions
		/// @returns The max number of concurrent TP sessions
		static std::uint32_t get_max_number_transport_protcol_sessions();

		/// @brief Sets the minimum time to wait between sending BAM frames
		/// @details The acceptable range as defined by ISO-11783 is 10 to 200 ms.
		/// This is a minumum time, so if you set it to some value, like 10 ms, the
		/// stack will attempt to transmit it as close to that time as it can, but it is
		/// not possible to 100% ensure it.
		/// @param[in] value The minimum time to wait between sending BAM frames
		static void set_minimum_time_between_transport_protocol_bam_frames(std::uint32_t value);

		/// @brief Returns the minimum time to wait between sending BAM frames
		/// @returns The minimum time to wait between sending BAM frames
		static std::uint32_t get_minimum_time_between_transport_protocol_bam_frames();

	private:
		static constexpr std::uint8_t DEFAULT_BAM_PACKET_DELAY_TIME_MS = 50; ///< The default time between BAM frames, as defined by J1939

		static std::uint32_t maxNumberTransportProtocolSessions; ///< The max number of TP sessions allowed
		static std::uint32_t minimumTimeBetweenTransportProtocolBAMFrames; ///< The configurable time between BAM frames
	};
} // namespace isobus

#endif // CAN_NETWORK_CONFIGURATION_HPP
