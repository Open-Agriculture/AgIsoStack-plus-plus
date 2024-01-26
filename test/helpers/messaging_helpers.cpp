#include "messaging_helpers.hpp"
#include <gtest/gtest.h>

namespace test_helpers
{
	using namespace isobus;

	std::uint32_t create_ext_can_id(std::uint8_t priority,
	                                std::uint32_t parameterGroupNumber,
	                                std::shared_ptr<ControlFunction> destination,
	                                std::shared_ptr<ControlFunction> source)
	{
		std::uint32_t identifier = 0;

		EXPECT_NE(source, nullptr);
		EXPECT_TRUE(source->get_address_valid());
		EXPECT_NE(destination, nullptr);
		EXPECT_TRUE(destination->get_address_valid());

		identifier |= (static_cast<std::uint32_t>(priority) & 0x07) << 26;
		identifier |= source->get_address();

		// Bounds check the parameter group number (PDU1 format, DA=specific)
		EXPECT_TRUE(parameterGroupNumber < 0x3FFFF); // max 18 bits
		EXPECT_LT((parameterGroupNumber & 0xFF00), (240 << 8)); // PDU1 format
		EXPECT_EQ((parameterGroupNumber & 0xFF), 0); // PDU1 format

		identifier |= (parameterGroupNumber & 0x3FF00) << 8;
		identifier |= destination->get_address() << 8;

		return identifier;
	}

	std::uint32_t create_ext_can_id_broadcast(std::uint8_t priority,
	                                          std::uint32_t parameterGroupNumber,
	                                          std::shared_ptr<ControlFunction> source)
	{
		std::uint32_t identifier = 0;

		EXPECT_NE(source, nullptr);
		EXPECT_TRUE(source->get_address_valid());

		identifier |= (static_cast<std::uint32_t>(priority) & 0x07) << 26;
		identifier |= source->get_address();

		// Bounds check the parameter group number
		EXPECT_TRUE(parameterGroupNumber < 0x3FFFF); // max 18 bits

		if ((parameterGroupNumber & 0xFF00) < (240 << 8))
		{
			// PDU1 format, DA=broadcast
			EXPECT_EQ((parameterGroupNumber & 0xFF), 0);
			identifier |= (parameterGroupNumber & 0x3FF00) << 8;
			identifier |= 0xFF << 8;
		}
		else
		{
			// PDU2 format
			identifier |= (parameterGroupNumber & 0x3FFFF) << 8;
		}

		return identifier;
	}

	isobus::CANMessage create_message(std::uint8_t priority, std::uint32_t parameterGroupNumber, std::shared_ptr<isobus::ControlFunction> destination, std::shared_ptr<isobus::ControlFunction> source, std::initializer_list<std::uint8_t> data)
	{
		return create_message(priority, parameterGroupNumber, destination, source, data.begin(), data.size());
	}

	CANMessage create_message(std::uint8_t priority,
	                          std::uint32_t parameterGroupNumber,
	                          std::shared_ptr<ControlFunction> destination,
	                          std::shared_ptr<ControlFunction> source,
	                          const std::uint8_t *dataBuffer,
	                          std::uint32_t dataLength)
	{
		EXPECT_NE(source, nullptr);
		EXPECT_TRUE(source->get_address_valid());
		EXPECT_NE(destination, nullptr);
		EXPECT_TRUE(destination->get_address_valid());

		CANIdentifier identifier(create_ext_can_id(priority, parameterGroupNumber, destination, source));
		CANMessage message(CANMessage::Type::Receive,
		                   identifier,
		                   dataBuffer,
		                   dataLength,
		                   source,
		                   destination,
		                   0); //! TODO: hack for now, will be fixed when we remove CANNetwork Singleton
		return message;
	}

	isobus::CANMessage create_message_broadcast(std::uint8_t priority, std::uint32_t parameterGroupNumber, std::shared_ptr<isobus::ControlFunction> source, std::initializer_list<std::uint8_t> data)
	{
		return create_message_broadcast(priority, parameterGroupNumber, source, data.begin(), data.size());
	}

	CANMessage create_message_broadcast(std::uint8_t priority,
	                                    std::uint32_t parameterGroupNumber,
	                                    std::shared_ptr<ControlFunction> source,
	                                    const std::uint8_t *dataBuffer,
	                                    std::uint32_t dataLength)
	{
		EXPECT_NE(source, nullptr);
		EXPECT_TRUE(source->get_address_valid());

		CANIdentifier identifier(create_ext_can_id_broadcast(priority, parameterGroupNumber, source));
		CANMessage message(CANMessage::Type::Receive,
		                   identifier,
		                   dataBuffer,
		                   dataLength,
		                   source,
		                   nullptr,
		                   0); //! TODO: hack for now, will be fixed when we remove CANNetwork Singleton
		return message;
	}

	CANMessageFrame create_message_frame_raw(std::uint32_t identifier,
	                                         std::initializer_list<std::uint8_t> data)

	{
		EXPECT_TRUE(data.size() <= 8);

		CANMessageFrame frame = {};
		frame.channel = 0; //! TODO: hack for now, will be fixed when we remove CANNetwork Singleton
		frame.identifier = identifier;
		frame.isExtendedFrame = true;
		frame.dataLength = data.size();
		std::copy(data.begin(), data.end(), frame.data);
		return frame;
	}

	CANMessageFrame create_message_frame(std::uint8_t priority,
	                                     std::uint32_t parameterGroupNumber,
	                                     std::shared_ptr<ControlFunction> destination,
	                                     std::shared_ptr<ControlFunction> source,
	                                     std::initializer_list<std::uint8_t> data)
	{
		EXPECT_NE(source, nullptr);
		EXPECT_TRUE(source->get_address_valid());
		EXPECT_NE(destination, nullptr);
		EXPECT_TRUE(destination->get_address_valid());
		EXPECT_TRUE(data.size() <= 8);

		return create_message_frame_raw(create_ext_can_id(priority, parameterGroupNumber, destination, source), data);
	}

	CANMessageFrame create_message_frame_broadcast(std::uint8_t priority,
	                                               std::uint32_t parameterGroupNumber,
	                                               std::shared_ptr<ControlFunction> source,
	                                               std::initializer_list<std::uint8_t> data)
	{
		EXPECT_NE(source, nullptr);
		EXPECT_TRUE(source->get_address_valid());
		EXPECT_TRUE(data.size() <= 8);

		return create_message_frame_raw(create_ext_can_id_broadcast(priority, parameterGroupNumber, source), data);
	}

	CANMessageFrame create_message_frame_pgn_request(std::uint32_t requestedParameterGroupNumber,
	                                                 std::shared_ptr<ControlFunction> source,
	                                                 std::shared_ptr<ControlFunction> destination)
	{
		std::uint32_t identifier = 0;
		if (destination != nullptr)
		{
			EXPECT_TRUE(destination->get_address_valid());
			EXPECT_TRUE(source->get_address_valid()); // The receiver must have an address to respond to
			identifier = create_ext_can_id(6, 0xEA00, destination, source);
		}
		else if (source != nullptr)
		{
			EXPECT_TRUE(source->get_address_valid());
			identifier = create_ext_can_id_broadcast(6, 0xEA00, source);
		}
		else
		{
			identifier = 0x18EAFFFE; // PGN request broadcast from NULL address
		}

		return create_message_frame_raw(
		  identifier,
		  {
		    static_cast<std::uint8_t>(requestedParameterGroupNumber),
		    static_cast<std::uint8_t>(requestedParameterGroupNumber >> 8),
		    static_cast<std::uint8_t>(requestedParameterGroupNumber >> 16),
		  });
	}

} // namespace test_helpers
