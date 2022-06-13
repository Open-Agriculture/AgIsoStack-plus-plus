#pragma once

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