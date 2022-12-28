//================================================================================================
/// @file mcp2515_can_interface.hpp
///
/// @brief An interface for using the MCP2515 can controller.
/// @author Daan Steenbergen
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#ifndef MCP2515_CAN_INTERFACE_HPP
#define MCP2515_CAN_INTERFACE_HPP

#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/hardware_integration/spi_transaction_plugin.hpp"
#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/can_frame.hpp"
#include "isobus/isobus/can_hardware_abstraction.hpp"

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
	/// @param[in] cfg1 The configuration register 1
	/// @param[in] cfg2 The configuration register 2
	/// @param[in] cfg3 The configuration register 3
	explicit MCP2515CANInterface(SPITransactionPlugin *transactionHandler, const std::uint8_t cfg1, const std::uint8_t cfg2, const std::uint8_t cfg3);

	/// @brief The destructor for SocketCANInterface
	virtual ~MCP2515CANInterface();

	/// @brief Returns if the socket connection is valid
	/// @returns `true` if connected, `false` if not connected
	bool get_is_valid() const override;

	/// @brief Closes the socket
	void close() override;

	/// @brief Connects to the socket
	void open() override;

	/// @brief Resets the MCP2515
	bool reset();

	/// @brief Returns a frame from the hardware (synchronous), or `false` if no frame can be read.
	/// @param[in, out] canFrame The CAN frame that was read
	/// @returns `true` if a CAN frame was read, otherwise `false`
	bool read_frame(isobus::HardwareInterfaceCANFrame &canFrame) override;

	/// @brief Writes a frame to the bus (synchronous)
	/// @param[in] canFrame The frame to write to the bus
	/// @returns `true` if the frame was written, otherwise `false`
	bool write_frame(const isobus::HardwareInterfaceCANFrame &canFrame) override;

private:
	/// @brief Some essential instructions of the MCP2515
	enum class MCPInstruction : std::uint8_t
	{
		WRITE = 0x02,
		READ = 0x03,
		BITMOD = 0x05,
		RX_STATUS = 0xB0,
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
		TXB0DATA = 0x36,
		TXB1CTRL = 0x40,
		TXB1SIDH = 0x41,
		TXB1DATA = 0x46,
		TXB2CTRL = 0x50,
		TXB2SIDH = 0x51,
		TXB2DATA = 0x56,
		RXB0CTRL = 0x60,
		RXB0DATA = 0x66,
		RXB1CTRL = 0x70,
		RXB1DATA = 0x76
	};

	/// @brief The needed registers for transmitting by TXBn for convenience
	struct TXBRegister
	{
		MCPRegister ctrl;
		MCPRegister sidh;
		MCPRegister data;
	};

	/// @brief The needed registers for receiving by RXBn for convenience
	struct RXBRegister
	{
		MCPRegister ctrl;
		MCPRegister data;
		std::uint8_t intf;
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

	static constexpr std::size_t NUM_WRITE_BUFFERS = 3; ///< The number of write buffers in the MCP2515
	static constexpr std::size_t NUM_READ_BUFFERS = 2; ///< The number of read buffers in the MCP2515

	static constexpr std::uint32_t CAN_SFF_MASK = 0x7FF; ///< The standard frame format (SFF) mask
	static constexpr std::uint32_t CAN_EFF_MASK = 0x1FFFFFFF; ///< The extended frame format (EFF) mask

	static constexpr std::uint32_t CAN_EFF_FLAG = 0x80000000; ///< This bit denotes if the frame is standard or extended format
	static constexpr std::uint32_t CAN_RTR_FLAG = 0x40000000; ///< This bit denotes if the frame is a remote transmission request (RTR)
	static constexpr std::uint32_t CAN_ERR_FLAG = 0x20000000; ///< This bit denotes if the frame is an error frame

	/// @brief Read the rx status of the mcp2515
	/// @param[out] status The status that was read
	/// @returns If the read was successfull
	bool get_rx_status(std::uint8_t &status);

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
	bool read_register(const MCPRegister address, std::uint8_t data[], const std::size_t length);

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
	/// @returns If the write was successfull
	bool write_register(const MCPRegister address, const std::uint8_t data[], const std::size_t length);

	/// @brief Reset the mcp2515 internally
	/// @returns If the reset was successfull
	bool write_reset();

	/// @brief set the mode of the mcp2515
	/// @param[in] mode The mode to set the mcp2515 to
	/// @returns If the mode was set successfully
	bool set_mode(const MCPMode mode);

	SPITransactionPlugin *transactionHandler; ///< The SPI transaction handler
	const std::uint8_t cfg1, cfg2, cfg3; ///< The configuration values for can and clock speed
	bool initialized; ///< If the mcp2515 has been initialized and no errors have occured
};

#endif // MCP2515_CAN_INTERFACE_HPP
