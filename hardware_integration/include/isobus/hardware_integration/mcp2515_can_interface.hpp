//================================================================================================
/// @file mcp2515_can_interface.hpp
///
/// @brief An interface for using the MCP2515 can controller.
/// @author Daan Steenbergen
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================
#ifndef MCP2515_CAN_INTERFACE_HPP
#define MCP2515_CAN_INTERFACE_HPP

#include <cstring>
#include <string>
#include <vector>

#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/hardware_integration/spi_hardware_plugin.hpp"
#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/can_hardware_abstraction.hpp"
#include "isobus/isobus/can_message_frame.hpp"

namespace isobus
{
	//================================================================================================
	/// @class MCP2515CANInterface
	///
	/// @brief A CAN Driver for the MCP2515 CAN controller
	//================================================================================================
	class MCP2515CANInterface : public CANHardwarePlugin
	{
	public:
		/// @brief Constructor for the socket CAN driver
		/// @param[in] transactionHandler The SPI transaction handler
		/// @param[in] cfg1 The configuration value for CFG register 1
		/// @param[in] cfg2 The configuration value for CFG register 2
		/// @param[in] cfg3 The configuration value for CFG register 3
		MCP2515CANInterface(SPIHardwarePlugin *transactionHandler, const std::uint8_t cfg1, const std::uint8_t cfg2, const std::uint8_t cfg3);

		/// @brief The destructor for SocketCANInterface
		virtual ~MCP2515CANInterface();

		/// @brief Returns the displayable name of the plugin
		/// @returns MCP2515
		std::string get_name() const override;

		/// @brief Returns if the socket connection is valid
		/// @returns `true` if connected, `false` if not connected
		bool get_is_valid() const override;

		/// @brief Closes the socket
		void close() override;

		/// @brief Connects to the socket
		void open() override;

		/// @brief Resets the MCP2515
		/// @returns `true` if the reset was successful, otherwise `false`
		bool reset();

		/// @brief Returns a frame from the hardware (synchronous), or `false` if no frame can be read.
		/// @param[in, out] canFrame The CAN frame that was read
		/// @returns `true` if a CAN frame was read, otherwise `false`
		bool read_frame(isobus::CANMessageFrame &canFrame) override;

		/// @brief Writes a frame to the bus (synchronous)
		/// @param[in] canFrame The frame to write to the bus
		/// @returns `true` if the frame was written, otherwise `false`
		bool write_frame(const isobus::CANMessageFrame &canFrame) override;

	private:
		/// @brief Some essential instructions of the MCP2515
		enum class MCPInstruction : std::uint8_t
		{
			WRITE = 0x02,
			READ = 0x03,
			BITMOD = 0x05,
			RX_STATUS = 0xB0,
			READ_STATUS = 0xA0,
			RESET = 0xC0
		};

		/// @brief Some essential registers of the MCP2515
		enum class MCPRegister : std::uint8_t
		{
			CANSTAT = 0x0E,
			CANCTRL = 0x0F,
			CNF3 = 0x28,
			CNF2 = 0x29,
			CNF1 = 0x2A,
			CANINTE = 0x2B,
			CANINTF = 0x2C,
			TXB0CTRL = 0x30,
			TXB0SIDH = 0x31,
			TXB1CTRL = 0x40,
			TXB1SIDH = 0x41,
			TXB2CTRL = 0x50,
			TXB2SIDH = 0x51,
			RXB0CTRL = 0x60,
			RXB0DATA = 0x66,
			RXB1CTRL = 0x70,
			RXB1DATA = 0x76
		};

		/// @brief The different modes of the MCP2515 associated with their internal bits
		enum class MCPMode : std::uint8_t
		{
			NORMAL = 0x00,
			SLEEP = 0x20,
			LOOPBACK = 0x40,
			LISTEN_ONLY = 0x60,
			CONFIG = 0x80,
		};

		static constexpr std::uint32_t RECEIVE_MESSAGE_READ_RATE = 10; ///< Hardcoded time in ms between polling the MCP2515 module for new messages, mostly arbitrary

		/// @brief Read the rx status of the mcp2515
		/// @param[out] status The status that was read
		/// @returns If the read was successfull
		bool get_read_status(std::uint8_t &status);

		/// @brief read a single byte register of the mcp2515
		/// @param[in] address The address of the register to read
		/// @param[out] data The data that was read
		/// @returns If the read was successfull
		bool read_register(const MCPRegister address, std::uint8_t &data);

		/// @brief read multiple byte register of the mcp2515
		/// @param[in] address The address of the register to read
		/// @param[out] data The data that was read
		/// @param[in] length The length of the data to read
		/// @returns If the read was successfull
		bool read_register(const MCPRegister address, std::uint8_t *data, const std::size_t length);

		/// @brief modify a register of the mcp2515
		/// @param[in] address The address of the register to modify
		/// @param[in] mask The mask to apply to the register
		/// @param[in] data The data to write to the register
		/// @returns If the write was successfull
		bool modify_register(const MCPRegister address, const std::uint8_t mask, const std::uint8_t data);

		/// @brief write a single byte register of the mcp2515
		/// @param[in] address The address of the register to write
		/// @param[in] data The data to write to the register
		/// @returns If the write was successfull
		bool write_register(const MCPRegister address, const std::uint8_t data);

		/// @brief write multiple byte register of the mcp2515
		/// @param[in] address The address of the register to write
		/// @param[in] data The data to write to the register
		/// @param[in] length The length of the data to write to the register
		/// @returns If the write was successfull
		bool write_register(const MCPRegister address, const std::uint8_t data[], const std::size_t length);

		/// @brief Reset the mcp2515 internally
		/// @returns If the reset was successfull
		bool write_reset();

		/// @brief set the mode of the mcp2515
		/// @param[in] mode The mode to set the mcp2515 to
		/// @returns If the mode was set successfully
		bool set_mode(const MCPMode mode);

		/// @brief Read a frame from a buffer on the mcp2515
		/// @param[in, out] canFrame The frame that was read
		/// @param[in] ctrlRegister The control register of the buffer to read from
		/// @param[in] dataRegister The data register of the buffer to read from
		/// @param[in] intfMask The interrupt flag of the buffer to reset after reading
		/// @returns If the read was successfull
		bool read_frame(isobus::CANMessageFrame &canFrame, const MCPRegister ctrlRegister, const MCPRegister dataRegister, const std::uint8_t intfMask);

		/// @brief Write a frame to a buffer on the mcp2515
		/// @param[in] canFrame The frame to write
		/// @param[in] ctrlRegister The control register of the buffer to write to
		/// @param[in] sidhRegister The sidh register of the buffer to write to
		/// @returns If the write was successfull
		bool write_frame(const isobus::CANMessageFrame &canFrame, const MCPRegister ctrlRegister, const MCPRegister sidhRegister);

		SPIHardwarePlugin *transactionHandler; ///< The SPI transaction handler
		std::uint8_t rxIndex = 0; ///< The index of the rx buffer to read from next
		std::uint8_t txIndex = 2; ///< The index of the tx buffer to write to next, start with 2 as it is the buffer with the highest priority
		std::uint8_t txPriority = 3; ///< The priority of the next tx frame
		const std::uint8_t cfg1; ///< Configuration value for CFG1 register
		const std::uint8_t cfg2; ///< Configuration value for CFG2 register
		const std::uint8_t cfg3; ///< Configuration value for CFG3 register
		bool initialized = false; ///< If the mcp2515 has been initialized and no errors have occurred
	};
}
#endif // MCP2515_CAN_INTERFACE_HPP
