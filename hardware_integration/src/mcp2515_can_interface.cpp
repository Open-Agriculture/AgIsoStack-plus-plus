//================================================================================================
/// @file mcp2515_can_interface.cpp
///
/// @brief An interface for using the MCP2515 can controller.
/// @author Daan Steenbergen
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus/hardware_integration/mcp2515_can_interface.hpp"
#include "isobus/utility/system_timing.hpp"

MCP2515CANInterface::MCP2515CANInterface(SPITransactionPlugin *transactionHandler, const std::uint8_t cfg1, const std::uint8_t cfg2, const std::uint8_t cfg3) :
  transactionHandler(transactionHandler),
  cfg1(cfg1),
  cfg2(cfg2),
  cfg3(cfg3)
{
}

MCP2515CANInterface::~MCP2515CANInterface()
{
	close();
}

bool MCP2515CANInterface::get_is_valid() const
{
	return (transactionHandler) && (initialized);
}

void MCP2515CANInterface::close()
{
	initialized = false;
}

void MCP2515CANInterface::open()
{
	if (reset())
	{
		if (set_mode(MCPMode::CONFIG))
		{
			if ((write_register(MCPRegister::CNF1, cfg1)) &&
			    (write_register(MCPRegister::CNF2, cfg2)) &&
			    (write_register(MCPRegister::CNF3, cfg3)))
			{
				if (set_mode(MCPMode::NORMAL))
				{
					initialized = true;
				}
			}
		}
	}
}

bool MCP2515CANInterface::reset()
{
	bool retVal = false;
	if (write_reset())
	{
		// Depends on oscillator & capacitors used
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		std::uint8_t zeros[14] = { 0 };
		if ((write_register(MCPRegister::TXB0CTRL, zeros, 14)) &&
		    (write_register(MCPRegister::TXB1CTRL, zeros, 14)) &&
		    (write_register(MCPRegister::TXB2CTRL, zeros, 14)) &&
		    (write_register(MCPRegister::RXB0CTRL, 0)) &&
		    (write_register(MCPRegister::RXB1CTRL, 0)) &&
		    (write_register(MCPRegister::CANINTE, 0xA3)))
		{
			retVal = true;
		}
	}
	return retVal;
}

bool MCP2515CANInterface::get_rx_status(std::uint8_t &status)
{
	bool retVal = false;
	if (transactionHandler)
	{
		transactionHandler->begin_transaction();
		transactionHandler->write(static_cast<std::uint8_t>(MCPInstruction::RX_STATUS));
		status = transactionHandler->read_write(0x00);
		transactionHandler->end_transaction();
		retVal = transactionHandler->get_is_valid();
	}
	return retVal;
}
bool MCP2515CANInterface::read_register(const MCPRegister address, std::uint8_t &data)
{
	bool retVal = false;
	if (transactionHandler)
	{
		transactionHandler->begin_transaction();
		transactionHandler->write(static_cast<std::uint8_t>(MCPInstruction::READ));
		transactionHandler->write(static_cast<std::uint8_t>(address));
		data = transactionHandler->read_write(0x00);
		transactionHandler->end_transaction();
		retVal = transactionHandler->get_is_valid();
	}
	return retVal;
}

bool MCP2515CANInterface::read_register(const MCPRegister address, std::uint8_t data[], const std::size_t length)
{
	bool retVal = false;
	if (transactionHandler)
	{
		transactionHandler->begin_transaction();
		transactionHandler->write(static_cast<std::uint8_t>(MCPInstruction::READ));
		transactionHandler->write(static_cast<std::uint8_t>(address));
		for (std::size_t i = 0; i < length; i++)
		{
			data[i] = transactionHandler->read_write(0x00);
		}
		transactionHandler->end_transaction();
		retVal = transactionHandler->get_is_valid();
	}
	return retVal;
}

bool MCP2515CANInterface::modify_register(const MCPRegister address, const std::uint8_t mask, const std::uint8_t data)
{
	bool retVal = false;
	if (transactionHandler)
	{
		transactionHandler->begin_transaction();
		transactionHandler->write(static_cast<std::uint8_t>(MCPInstruction::BITMOD));
		transactionHandler->write(static_cast<std::uint8_t>(address));
		transactionHandler->write(mask);
		transactionHandler->write(data);
		transactionHandler->end_transaction();
		retVal = transactionHandler->get_is_valid();
	}
	return retVal;
}

bool MCP2515CANInterface::write_reset()
{
	bool retVal = false;
	if (transactionHandler)
	{
		transactionHandler->begin_transaction();
		transactionHandler->write(static_cast<std::uint8_t>(MCPInstruction::RESET));
		transactionHandler->end_transaction();
		retVal = transactionHandler->get_is_valid();
	}
	return retVal;
}

bool MCP2515CANInterface::write_register(const MCPRegister address, const std::uint8_t data)
{
	bool retVal = false;
	if (transactionHandler)
	{
		transactionHandler->begin_transaction();
		transactionHandler->write(static_cast<std::uint8_t>(MCPInstruction::WRITE));
		transactionHandler->write(static_cast<std::uint8_t>(address));
		transactionHandler->write(data);
		transactionHandler->end_transaction();
		retVal = transactionHandler->get_is_valid();
	}
	return retVal;
}

bool MCP2515CANInterface::write_register(const MCPRegister address, const std::uint8_t data[], const std::size_t length)
{
	bool retVal = false;
	if (transactionHandler)
	{
		transactionHandler->begin_transaction();
		transactionHandler->write(static_cast<std::uint8_t>(MCPInstruction::WRITE));
		transactionHandler->write(static_cast<std::uint8_t>(address));
		for (std::size_t i = 0; i < length; i++)
		{
			transactionHandler->write(data[i]);
		}
		transactionHandler->end_transaction();
		retVal = transactionHandler->get_is_valid();
	}
	return retVal;
}

bool MCP2515CANInterface::set_mode(const MCPMode mode)
{
	bool retVal = false;
	if (modify_register(MCPRegister::CANCTRL, 0xE0, static_cast<std::uint8_t>(mode)))
	{
		// Wait for the mode to be set within 10ms
		const auto start = isobus::SystemTiming::get_timestamp_ms();
		while (isobus::SystemTiming::get_time_elapsed_ms(start) < 10)
		{
			std::uint8_t newMode;
			if (read_register(MCPRegister::CANSTAT, newMode))
			{
				if ((newMode & 0xE0) == static_cast<std::uint8_t>(mode))
				{
					retVal = true;
					break;
				}
			}
		}
	}
	return retVal;
}

bool MCP2515CANInterface::read_frame(isobus::HardwareInterfaceCANFrame &canFrame)
{
	constexpr static std::array<RXBRegister, NUM_READ_BUFFERS> RXB = { { { MCPRegister::RXB0CTRL, MCPRegister::RXB0DATA, 0x01 },
		                                                                   { MCPRegister::RXB1CTRL, MCPRegister::RXB1DATA, 0x02 } } }; ///< The registers of the RX buffers
	bool retVal = false;

	// Wait for message to be available
	const struct RXBRegister *rxb;

	std::uint8_t status;
	while (get_rx_status(status))
	{
		// Check if any of the buffers have a message
		if (status & 0x01)
		{
			rxb = &RXB[0];
			break;
		}
		else if (status & 0x02)
		{
			rxb = &RXB[1];
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(RECEIVE_MESSAGE_READ_RATE));
	}

	if (rxb)
	{
		std::uint8_t rxData[8];
		if (read_register(rxb->ctrl, rxData, 6))
		{
			std::uint32_t id = (rxData[1] << 3) + (rxData[2] >> 5);

			if (0x08 == (rxData[2] & 0x08))
			{
				id = (id << 2) + (rxData[2] & 0x03);
				id = (id << 8) + rxData[3];
				id = (id << 8) + rxData[4];
				id |= CAN_EFF_FLAG;
			}

			std::uint8_t ctrl = rxData[0];
			if (ctrl & 0x08)
			{
				id |= CAN_RTR_FLAG;
			}

			std::uint8_t len = (rxData[5] & 0x0F);
			if (isobus::CAN_DATA_LENGTH >= len)
			{
				canFrame.dataLength = len;
				retVal = true;

				if ((read_register(rxb->data, rxData, 8)) &&
				    (modify_register(MCPRegister::CANINTF, rxb->intf, 0)) &&
				    (0 == (id & CAN_ERR_FLAG)))
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
					memset(canFrame.data, 0, sizeof(canFrame.data));
					memcpy(canFrame.data, rxData, canFrame.dataLength);

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
	// The register addresses for the TX buffers
	constexpr static std::array<TXBRegister, NUM_WRITE_BUFFERS> TXB = { { { MCPRegister::TXB0CTRL, MCPRegister::TXB0SIDH, MCPRegister::TXB0DATA },
		                                                                    { MCPRegister::TXB1CTRL, MCPRegister::TXB1SIDH, MCPRegister::TXB1DATA },
		                                                                    { MCPRegister::TXB2CTRL, MCPRegister::TXB2SIDH, MCPRegister::TXB2DATA } } };

	bool retVal = false;

	for (std::size_t i = 0; i < NUM_WRITE_BUFFERS; i++)
	{
		const struct TXBRegister *txbuf = &TXB[i];
		std::uint8_t ctrl;
		if (read_register(txbuf->ctrl, ctrl) &&
		    ctrl & 0x08)
		{
			// Buffer can be written to
			std::uint8_t data[13] = { 0 };

			if (canFrame.isExtendedFrame)
			{
				data[3] = static_cast<std::uint8_t>(canFrame.identifier & 0xFF); //< EID0
				data[2] = static_cast<std::uint8_t>((canFrame.identifier >> 8) & 0xFF); //< EID8
				data[1] = static_cast<std::uint8_t>((canFrame.identifier >> 16) & 0x03); //< SIDL EID17-16
				data[1] |= static_cast<std::uint8_t>((canFrame.identifier >> 13) & 0xE0); //< SIDL SID0-2
				data[1] |= 0x08; //< SIDL exide mask
				data[0] = static_cast<std::uint8_t>((canFrame.identifier >> 21) & 0xFF); //< SIDH
			}
			else
			{
				data[1] = static_cast<std::uint8_t>((canFrame.identifier << 5) & 0xE0); // SIDL
				data[0] = static_cast<std::uint8_t>(canFrame.identifier >> 3); // SIDH
			}

			data[4] = canFrame.dataLength; //< DLC
			memcpy(&data[5], canFrame.data, canFrame.dataLength); //< Data

			if ((write_register(txbuf->sidh, data, 13)) &&
			    (modify_register(txbuf->ctrl, 0x08, 0x08)) &&
			    (read_register(txbuf->ctrl, ctrl)) &&
			    (0 == (ctrl & (0x40 | 0x20 | 0x10))))
			{
				retVal = true;
			}
		}
	}

	return retVal;
}
