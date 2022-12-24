//================================================================================================
/// @file mcp2515_can_interface.cpp
///
/// @brief An interface for using the MCP2515 can controller.
/// @author Daan Steenbergen
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus/hardware_integration/mcp2515_can_interface.hpp"

MCP2515CANInterface::MCP2515CANInterface(SPITransmissionCallback callback, const std::uint8_t cfg1, const std::uint8_t cfg2, const std::uint8_t cfg3) :
  callback(callback),
  cfg1(cfg1),
  cfg2(cfg2),
  cfg3(cfg3) {}

MCP2515CANInterface::~MCP2515CANInterface()
{
	close();
}

bool MCP2515CANInterface::get_is_valid() const
{
	return (callback != nullptr) && (initialized);
}

void MCP2515CANInterface::close()
{
	initialized = false;
}

void MCP2515CANInterface::open()
{
	if (callback != nullptr)
	{
		if (set_mode(MCPMode::CONFIG))
		{
			if (write_register(MCPRegister::CNF1, cfg1) &&
			    write_register(MCPRegister::CNF2, cfg2) &&
			    write_register(MCPRegister::CNF3, cfg3))
			{
				initialized = true;
			}
		}
	}
}

bool MCP2515CANInterface::reset()
{
	bool retVal = false;
	if (write_instruction(MCPInstruction::RESET, 1))
	{
		// Depends on oscillator & capacitors used
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		uint8_t zeros[14] = { 0 };
		if (write_register(MCPRegister::TXB0CTRL, zeros, 14) &&
		    write_register(MCPRegister::TXB1CTRL, zeros, 14) &&
		    write_register(MCPRegister::TXB2CTRL, zeros, 14) &&
		    write_register(MCPRegister::RXB0CTRL, 0) &&
		    write_register(MCPRegister::RXB1CTRL, 0) &&
		    write_register(MCPRegister::CANINTE, 0xA3))
		{
			retVal = true;
		}
		return retVal;
	}
}

std::uint8_t MCP2515CANInterface::get_rx_status() const
{
	std::uint8_t retVal = 0;
	if (get_is_valid())
	{
		const std::vector<std::uint8_t> txData = { static_cast<std::uint8_t>(MCPInstruction::RX_STATUS), 0x00 };
		std::vector<std::uint8_t> response;
		if (callback(txData, response) && response.size() == 2)
		{
			retVal = response[1];
		}
	}
	return retVal;
}
bool MCP2515CANInterface::read_register(const MCPRegister address, std::uint8_t &data)
{
	bool retVal = false;
	std::uint8_t rxData[1];
	if (read_register(address, rxData, 1))
	{
		data = rxData[0];
		retVal = true;
	}
	return retVal;
}

bool MCP2515CANInterface::read_register(const MCPRegister address, std::uint8_t data[], const std::size_t length)
{
	bool retVal = false;
	if (get_is_valid())
	{
		std::vector<std::uint8_t> txData = { static_cast<std::uint8_t>(MCPInstruction::READ), static_cast<uint8_t>(address) };
		txData.resize(length + 2);
		std::vector<std::uint8_t> rxData;

		if (callback(txData, rxData) && rxData.size() == length + 2)
		{
			for (std::size_t i = 2; i < length + 2; i++)
			{
				data[i - 2] = rxData[i];
			}
			retVal = true;
		}
	}
	return retVal;
}
bool MCP2515CANInterface::write_instruction(const MCPInstruction instruction, const std::uint8_t data)
{
	bool retVal = false;
	if (write_instruction(instruction, &data, 1))
	{
		retVal = true;
	}
	return retVal;
}

bool MCP2515CANInterface::write_instruction(const MCPInstruction instruction, const std::uint8_t data[], const std::size_t length)
{
	bool retVal = false;
	if (get_is_valid())
	{
		std::vector<std::uint8_t> txData = { static_cast<std::uint8_t>(instruction) };
		txData.insert(txData.end(), data, data + length);
		std::vector<std::uint8_t> rxData;
		if (callback(txData, rxData) && rxData.size() == length + 1)
		{
			retVal = true;
		}
	}
	return retVal;
}

bool MCP2515CANInterface::modify_register(const MCPRegister address, const std::uint8_t mask, const std::uint8_t data)
{
	bool retVal = false;
	std::uint8_t txData[] = { static_cast<uint8_t>(address), mask, data };
	if (write_instruction(MCPInstruction::BITMOD, txData, 3))
	{
		retVal = true;
	}
	return retVal;
}

bool MCP2515CANInterface::write_register(const MCPRegister address, const std::uint8_t data)
{
	bool retVal = false;
	if (write_register(address, &data, 1))
	{
		retVal = true;
	}
	return retVal;
}

bool MCP2515CANInterface::write_register(const MCPRegister address, const std::uint8_t data[], const std::size_t length)
{
	bool retVal = false;
	if (write_instruction(MCPInstruction::WRITE, (static_cast<uint8_t>(address), data), length + 1))
	{
		retVal = true;
	}
	// if (get_is_valid())
	// {
	// 	std::vector<std::uint8_t> txData = { static_cast<std::uint8_t>(MCPInstruction::WRITE), static_cast<uint8_t>(address) };
	// 	txData.insert(txData.end(), data, data + length);
	// 	std::vector<std::uint8_t> rxData;
	// 	if (callback(txData, rxData) && rxData.size() == length + 2)
	// 	{
	// 		retVal = true;
	// 	}
	// }
	return retVal;
}

bool MCP2515CANInterface::set_mode(const MCPMode mode)
{
	bool retVal = false;
	if (modify_register(MCPRegister::CANCTRL, 1, static_cast<std::uint8_t>(mode)))
	{
		// Wait for the mode to be set within 10ms
		const auto start = std::chrono::steady_clock::now();
		while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < 10)
		{
			std::uint8_t newMode;
			if (read_register(MCPRegister::CANSTAT, newMode) && (newMode & 0xE0) == static_cast<std::uint8_t>(mode))
			{
				retVal = true;
				break;
			}
		}
	}
	return retVal;
}

bool MCP2515CANInterface::read_frame(isobus::HardwareInterfaceCANFrame &canFrame)
{
	bool retVal = false;
	uint8_t status = get_rx_status();

	// Check if any of the buffers have a message
	const struct RXBRegister *rxb;
	if (status & 0x01)
	{
		rxb = &RXB[0];
	}
	else if (status & 0x02)
	{
		rxb = &RXB[1];
	}

	if (rxb != nullptr)
	{
		std::uint8_t rxData[6];
		if (read_register(rxb->ctrl, rxData, 6))
		{
			uint32_t id = (rxData[1] << 3) + (rxData[2] >> 5);

			if (0x08 == (rxData[2] & 0x08))
			{
				id = (id << 2) + (rxData[2] & 0x03);
				id = (id << 8) + rxData[3];
				id = (id << 8) + rxData[4];
				id |= CAN_EFF_FLAG;
			}

			uint8_t ctrl = rxData[0];
			if (ctrl & 0x08)
			{
				id |= CAN_RTR_FLAG;
			}

			uint8_t len = (rxData[5] & 0x0F);
			if (len <= CAN_MAX_DLEN)
			{
				retVal = true;

				uint8_t data[CAN_MAX_DLEN];
				if (read_register(rxb->data, data, len) &&
				    modify_register(MCPRegister::CANINTF, rxb->intf, 0) &&
				    0 == (id & CAN_ERR_FLAG))
				{
					if (0 != (id & CAN_EFF_FLAG))
					{
						canFrame.identifier = (id & CAN_EFF_MASK);
						canFrame.isExtendedFrame = true;
					}
					else
					{
						canFrame.identifier = (id & CAN_SFF_MASK);
						canFrame.isExtendedFrame = false;
					}
					canFrame.dataLength = len;
					memset(canFrame.data, 0, sizeof(canFrame.data));
					memcpy(canFrame.data, data, canFrame.dataLength);

					// Timestamp is not supported by MCP2515

					retVal = true;
				}
			}
		}
	}

	return retVal;
}

bool MCP2515CANInterface::write_frame(const isobus::HardwareInterfaceCANFrame &canFrame)
{
	bool retVal = false;

	for (int i = 0; i < NUM_WRITE_BUFFERS; i++)
	{
		const struct TXBRegister *txbuf = &TXB[i];
		uint8_t ctrl;
		if (read_register(txbuf->ctrl, ctrl) &&
		    ctrl & 0x08)
		{
			// Buffer can be written to
			uint8_t data[13];

			if (canFrame.isExtendedFrame)
			{
				data[3] = static_cast<uint8_t>(canFrame.identifier & 0xFF); //< EID0
				data[2] = static_cast<uint8_t>((canFrame.identifier >> 8) & 0xFF); //< EID8
				data[1] = static_cast<uint8_t>((canFrame.identifier >> 16) & 0x03); //< SIDL EID17-16
				data[1] |= static_cast<uint8_t>((canFrame.identifier >> 13) & 0xE0); //< SIDL SID0-2
				data[1] |= 0x08; //< SIDL exide mask
				data[0] = static_cast<uint8_t>((canFrame.identifier >> 21) & 0xFF); //< SIDH
			}
			else
			{
				data[1] = static_cast<uint8_t>((canFrame.identifier << 5) & 0xE0); // SIDL
				data[0] = static_cast<uint8_t>(canFrame.identifier >> 3); // SIDH
			}

			data[4] = canFrame.dataLength; //< DLC
			memcpy(&data[5], canFrame.data, canFrame.dataLength); //< Data

			if ((write_register(txbuf->sidh, data, 5 + canFrame.dataLength)) &&
			    (modify_register(txbuf->ctrl, 0x08, 0x08)) &&
			    (read_register(txbuf->ctrl, ctrl)) &&
			    ((ctrl & (0x40 | 0x20 | 0x10)) == 0))
			{
				retVal = true;
			}
		}
	}

	return retVal;
}
