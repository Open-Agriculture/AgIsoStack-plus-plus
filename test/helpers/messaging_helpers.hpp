#ifndef MESSAGING_HELPERS_HPP
#define MESSAGING_HELPERS_HPP

#include "isobus/isobus/can_message.hpp"
#include "isobus/isobus/can_message_frame.hpp"

namespace test_helpers
{
	std::uint32_t create_ext_can_id(std::uint8_t priority,
	                                std::uint32_t parameterGroupNumber,
	                                std::shared_ptr<isobus::ControlFunction> destination,
	                                std::shared_ptr<isobus::ControlFunction> source);

	std::uint32_t create_ext_can_id_broadcast(std::uint8_t priority,
	                                          std::uint32_t parameterGroupNumber,
	                                          std::shared_ptr<isobus::ControlFunction> source);

	isobus::CANMessage create_message(std::uint8_t priority,
	                                  std::uint32_t parameterGroupNumber,
	                                  std::shared_ptr<isobus::ControlFunction> destination,
	                                  std::shared_ptr<isobus::ControlFunction> source,
	                                  std::initializer_list<std::uint8_t> data);

	isobus::CANMessage create_message(std::uint8_t priority,
	                                  std::uint32_t parameterGroupNumber,
	                                  std::shared_ptr<isobus::ControlFunction> destination,
	                                  std::shared_ptr<isobus::ControlFunction> source,
	                                  const std::uint8_t *dataBuffer,
	                                  std::uint32_t dataLength);

	isobus::CANMessage create_message_broadcast(std::uint8_t priority,
	                                            std::uint32_t parameterGroupNumber,
	                                            std::shared_ptr<isobus::ControlFunction> source,
	                                            std::initializer_list<std::uint8_t> data);

	isobus::CANMessage create_message_broadcast(std::uint8_t priority,
	                                            std::uint32_t parameterGroupNumber,
	                                            std::shared_ptr<isobus::ControlFunction> source,
	                                            const std::uint8_t *dataBuffer,
	                                            std::uint32_t dataLength);

	isobus::CANMessageFrame create_message_frame_raw(std::uint32_t identifier,
	                                                 std::initializer_list<std::uint8_t> data);

	isobus::CANMessageFrame create_message_frame(std::uint8_t priority,
	                                             std::uint32_t parameterGroupNumber,
	                                             std::shared_ptr<isobus::ControlFunction> destination,
	                                             std::shared_ptr<isobus::ControlFunction> source,
	                                             std::initializer_list<std::uint8_t> data);

	isobus::CANMessageFrame create_message_frame_broadcast(std::uint8_t priority,
	                                                       std::uint32_t parameterGroupNumber,
	                                                       std::shared_ptr<isobus::ControlFunction> source,
	                                                       std::initializer_list<std::uint8_t> data);

	isobus::CANMessageFrame create_message_frame_pgn_request(std::uint32_t requestedParameterGroupNumber,
	                                                         std::shared_ptr<isobus::ControlFunction> source,
	                                                         std::shared_ptr<isobus::ControlFunction> destination);

} // namespace test_helpers
#endif // MESSAGING_HELPERS_HPP
