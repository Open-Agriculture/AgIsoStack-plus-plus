//================================================================================================
/// @file proemion_canlink_interface.hpp
///
/// @brief AgIsoStack++ CAN hardware interface for Proemion CANlink Wifi-CAN devices
///
/// The interface uses the Proemion Byte Command Protocol over TCP.
/// https://docs.proemion.com/docs/Byte_Command-Manual.html#process-data-messages
/// @author Miklos Marton
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================

#ifndef PROEMION_CANLINK_INTERFACE_HPP
#define PROEMION_CANLINK_INTERFACE_HPP

#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/isobus/can_message_frame.hpp"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace isobus
{
	//================================================================================================
	/// @class ProemionCANLinkInterface
	///
	/// @brief A CAN hardware driver for Proemion CANlink devices using the Byte Command Protocol
	/// over TCP.
	///
	/// Supported interface formats:
	///
	///   <ip-address>
	///   <ip-address>:<tcp-port>
	///
	/// If the TCP port is omitted, port 30000 is used.
	///
	/// Examples:
	///
	///   192.168.1.50
	///   192.168.1.50:31000
	///
	/// The CAN interface is configured to the ISOBUS bitrate of 250 kbit/s when opened.
	//================================================================================================
	class ProemionCANLinkInterface : public CANHardwarePlugin
	{
	public:
		/**
		 * @brief An enum for the bitrate values supported by the device
		 */
		enum BitrateKeys
		{
			Bitrate_10kb = 0x00, // 10 kBit / sec
			Bitrate_20kb = 0x01, // 20 kBit / sec
			Bitrate_50kb = 0x02, // 50 kBit / sec
			Bitrate_100kb = 0xFE, // 100 kBit / sec
			Bitrate_125kb = 0x03, // 125 kBit / sec
			Bitrate_250kb = 0x04, // 250 kBit / sec
			Bitrate_500kb = 0x05, // 500 kBit / sec
			Bitrate_800kb = 0x06, // 800 kBit / sec
			Bitrate_1Mb = 0x07, // 1 MBit / sec
			Bitrate_Custom = 0xFF,
		};
		/// @brief Constructor
		/// @param[in] interface Interface specification in the form IP or IP:port
		explicit ProemionCANLinkInterface(const std::string &interface);

		/// @brief Destructor
		~ProemionCANLinkInterface() override;

		/// @brief Returns the displayable name of the plugin
		/// @returns "Proemion CANlink"
		std::string get_name() const override;

		/// @brief Returns whether the TCP connection is currently valid
		/// @returns true if connected
		bool get_is_valid() const override;

		/// @brief Opens the TCP connection and configures the CAN bitrate to 250 kbit/s
		void open() override;

		/// @brief Closes the TCP connection
		void close() override;

		/// @brief Reads one CAN frame from the CANlink device
		/// @param[in,out] canFrame CAN frame to populate
		/// @returns true if a CAN frame was received
		bool read_frame(CANMessageFrame &canFrame) override;

		/// @brief Writes one CAN frame through the CANlink device
		/// @param[in] canFrame CAN frame to transmit
		/// @returns true if the frame was accepted by the TCP socket
		bool write_frame(const CANMessageFrame &canFrame) override;

	private:
		/// @brief DEFAULT_TCP_PORT The default TCP port
		static constexpr std::uint16_t DEFAULT_TCP_PORT = 30000;

		/// @brief Start of frame byte, suffixed with _BYTE to avoid collision with other defines
		static constexpr std::uint8_t SOF_BYTE = 0x43;
		/// @brief End of frame byte, suffixed with _BYTE to avoid collision with other defines
		static constexpr std::uint8_t EOF_BYTE = 0x0D;

		/// @brief Message type for timestamp less 11 bit frames
		static constexpr std::uint8_t COMMAND_CAN_11_BIT = 0x00;
		/// @brief Message type for timestamped 11 bit frames
		static constexpr std::uint8_t COMMAND_CAN_11_BIT_TIMESTAMP = 0x01;
		/// @brief Message type for timestamp less 29 bit frames
		static constexpr std::uint8_t COMMAND_CAN_29_BIT = 0x02;
		/// @brief Message type for timestamped 29 bit frames
		static constexpr std::uint8_t COMMAND_CAN_29_BIT_TIMESTAMP = 0x03;

		/// @brief Message type for get bitrate command
		static constexpr std::uint8_t COMMAND_GET_BITRATE = 0x56;
		/// @brief Message type for set bitrate command
		static constexpr std::uint8_t COMMAND_SET_BITRATE = 0x57;

		/**
		 * @brief parse_interface Parse the hostname:port supplied by the user
		 * @return True if the interface string matches the expected format
		 */
		bool parse_interface();

		/**
		 * @brief connect_tcp Initiates the TCP socket connection to the device
		 * @return true if the socket connection initialized successfully
		 */
		bool connect_tcp();

		/**
		 * @brief configure_isobus_bitrate Configures the CAN interface to 250 kb/s
		 * @return true if the configuration is successful
		 */
		bool configure_isobus_bitrate();

		/**
		 * @brief request_current_bitrate Query the current bitrate from the device
		 * @param bitrate The bitrate read from the device. If the request fails the value will be unchanged
		 * @return True if the bitrate query was successful
		 */
		bool request_current_bitrate(BitrateKeys &bitrate);

		/**
		 * @brief set_bitrate Sets the bitrate of the device
		 * @param bitrate The bitrate key
		 * @return
		 */
		bool set_bitrate(BitrateKeys bitrate);

		/**
		 * @brief create_pdu Creates a valid PDU from the data provided (adds message boundaries, checksum)
		 * @param command Command code
		 * @param data Payload data
		 * @param dataLength Length of payload data
		 * @return Created PDU
		 */
		std::vector<std::uint8_t> create_pdu(std::uint8_t command, const std::uint8_t *data, std::size_t dataLength) const;

		/**
		 * @brief send_pdu Sends a PDU to the TCP socket
		 * @param pdu PDU to be ent
		 * @return True if the PDU is successfully sent over the socket
		 */
		bool send_pdu(const std::vector<std::uint8_t> &pdu);

		/**
		 * @brief receive_pdu Try to receive a PDU from the socket
		 * @param pdu [out] PDU read from the socket
		 * @param timeoutMs Timout in ms
		 * @return True if the PDU was read successfully from the scoket
		 */
		bool receive_pdu(std::vector<std::uint8_t> &pdu, int timeoutMs);

		/**
		 * @brief extract_pdu_from_receive_buffer Extracts a PDU from the receive buffer and free up it's place
		 * @param pdu [out] The read PDU
		 * @return True if the PDU had been read successfully
		 */
		bool extract_pdu_from_receive_buffer(std::vector<std::uint8_t> &pdu);

		/**
		 * @brief decode_can_frame Method to decode TCP data to the stack's CAN frame format
		 * @param pdu Raw TCP data
		 * @param canFrame Output CAN frame
		 * @return True if the decoding is successful
		 */
		bool decode_can_frame(const std::vector<std::uint8_t> &pdu, CANMessageFrame &canFrame) const;

		/**
		 * @brief calculate_checksum Calculates
		 * @param data Pointer to the data
		 * @param length Number of bytes in the data
		 * @return XOR checksum of the data
		 */
		static std::uint8_t calculate_checksum(const std::uint8_t *data, std::size_t length);

		/**
		 * @brief get_timestamp_us Helper function to return the host device uptime in us
		 * @return Uptime in us
		 */
		static std::uint64_t get_timestamp_us();

		/**
		 * @brief interfaceString The interface string as provided by the caller
		 */
		std::string interfaceString;

		/**
		 * @brief ipAddress The extracted hostname/IP address from the passed interfaceString parameter
		 */
		std::string ipAddress;

		/**
		 * @brief tcpPort The TCP port used for the connection
		 */
		std::uint16_t tcpPort = DEFAULT_TCP_PORT;

		/**
		 * @brief socketFd Socket descriptor
		 */
		int socketFd = -1;

		/**
		 * @brief receiveBuffer Buffer for the received data over TCP
		 */
		std::vector<std::uint8_t> receiveBuffer;

		/**
		 * @brief transmitMutex Mutex for CAN transmission
		 */
		std::mutex transmitMutex;
	};
}

#endif // PROEMION_CANLINK_INTERFACE_HPP
