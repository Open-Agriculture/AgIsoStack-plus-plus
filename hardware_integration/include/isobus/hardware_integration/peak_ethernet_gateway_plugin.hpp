//================================================================================================
/// @file peak_ethernet_gateway_plugin.hpp
///
/// @brief An interface for using a PEAK Ethernet Gateway DR.
/// @author Adrian Del Grosso
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#ifndef PEAK_ETHERNET_GATEWAY_PLUGIN_HPP
#define PEAK_ETHERNET_GATEWAY_PLUGIN_HPP

#include <deque>
#include <memory>
#include <string>

#ifdef WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#endif

#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/isobus/can_hardware_abstraction.hpp"
#include "isobus/isobus/can_message_frame.hpp"

namespace isobus
{
	/// @brief Peak Ethernet Gateway CAN hardware plugin
	class PeakEthernetGatewayPlugin : public CANHardwarePlugin
	{
	public:
		/// @brief Constructor for a PeakEthernetGatewayPlugin
		/// @param[in] targetIPAddress The IP address of the gateway
		/// @param[in] txPort The port to send data to
		/// @param[in] rxPort The port to receive data from (this is the port the gateway sends data from)
		/// @param[in] useUDP If `true`, use UDP, otherwise use TCP
		PeakEthernetGatewayPlugin(std::string targetIPAddress,
		                          std::uint16_t txPort,
		                          std::uint16_t rxPort,
		                          bool useUDP = true);

		/// @brief The destructor for PCANBasicWindowsPlugin
		virtual ~PeakEthernetGatewayPlugin();

		/// @brief Returns if the connection with the hardware is valid
		/// @returns `true` if connected, `false` if not connected
		bool get_is_valid() const override;

		/// @brief Closes the connection to the hardware
		void close() override;

		/// @brief Connects to the hardware you specified in the constructor's channel argument
		void open() override;

		/// @brief Returns a frame from the hardware (synchronous), or `false` if no frame can be read.
		/// @param[in, out] canFrame The CAN frame that was read
		/// @returns `true` if a CAN frame was read, otherwise `false`
		bool read_frame(isobus::CANMessageFrame &canFrame) override;

		/// @brief Writes a frame to the bus (synchronous)
		/// @param[in] canFrame The frame to write to the bus
		/// @returns `true` if the frame was written, otherwise `false`
		bool write_frame(const isobus::CANMessageFrame &canFrame) override;

	private:
		/// @brief Opens the receive socket
		/// @returns `true` if the socket was opened, otherwise `false`
		bool open_rx_socket();

		/// @brief Opens the transmit socket
		/// @returns `true` if the socket was opened, otherwise `false`
		bool open_tx_socket();

		static constexpr std::uint32_t RX_BUFFER_SIZE_BYTES = 2048; ///< Default size of the receive buffer

		std::unique_ptr<std::uint8_t> rxBuffer; ///< The receive buffer (multiple frames can be in the buffer, per ethernet frame)
		std::deque<isobus::CANMessageFrame> rxQueue; ///< The queue of received frames, which get passed to the CAN Hardware Abstraction when needed
		std::string ipAddress; ///< The IP address of the gateway
		int rxSocket = 0; ///< The receive socket identifier
		int txSocket = 0; ///< The transmit socket identifier
		std::uint16_t sendPort; ///< The port to send data to
		std::uint16_t receivePort; ///< The port to receive data from
		bool isOpen = false; ///< If the connection is open
		bool udp; ///< If the connection is using UDP
	};
}
#endif // PEAK_ETHERNET_GATEWAY_PLUGIN_HPP
