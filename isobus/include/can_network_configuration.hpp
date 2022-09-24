//================================================================================================
/// @file can_network_configuration.hpp
///
/// @brief This is a class for changing stack settings.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_NETWORK_CONFIGURATION_HPP
#define CAN_NETWORK_CONFIGURAION_HPP

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

    private:
        static std::uint32_t maxNumberTransportProtocolSessions; ///< The max number of TP sessions allowed
    };
} // namespace isobus

#endif // CAN_NETWORK_CONFIGURATION_HPP
