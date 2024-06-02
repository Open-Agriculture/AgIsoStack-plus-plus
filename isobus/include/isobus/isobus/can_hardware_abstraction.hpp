//================================================================================================
/// @file can_hardware_abstraction.hpp
///
/// @brief An abstraction between this CAN stack and any hardware layer
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================

#ifndef CAN_HARDWARE_ABSTRACTION_HPP
#define CAN_HARDWARE_ABSTRACTION_HPP

#include "isobus/isobus/can_message_frame.hpp"

#include <cstdint>

namespace isobus
{
	/// @brief The sending abstraction layer between the hardware and the stack
	/// @param[in] frame The frame to transmit from the hardware
	/// @returns true if the frame was successfully sent, false otherwise
	bool send_can_message_frame_to_hardware(const CANMessageFrame &frame);

	/// @brief The receiving abstraction layer between the hardware and the stack
	/// @param[in] frame The frame to receive from the hardware
	void receive_can_message_frame_from_hardware(const CANMessageFrame &frame);

	/// @brief Informs the network manager whenever messages are emitted on the bus
	/// @param[in] txFrame The CAN frame that was just emitted
	void on_transmit_can_message_frame_from_hardware(const CANMessageFrame &txFrame);

	/// @brief The periodic update abstraction layer between the hardware and the stack
	void periodic_update_from_hardware();

} // namespace isobus

#endif // CAN_HARDWARE_ABSTRACTION_HPP
