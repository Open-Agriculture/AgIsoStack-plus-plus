//================================================================================================
/// @file can_frame.hpp
///
/// @brief A classical CAN frame, with 8 data bytes
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_FRAME_HPP
#define CAN_FRAME_HPP

#include <cstdint>

namespace isobus
{

class HardwareInterfaceCANFrame
{
public:
  std::uint64_t timestamp_us;
  std::uint32_t identifier;
  std::uint8_t channel;
  std::uint8_t data[8];
  std::uint8_t dataLength;
  bool isExtendedFrame;
};

} // namespace isobus

#endif // CAN_FRAME_HPP