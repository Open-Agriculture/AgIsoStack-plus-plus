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
#include "isobus/isobus/can_frame.hpp"
#include "isobus/isobus/can_hardware_abstraction.hpp"

/* special address description flags for the CAN_ID */
#define CAN_EFF_FLAG 0x80000000UL /* EFF/SFF is set in the MSB */
#define CAN_RTR_FLAG 0x40000000UL /* remote transmission request */
#define CAN_ERR_FLAG 0x20000000UL /* error message frame */

/* valid bits in CAN ID for frame formats */
#define CAN_SFF_MASK 0x000007FFUL /* standard frame format (SFF) */
#define CAN_EFF_MASK 0x1FFFFFFFUL /* extended frame format (EFF) */
#define CAN_ERR_MASK 0x1FFFFFFFUL /* omit EFF, RTR, ERR flags */

/* CAN payload length and DLC definitions according to ISO 11898-1 */
#define CAN_MAX_DLEN 8

//================================================================================================
/// @class MCP2515CANInterface
///
/// @brief A CAN Driver for the MCP2515 CAN controller
//================================================================================================
class MCP2515CANInterface : public CANHardwarePlugin
{
public:
	/// @brief A callback for SPI transmission
	typedef bool (*SPITransmissionCallback)(const std::vector<std::uint8_t> txData, std::vector<std::uint8_t> &rxData);

	/// @brief Constructor for the socket CAN driver
	explicit MCP2515CANInterface(SPITransmissionCallback callback, const std::uint8_t cfg1, const std::uint8_t cfg2, const std::uint8_t cfg3);

	/// @brief The destructor for SocketCANInterface
	~MCP2515CANInterface();

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
	/// @brief All the instructions of the MCP2515
	enum class MCPInstruction : uint8_t
	{
		WRITE = 0x02,
		READ = 0x03,
		BITMOD = 0x05,
		LOAD_TX0 = 0x40,
		LOAD_TX1 = 0x42,
		LOAD_TX2 = 0x44,
		RTS_TX0 = 0x81,
		RTS_TX1 = 0x82,
		RTS_TX2 = 0x84,
		RTS_ALL = 0x87,
		READ_RX0 = 0x90,
		READ_RX1 = 0x94,
		READ_STATUS = 0xA0,
		RX_STATUS = 0xB0,
		RESET = 0xC0
	};

	/// @brief All the registers of the MCP2515
	enum class MCPRegister : uint8_t
	{
		RXF0SIDH = 0x00,
		RXF0SIDL = 0x01,
		RXF0EID8 = 0x02,
		RXF0EID0 = 0x03,
		RXF1SIDH = 0x04,
		RXF1SIDL = 0x05,
		RXF1EID8 = 0x06,
		RXF1EID0 = 0x07,
		RXF2SIDH = 0x08,
		RXF2SIDL = 0x09,
		RXF2EID8 = 0x0A,
		RXF2EID0 = 0x0B,
		CANSTAT = 0x0E,
		CANCTRL = 0x0F,
		RXF3SIDH = 0x10,
		RXF3SIDL = 0x11,
		RXF3EID8 = 0x12,
		RXF3EID0 = 0x13,
		RXF4SIDH = 0x14,
		RXF4SIDL = 0x15,
		RXF4EID8 = 0x16,
		RXF4EID0 = 0x17,
		RXF5SIDH = 0x18,
		RXF5SIDL = 0x19,
		RXF5EID8 = 0x1A,
		RXF5EID0 = 0x1B,
		TEC = 0x1C,
		REC = 0x1D,
		RXM0SIDH = 0x20,
		RXM0SIDL = 0x21,
		RXM0EID8 = 0x22,
		RXM0EID0 = 0x23,
		RXM1SIDH = 0x24,
		RXM1SIDL = 0x25,
		RXM1EID8 = 0x26,
		RXM1EID0 = 0x27,
		CNF3 = 0x28,
		CNF2 = 0x29,
		CNF1 = 0x2A,
		CANINTE = 0x2B,
		CANINTF = 0x2C,
		EFLG = 0x2D,
		TXB0CTRL = 0x30,
		TXB0SIDH = 0x31,
		TXB0SIDL = 0x32,
		TXB0EID8 = 0x33,
		TXB0EID0 = 0x34,
		TXB0DLC = 0x35,
		TXB0DATA = 0x36,
		TXB1CTRL = 0x40,
		TXB1SIDH = 0x41,
		TXB1SIDL = 0x42,
		TXB1EID8 = 0x43,
		TXB1EID0 = 0x44,
		TXB1DLC = 0x45,
		TXB1DATA = 0x46,
		TXB2CTRL = 0x50,
		TXB2SIDH = 0x51,
		TXB2SIDL = 0x52,
		TXB2EID8 = 0x53,
		TXB2EID0 = 0x54,
		TXB2DLC = 0x55,
		TXB2DATA = 0x56,
		RXB0CTRL = 0x60,
		RXB0SIDH = 0x61,
		RXB0SIDL = 0x62,
		RXB0EID8 = 0x63,
		RXB0EID0 = 0x64,
		RXB0DLC = 0x65,
		RXB0DATA = 0x66,
		RXB1CTRL = 0x70,
		RXB1SIDH = 0x71,
		RXB1SIDL = 0x72,
		RXB1EID8 = 0x73,
		RXB1EID0 = 0x74,
		RXB1DLC = 0x75,
		RXB1DATA = 0x76
	};

	struct TXBRegister
	{
		MCPRegister ctrl;
		MCPRegister sidh;
		MCPRegister data;
	};

	struct RXBRegister
	{
		MCPRegister ctrl;
		MCPRegister data;
		std::uint8_t intf;
	};

	enum class MCPMode : std::uint8_t
	{
		NORMAL = 0x00,
		SLEEP = 0x20,
		LOOPBACK = 0x40,
		LISTEN_ONLY = 0x60,
		CONFIG = 0x80,
		POWERUP = 0xE0
	};

	constexpr static std::uint8_t NUM_WRITE_BUFFERS = 3; ///< The number of write buffers in the MCP2515
	constexpr static std::uint8_t NUM_READ_BUFFERS = 2; ///< The number of read buffers in the MCP2515

	constexpr static TXBRegister TXB[NUM_WRITE_BUFFERS] = {
		{ MCPRegister::TXB0CTRL, MCPRegister::TXB0SIDH, MCPRegister::TXB0DATA },
		{ MCPRegister::TXB1CTRL, MCPRegister::TXB1SIDH, MCPRegister::TXB1DATA },
		{ MCPRegister::TXB2CTRL, MCPRegister::TXB2SIDH, MCPRegister::TXB2DATA }
	}; ///< The registers of the TX buffers

	constexpr static RXBRegister RXB[NUM_READ_BUFFERS] = {
		{ MCPRegister::RXB0CTRL, MCPRegister::RXB0DATA, 0x01 },
		{ MCPRegister::RXB1CTRL, MCPRegister::RXB1DATA, 0x02 }
	}; ///< The registers of the RX buffers

	/// @brief Get the RX status
	/// @returns The RX status
	std::uint8_t get_rx_status() const;

	/// @brief read a single byte register of the mcp2515
	/// @param[in] address The address of the register to read
	/// @param[in, out] data The data that was read
	/// @returns If the read was successfull
	bool read_register(const MCPRegister address, std::uint8_t &data);

	/// @brief read multiple byte register of the mcp2515
	/// @param[in] address The address of the register to read
	/// @param[in, out] data The data that was read
	/// @param[in] length The length of the data to read
	/// @returns If the read was successfull
	bool read_register(const MCPRegister address, std::uint8_t data[], const std::size_t length);

	/// @brief writes an instruction with single data to the mcp2515
	/// @param[in] instruction The instruction to write
	/// @param[in] data The data to write
	/// @returns If the write was successfull
	bool write_instruction(const MCPInstruction instruction, const std::uint8_t data);

	/// @brief writes an instruction with data array to the mcp2515
	/// @param[in] instruction The instruction to write
	/// @param[in] data The data to write
	/// @param[in] length The length of the data to write
	/// @returns If the write was successfull
	bool write_instruction(const MCPInstruction instruction, const std::uint8_t data[], const std::size_t length);

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
	/// @param[in] length The length of the data to write
	/// @returns If the write was successfull
	bool write_register(const MCPRegister address, const std::uint8_t data[], const std::size_t length);

	/// @brief set the mode of the mcp2515
	/// @param[in] mode The mode to set the mcp2515 to
	/// @returns If the mode was set successfully
	bool set_mode(const MCPMode mode);

	SPITransmissionCallback callback; ///< The callback for SPI transmission
	const std::uint8_t cfg1, cfg2, cfg3; ///< The configuration values for can and clock speed
	bool initialized; ///< If the mcp2515 has been initialized and no errors have occured
};

#endif // MCP2515_CAN_INTERFACE_HPP
