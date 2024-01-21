#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_task_controller_server.hpp"
#include "isobus/utility/system_timing.hpp"

#include "helpers/control_function_helpers.hpp"
#include "helpers/messaging_helpers.hpp"

using namespace isobus;

// clang-format off
// array size is 2356
// This is the binary version of the example program's DDOP when it gets serialized
constexpr uint8_t testDDOP[]  = {
  0x44, 0x56, 0x43, 0x00, 0x00, 0x11, 0x49, 0x73, 0x6f, 0x62, 0x75, 0x73, 0x2b, 0x2b, 0x20, 0x55, 
  0x6e, 0x69, 0x74, 0x54, 0x65, 0x73, 0x74, 0x05, 0x31, 0x2e, 0x30, 0x2e, 0x30, 0x02, 0x00, 0x00, 
  0x08, 0x00, 0x80, 0x0c, 0xa0, 0x03, 0x31, 0x32, 0x33, 0x49, 0x2b, 0x2b, 0x31, 0x2e, 0x30, 0x20, 
  0x65, 0x6e, 0x50, 0x00, 0x55, 0x55, 0xff, 0x44, 0x45, 0x54, 0x01, 0x00, 0x01, 0x07, 0x53, 0x70, 
  0x72, 0x61, 0x79, 0x65, 0x72, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x04, 0x00, 0x44, 
  0x50, 0x44, 0x02, 0x00, 0x8d, 0x00, 0x01, 0x08, 0x11, 0x41, 0x63, 0x74, 0x75, 0x61, 0x6c, 0x20, 
  0x57, 0x6f, 0x72, 0x6b, 0x20, 0x53, 0x74, 0x61, 0x74, 0x65, 0xff, 0xff, 0x44, 0x50, 0x44, 0x03, 
  0x00, 0x03, 0x00, 0x00, 0x10, 0x12, 0x52, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x20, 0x44, 0x65, 
  0x66, 0x61, 0x75, 0x6c, 0x74, 0x20, 0x50, 0x44, 0xff, 0xff, 0x44, 0x50, 0x44, 0x04, 0x00, 0x77, 
  0x00, 0x03, 0x10, 0x0a, 0x54, 0x6f, 0x74, 0x61, 0x6c, 0x20, 0x54, 0x69, 0x6d, 0x65, 0x3b, 0x04, 
  0x44, 0x45, 0x54, 0x05, 0x00, 0x06, 0x09, 0x43, 0x6f, 0x6e, 0x6e, 0x65, 0x63, 0x74, 0x6f, 0x72, 
  0x01, 0x00, 0x01, 0x00, 0x03, 0x00, 0x06, 0x00, 0x07, 0x00, 0x08, 0x00, 0x44, 0x50, 0x44, 0x06, 
  0x00, 0x86, 0x00, 0x02, 0x00, 0x0b, 0x43, 0x6f, 0x6e, 0x6e, 0x65, 0x63, 0x74, 0x6f, 0x72, 0x20, 
  0x58, 0x3c, 0x04, 0x44, 0x50, 0x44, 0x07, 0x00, 0x87, 0x00, 0x02, 0x00, 0x0b, 0x43, 0x6f, 0x6e, 
  0x6e, 0x65, 0x63, 0x74, 0x6f, 0x72, 0x20, 0x59, 0x3c, 0x04, 0x44, 0x50, 0x54, 0x08, 0x00, 0x9d, 
  0x00, 0x09, 0x00, 0x00, 0x00, 0x04, 0x54, 0x79, 0x70, 0x65, 0xff, 0xff, 0x44, 0x45, 0x54, 0x09, 
  0x00, 0x02, 0x04, 0x42, 0x6f, 0x6f, 0x6d, 0x02, 0x00, 0x01, 0x00, 0x17, 0x00, 0x0f, 0x00, 0x10, 
  0x00, 0x11, 0x00, 0x0b, 0x00, 0x0e, 0x00, 0x12, 0x04, 0x22, 0x04, 0x12, 0x00, 0x13, 0x00, 0x14, 
  0x00, 0x15, 0x00, 0x16, 0x00, 0x17, 0x00, 0x18, 0x00, 0x19, 0x00, 0x1a, 0x00, 0x1b, 0x00, 0x1c, 
  0x00, 0x1d, 0x00, 0x1e, 0x00, 0x1f, 0x00, 0x20, 0x00, 0x21, 0x00, 0x44, 0x50, 0x54, 0x0f, 0x00, 
  0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3c, 
  0x04, 0x44, 0x50, 0x54, 0x10, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x4f, 0x66, 0x66, 
  0x73, 0x65, 0x74, 0x20, 0x59, 0x3c, 0x04, 0x44, 0x50, 0x54, 0x11, 0x00, 0x88, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x5a, 0x3c, 0x04, 0x44, 0x50, 0x44, 
  0x0b, 0x00, 0x43, 0x00, 0x01, 0x08, 0x14, 0x41, 0x63, 0x74, 0x75, 0x61, 0x6c, 0x20, 0x57, 0x6f, 
  0x72, 0x6b, 0x69, 0x6e, 0x67, 0x20, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x50, 0x44, 
  0x0d, 0x00, 0x21, 0x01, 0x03, 0x08, 0x13, 0x53, 0x65, 0x74, 0x70, 0x6f, 0x69, 0x6e, 0x74, 0x20, 
  0x57, 0x6f, 0x72, 0x6b, 0x20, 0x53, 0x74, 0x61, 0x74, 0x65, 0xff, 0xff, 0x44, 0x50, 0x44, 0x0c, 
  0x00, 0x74, 0x00, 0x01, 0x10, 0x0a, 0x41, 0x72, 0x65, 0x61, 0x20, 0x54, 0x6f, 0x74, 0x61, 0x6c, 
  0x3a, 0x04, 0x44, 0x50, 0x44, 0x0e, 0x00, 0xa0, 0x00, 0x03, 0x09, 0x15, 0x53, 0x65, 0x63, 0x74, 
  0x69, 0x6f, 0x6e, 0x20, 0x43, 0x6f, 0x6e, 0x74, 0x72, 0x6f, 0x6c, 0x20, 0x53, 0x74, 0x61, 0x74, 
  0x65, 0xff, 0xff, 0x44, 0x45, 0x54, 0x32, 0x04, 0x03, 0x07, 0x50, 0x72, 0x6f, 0x64, 0x75, 0x63, 
  0x74, 0x03, 0x00, 0x09, 0x00, 0x07, 0x00, 0x33, 0x04, 0x34, 0x04, 0x35, 0x04, 0x36, 0x04, 0x37, 
  0x04, 0x38, 0x04, 0x39, 0x04, 0x44, 0x50, 0x44, 0x33, 0x04, 0x49, 0x00, 0x01, 0x09, 0x0d, 0x54, 
  0x61, 0x6e, 0x6b, 0x20, 0x43, 0x61, 0x70, 0x61, 0x63, 0x69, 0x74, 0x79, 0x3e, 0x04, 0x44, 0x50, 
  0x44, 0x34, 0x04, 0x48, 0x00, 0x03, 0x09, 0x0b, 0x54, 0x61, 0x6e, 0x6b, 0x20, 0x56, 0x6f, 0x6c, 
  0x75, 0x6d, 0x65, 0x3e, 0x04, 0x44, 0x50, 0x44, 0x35, 0x04, 0x45, 0x01, 0x01, 0x10, 0x15, 0x4c, 
  0x69, 0x66, 0x65, 0x74, 0x69, 0x6d, 0x65, 0x20, 0x54, 0x6f, 0x74, 0x61, 0x6c, 0x20, 0x56, 0x6f, 
  0x6c, 0x75, 0x6d, 0x65, 0x3e, 0x04, 0x44, 0x50, 0x44, 0x36, 0x04, 0x9e, 0x00, 0x03, 0x09, 0x10, 
  0x52, 0x78, 0x20, 0x43, 0x6f, 0x6e, 0x74, 0x72, 0x6f, 0x6c, 0x20, 0x53, 0x74, 0x61, 0x74, 0x65, 
  0xff, 0xff, 0x44, 0x50, 0x44, 0x38, 0x04, 0x01, 0x00, 0x03, 0x08, 0x0b, 0x54, 0x61, 0x72, 0x67, 
  0x65, 0x74, 0x20, 0x52, 0x61, 0x74, 0x65, 0x3f, 0x04, 0x44, 0x50, 0x44, 0x39, 0x04, 0x02, 0x00, 
  0x01, 0x09, 0x0b, 0x41, 0x63, 0x74, 0x75, 0x61, 0x6c, 0x20, 0x52, 0x61, 0x74, 0x65, 0x3f, 0x04, 
  0x44, 0x50, 0x54, 0x37, 0x04, 0xb3, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0e, 0x4f, 0x70, 0x65, 0x72, 
  0x61, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x54, 0x79, 0x70, 0x65, 0xff, 0xff, 0x44, 0x45, 0x54, 0x12, 
  0x00, 0x04, 0x09, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x30, 0x04, 0x00, 0x09, 0x00, 
  0x03, 0x00, 0x12, 0x02, 0x12, 0x01, 0x12, 0x03, 0x44, 0x50, 0x54, 0x12, 0x01, 0x86, 0x00, 0xec, 
  0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 
  0x54, 0x12, 0x02, 0x87, 0x00, 0x07, 0xbd, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 
  0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x12, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 
  0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x13, 0x00, 0x04, 0x09, 0x53, 0x65, 
  0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x31, 0x05, 0x00, 0x09, 0x00, 0x03, 0x00, 0x13, 0x02, 0x13, 
  0x01, 0x13, 0x03, 0x44, 0x50, 0x54, 0x13, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 
  0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x13, 0x02, 0x87, 0x00, 
  0xf5, 0xc5, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 
  0x50, 0x54, 0x13, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 
  0x3d, 0x04, 0x44, 0x45, 0x54, 0x14, 0x00, 0x04, 0x09, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 
  0x20, 0x32, 0x06, 0x00, 0x09, 0x00, 0x03, 0x00, 0x14, 0x02, 0x14, 0x01, 0x14, 0x03, 0x44, 0x50, 
  0x54, 0x14, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 
  0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x14, 0x02, 0x87, 0x00, 0xe3, 0xce, 0xff, 0xff, 0x08, 
  0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x14, 0x03, 0x43, 
  0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 
  0x15, 0x00, 0x04, 0x09, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x33, 0x07, 0x00, 0x09, 
  0x00, 0x03, 0x00, 0x15, 0x02, 0x15, 0x01, 0x15, 0x03, 0x44, 0x50, 0x54, 0x15, 0x01, 0x86, 0x00, 
  0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 
  0x50, 0x54, 0x15, 0x02, 0x87, 0x00, 0xd1, 0xd7, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 
  0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x15, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 
  0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x16, 0x00, 0x04, 0x09, 0x53, 
  0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x34, 0x08, 0x00, 0x09, 0x00, 0x03, 0x00, 0x16, 0x02, 
  0x16, 0x01, 0x16, 0x03, 0x44, 0x50, 0x54, 0x16, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 
  0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x16, 0x02, 0x87, 
  0x00, 0xbf, 0xe0, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 
  0x44, 0x50, 0x54, 0x16, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 
  0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x17, 0x00, 0x04, 0x09, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 
  0x6e, 0x20, 0x35, 0x09, 0x00, 0x09, 0x00, 0x03, 0x00, 0x17, 0x02, 0x17, 0x01, 0x17, 0x03, 0x44, 
  0x50, 0x54, 0x17, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 
  0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x17, 0x02, 0x87, 0x00, 0xad, 0xe9, 0xff, 0xff, 
  0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x17, 0x03, 
  0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 
  0x54, 0x18, 0x00, 0x04, 0x09, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x36, 0x0a, 0x00, 
  0x09, 0x00, 0x03, 0x00, 0x18, 0x02, 0x18, 0x01, 0x18, 0x03, 0x44, 0x50, 0x54, 0x18, 0x01, 0x86, 
  0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 
  0x44, 0x50, 0x54, 0x18, 0x02, 0x87, 0x00, 0x9b, 0xf2, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 
  0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x18, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 
  0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x19, 0x00, 0x04, 0x09, 
  0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x37, 0x0b, 0x00, 0x09, 0x00, 0x03, 0x00, 0x19, 
  0x02, 0x19, 0x01, 0x19, 0x03, 0x44, 0x50, 0x54, 0x19, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 
  0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x19, 0x02, 
  0x87, 0x00, 0x89, 0xfb, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 
  0x04, 0x44, 0x50, 0x54, 0x19, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 
  0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x1a, 0x00, 0x04, 0x09, 0x53, 0x65, 0x63, 0x74, 0x69, 
  0x6f, 0x6e, 0x20, 0x38, 0x0c, 0x00, 0x09, 0x00, 0x03, 0x00, 0x1a, 0x02, 0x1a, 0x01, 0x1a, 0x03, 
  0x44, 0x50, 0x54, 0x1a, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 
  0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x1a, 0x02, 0x87, 0x00, 0x77, 0x04, 0x00, 
  0x00, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x1a, 
  0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 
  0x45, 0x54, 0x1b, 0x00, 0x04, 0x09, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x39, 0x0d, 
  0x00, 0x09, 0x00, 0x03, 0x00, 0x1b, 0x02, 0x1b, 0x01, 0x1b, 0x03, 0x44, 0x50, 0x54, 0x1b, 0x01, 
  0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 
  0x04, 0x44, 0x50, 0x54, 0x1b, 0x02, 0x87, 0x00, 0x65, 0x0d, 0x00, 0x00, 0x08, 0x4f, 0x66, 0x66, 
  0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x1b, 0x03, 0x43, 0x00, 0xee, 0x08, 
  0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x1c, 0x00, 0x04, 
  0x0a, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x31, 0x30, 0x0e, 0x00, 0x09, 0x00, 0x03, 
  0x00, 0x1c, 0x02, 0x1c, 0x01, 0x1c, 0x03, 0x44, 0x50, 0x54, 0x1c, 0x01, 0x86, 0x00, 0xec, 0xff, 
  0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 
  0x1c, 0x02, 0x87, 0x00, 0x53, 0x16, 0x00, 0x00, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 
  0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x1c, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 
  0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x1d, 0x00, 0x04, 0x0a, 0x53, 0x65, 0x63, 
  0x74, 0x69, 0x6f, 0x6e, 0x20, 0x31, 0x31, 0x0f, 0x00, 0x09, 0x00, 0x03, 0x00, 0x1d, 0x02, 0x1d, 
  0x01, 0x1d, 0x03, 0x44, 0x50, 0x54, 0x1d, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 
  0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x1d, 0x02, 0x87, 0x00, 
  0x41, 0x1f, 0x00, 0x00, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 
  0x50, 0x54, 0x1d, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 
  0x3d, 0x04, 0x44, 0x45, 0x54, 0x1e, 0x00, 0x04, 0x0a, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 
  0x20, 0x31, 0x32, 0x10, 0x00, 0x09, 0x00, 0x03, 0x00, 0x1e, 0x02, 0x1e, 0x01, 0x1e, 0x03, 0x44, 
  0x50, 0x54, 0x1e, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 
  0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x1e, 0x02, 0x87, 0x00, 0x2f, 0x28, 0x00, 0x00, 
  0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x1e, 0x03, 
  0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 
  0x54, 0x1f, 0x00, 0x04, 0x0a, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x31, 0x33, 0x11, 
  0x00, 0x09, 0x00, 0x03, 0x00, 0x1f, 0x02, 0x1f, 0x01, 0x1f, 0x03, 0x44, 0x50, 0x54, 0x1f, 0x01, 
  0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 
  0x04, 0x44, 0x50, 0x54, 0x1f, 0x02, 0x87, 0x00, 0x1d, 0x31, 0x00, 0x00, 0x08, 0x4f, 0x66, 0x66, 
  0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x1f, 0x03, 0x43, 0x00, 0xee, 0x08, 
  0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x20, 0x00, 0x04, 
  0x0a, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x31, 0x34, 0x12, 0x00, 0x09, 0x00, 0x03, 
  0x00, 0x20, 0x02, 0x20, 0x01, 0x20, 0x03, 0x44, 0x50, 0x54, 0x20, 0x01, 0x86, 0x00, 0xec, 0xff, 
  0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 
  0x20, 0x02, 0x87, 0x00, 0x0b, 0x3a, 0x00, 0x00, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 
  0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x20, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 
  0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x21, 0x00, 0x04, 0x0a, 0x53, 0x65, 0x63, 
  0x74, 0x69, 0x6f, 0x6e, 0x20, 0x31, 0x35, 0x13, 0x00, 0x09, 0x00, 0x03, 0x00, 0x21, 0x02, 0x21, 
  0x01, 0x21, 0x03, 0x44, 0x50, 0x54, 0x21, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 
  0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x21, 0x02, 0x87, 0x00, 
  0xf9, 0x42, 0x00, 0x00, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 
  0x50, 0x54, 0x21, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 
  0x3d, 0x04, 0x44, 0x50, 0x44, 0x12, 0x04, 0xa1, 0x00, 0x01, 0x08, 0x16, 0x41, 0x63, 0x74, 0x75, 
  0x61, 0x6c, 0x20, 0x57, 0x6f, 0x72, 0x6b, 0x20, 0x53, 0x74, 0x61, 0x74, 0x65, 0x20, 0x31, 0x2d, 
  0x31, 0x36, 0xff, 0xff, 0x44, 0x50, 0x44, 0x22, 0x04, 0x22, 0x01, 0x03, 0x08, 0x18, 0x53, 0x65, 
  0x74, 0x70, 0x6f, 0x69, 0x6e, 0x74, 0x20, 0x57, 0x6f, 0x72, 0x6b, 0x20, 0x53, 0x74, 0x61, 0x74, 
  0x65, 0x20, 0x31, 0x2d, 0x31, 0x36, 0xff, 0xff, 0x44, 0x56, 0x50, 0x3c, 0x04, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x02, 0x6d, 0x6d, 0x44, 0x56, 0x50, 0x3d, 0x04, 0x00, 0x00, 
  0x00, 0x00, 0x6f, 0x12, 0x83, 0x3a, 0x00, 0x01, 0x6d, 0x44, 0x56, 0x50, 0x3a, 0x04, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x03, 0x6d, 0x5e, 0x32, 0x44, 0x56, 0x50, 0x3e, 0x04, 
  0x00, 0x00, 0x00, 0x00, 0x6f, 0x12, 0x83, 0x3a, 0x00, 0x01, 0x4c, 0x44, 0x56, 0x50, 0x3b, 0x04, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x01, 0x07, 0x6d, 0x69, 0x6e, 0x75, 0x74, 0x65, 
  0x73, 0x44, 0x56, 0x50, 0x3f, 0x04, 0x00, 0x00, 0x00, 0x00, 0x6f, 0x12, 0x83, 0x3a, 0x01, 0x04, 
  0x4c, 0x2f, 0x68, 0x61
};

// clang-format on

class DerivedTcServer : public TaskControllerServer
{
public:
	DerivedTcServer(std::shared_ptr<InternalControlFunction> internalControlFunction,
	                std::uint8_t numberBoomsSupported,
	                std::uint8_t numberSectionsSupported,
	                std::uint8_t numberChannelsSupportedForPositionBasedControl,
	                std::uint8_t optionsBitfield) :
	  TaskControllerServer(internalControlFunction,
	                       numberBoomsSupported,
	                       numberSectionsSupported,
	                       numberChannelsSupportedForPositionBasedControl,
	                       optionsBitfield)
	{
	}

	bool activate_object_pool(std::shared_ptr<ControlFunction>, ObjectPoolActivationError &, ObjectPoolErrorCodes &, std::uint16_t &, std::uint16_t &) override
	{
		return true;
	}

	bool change_designator(std::shared_ptr<ControlFunction>, std::uint16_t, const std::vector<std::uint8_t> &)
	{
		return true;
	}

	bool deactivate_object_pool(std::shared_ptr<ControlFunction>)
	{
		return true;
	}

	bool delete_device_descriptor_object_pool(std::shared_ptr<ControlFunction>, ObjectPoolDeletionErrors &)
	{
		return true;
	}

	bool get_is_stored_device_descriptor_object_pool_by_structure_label(std::shared_ptr<ControlFunction>, const std::vector<std::uint8_t> &, const std::vector<std::uint8_t> &)
	{
		return !testStructureLabel.empty();
	}

	bool get_is_stored_device_descriptor_object_pool_by_localization_label(std::shared_ptr<ControlFunction>, const std::array<std::uint8_t, 7> &)
	{
		return 0 != testLocalizationLabel.at(0);
	}

	bool get_is_enough_memory_available(std::uint32_t)
	{
		return true;
	}

	std::uint32_t get_number_of_complete_object_pools_stored_for_client(std::shared_ptr<ControlFunction>)
	{
		return 0;
	}

	void identify_task_controller(std::uint8_t)
	{
	}

	void on_client_timeout(std::shared_ptr<ControlFunction>)
	{
	}

	void on_process_data_acknowledge(std::shared_ptr<ControlFunction>, std::uint16_t, std::uint16_t, std::uint8_t, ProcessDataCommands)
	{
	}

	bool on_value_command(std::shared_ptr<ControlFunction>, std::uint16_t, std::uint16_t, std::int32_t, std::uint8_t &)
	{
		return true;
	}

	bool store_device_descriptor_object_pool(std::shared_ptr<ControlFunction>, const std::vector<std::uint8_t> &, bool)
	{
		return true;
	}

	std::vector<std::uint8_t> testStructureLabel;
	std::array<std::uint8_t, 7> testLocalizationLabel = { 0 };
};

void isNack(const CANMessageFrame &frame)
{
	EXPECT_EQ(frame.identifier, 0x18E88887); // Priority 5, source 0x88, destination 0x87
	EXPECT_EQ(8, frame.dataLength);
	EXPECT_EQ(0x01, frame.data[0]);
	EXPECT_EQ(0xFF, frame.data[1]);
	EXPECT_EQ(0xFF, frame.data[2]);
	EXPECT_EQ(0xFF, frame.data[3]);
	EXPECT_EQ(0x88, frame.data[4]);
	EXPECT_EQ(0x00, frame.data[5]);
	EXPECT_EQ(0xCB, frame.data[6]);
	EXPECT_EQ(0x00, frame.data[7]);
}

void isPDNack(const CANMessageFrame &frame)
{
	EXPECT_EQ(frame.identifier, 0x10CB8887); // Priority 4
	EXPECT_EQ(static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::Acknowledge), (frame.data[0] & 0x0F));
}

bool readFrameFilterStatus(VirtualCANPlugin &plugin, CANMessageFrame &frame)
{
	bool retVal = plugin.read_frame(frame);

	if (frame.data[0] == 0xFE) // Filter out status messages
	{
		EXPECT_TRUE(retVal);
		retVal = plugin.read_frame(frame);
	}
	return retVal;
}

void testNackWrapper(VirtualCANPlugin &plugin,
                     DerivedTcServer &server,
                     CANMessageFrame &frame,
                     std::uint8_t mux,
                     std::shared_ptr<InternalControlFunction> icf,
                     std::shared_ptr<PartneredControlFunction> partner)
{
	CANNetworkManager::CANNetwork.receive_can_message(test_helpers::create_message(5,
	                                                                               0xCB00,
	                                                                               icf,
	                                                                               partner,
	                                                                               {
	                                                                                 mux,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,

	                                                                               }));
	CANNetworkManager::CANNetwork.update();
	server.update();
	EXPECT_TRUE(readFrameFilterStatus(plugin, frame));
	isNack(frame);
}

void testPDNackWrapper(VirtualCANPlugin &plugin,
                       DerivedTcServer &server,
                       CANMessageFrame &frame,
                       std::uint8_t mux,
                       std::shared_ptr<InternalControlFunction> icf,
                       std::shared_ptr<PartneredControlFunction> partner)
{
	CANNetworkManager::CANNetwork.receive_can_message(test_helpers::create_message(5,
	                                                                               0xCB00,
	                                                                               icf,
	                                                                               partner,
	                                                                               {
	                                                                                 mux,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,

	                                                                               }));
	CANNetworkManager::CANNetwork.update();
	server.update();
	EXPECT_TRUE(readFrameFilterStatus(plugin, frame));
	isPDNack(frame);
}

TEST(TASK_CONTROLLER_SERVER_TESTS, MessageEncoding)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	NAME clientNAME(0);
	clientNAME.set_industry_group(2);
	clientNAME.set_function_code(static_cast<std::uint8_t>(NAME::Function::TaskController));
	auto internalECU = test_helpers::claim_internal_control_function(0x87, 0);
	auto partnerClient = test_helpers::force_claim_partnered_control_function(0x88, 0);

	DerivedTcServer server(internalECU, 4, 255, 16, 0x17);
	server.initialize();

	testPlugin.clear_queue();

	// Test that the server responds to requests for version information
	CANNetworkManager::CANNetwork.receive_can_message(test_helpers::create_message(5,
	                                                                               0xCB00,
	                                                                               internalECU,
	                                                                               partnerClient,
	                                                                               {
	                                                                                 0x00,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                               }));
	CANNetworkManager::CANNetwork.update();
	server.update();
	CANMessageFrame testFrame = {};
	EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));
	EXPECT_EQ(testFrame.identifier, 0x14CB8887); // Priority 5, source 0x88, destination 0x87
	EXPECT_EQ(8, testFrame.dataLength);
	EXPECT_EQ(0x10, testFrame.data[0]);
	EXPECT_EQ(0x04, testFrame.data[1]); // version
	EXPECT_EQ(0xFF, testFrame.data[2]); // boot time
	EXPECT_EQ(0x17, testFrame.data[3]); // options
	EXPECT_EQ(0x00, testFrame.data[4]); // options 2 (reserved)
	EXPECT_EQ(0x04, testFrame.data[5]); // booms
	EXPECT_EQ(0xFF, testFrame.data[6]); // sections
	EXPECT_EQ(0x10, testFrame.data[7]); // channels

	// Test that the server also sent a version request to the client
	EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));
	EXPECT_EQ(testFrame.identifier, 0x14CB8887);
	EXPECT_EQ(0x00, testFrame.data[0]);
	EXPECT_EQ(0xFF, testFrame.data[1]);
	EXPECT_EQ(0xFF, testFrame.data[2]);
	EXPECT_EQ(0xFF, testFrame.data[3]);
	EXPECT_EQ(0xFF, testFrame.data[4]);
	EXPECT_EQ(0xFF, testFrame.data[5]);
	EXPECT_EQ(0xFF, testFrame.data[6]);
	EXPECT_EQ(0xFF, testFrame.data[7]);

	// Try to test all messages that the server should respond to with a NACK at this stage of connection
	testNackWrapper(testPlugin, server, testFrame, static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::DeviceDescriptor), internalECU, partnerClient); // request structure label
	testNackWrapper(testPlugin, server, testFrame, 0x20 | static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::DeviceDescriptor), internalECU, partnerClient); // request localization label
	testNackWrapper(testPlugin, server, testFrame, 0x80 | static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::DeviceDescriptor), internalECU, partnerClient); // activate pool
	testNackWrapper(testPlugin, server, testFrame, static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::Acknowledge), internalECU, partnerClient);
	testNackWrapper(testPlugin, server, testFrame, 0x0A, internalECU, partnerClient); // set and ack

	// Test PDNACKs
	testPDNackWrapper(testPlugin, server, testFrame, static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::MeasurementTimeInterval), internalECU, partnerClient);
	testPDNackWrapper(testPlugin, server, testFrame, static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::MeasurementDistanceInterval), internalECU, partnerClient);
	testPDNackWrapper(testPlugin, server, testFrame, static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::MeasurementMinimumWithinThreshold), internalECU, partnerClient);
	testPDNackWrapper(testPlugin, server, testFrame, static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::MeasurementMaximumWithinThreshold), internalECU, partnerClient);
	testPDNackWrapper(testPlugin, server, testFrame, static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::MeasurementChangeThreshold), internalECU, partnerClient);

	// Send working set master
	CANNetworkManager::CANNetwork.receive_can_message(test_helpers::create_message_broadcast(6,
	                                                                                         0xFE0D,
	                                                                                         partnerClient,
	                                                                                         {
	                                                                                           0x01,
	                                                                                           0xFF,
	                                                                                           0xFF,
	                                                                                           0xFF,
	                                                                                           0xFF,
	                                                                                           0xFF,
	                                                                                           0xFF,
	                                                                                           0xFF,
	                                                                                         }));
	CANNetworkManager::CANNetwork.update();
	server.update();

	// Request structure label
	CANNetworkManager::CANNetwork.receive_can_message(test_helpers::create_message(5,
	                                                                               0xCB00,
	                                                                               internalECU,
	                                                                               partnerClient,
	                                                                               {
	                                                                                 0x01,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                               }));
	CANNetworkManager::CANNetwork.update();
	server.update();
	EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));
	EXPECT_EQ(testFrame.identifier, 0x14CB8887); // Priority 5, source 0x88, destination 0x87
	EXPECT_EQ(8, testFrame.dataLength);
	EXPECT_EQ(0x11, testFrame.data[0]);
	EXPECT_EQ(0xFF, testFrame.data[1]);
	EXPECT_EQ(0xFF, testFrame.data[2]);
	EXPECT_EQ(0xFF, testFrame.data[3]);
	EXPECT_EQ(0xFF, testFrame.data[4]);
	EXPECT_EQ(0xFF, testFrame.data[5]);
	EXPECT_EQ(0xFF, testFrame.data[6]);
	EXPECT_EQ(0xFF, testFrame.data[7]);

	// Make sure a valid label is echoed back
	server.testStructureLabel = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
	CANNetworkManager::CANNetwork.receive_can_message(test_helpers::create_message(5,
	                                                                               0xCB00,
	                                                                               internalECU,
	                                                                               partnerClient,
	                                                                               {
	                                                                                 0x01,
	                                                                                 0x01,
	                                                                                 0x02,
	                                                                                 0x03,
	                                                                                 0x04,
	                                                                                 0x05,
	                                                                                 0x06,
	                                                                                 0x07,
	                                                                               }));
	CANNetworkManager::CANNetwork.update();
	server.update();
	EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));
	EXPECT_EQ(testFrame.identifier, 0x14CB8887); // Priority 5, source 0x88, destination 0x87
	EXPECT_EQ(8, testFrame.dataLength);
	EXPECT_EQ(0x11, testFrame.data[0]);
	EXPECT_EQ(0x01, testFrame.data[1]);
	EXPECT_EQ(0x02, testFrame.data[2]);
	EXPECT_EQ(0x03, testFrame.data[3]);
	EXPECT_EQ(0x04, testFrame.data[4]);
	EXPECT_EQ(0x05, testFrame.data[5]);
	EXPECT_EQ(0x06, testFrame.data[6]);
	EXPECT_EQ(0x07, testFrame.data[7]);

	// Request localization label
	CANNetworkManager::CANNetwork.receive_can_message(test_helpers::create_message(5,
	                                                                               0xCB00,
	                                                                               internalECU,
	                                                                               partnerClient,
	                                                                               { 0x21,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF }));
	CANNetworkManager::CANNetwork.update();
	server.update();
	EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));
	EXPECT_EQ(testFrame.identifier, 0x14CB8887); // Priority 5, source 0x88, destination 0x87
	EXPECT_EQ(8, testFrame.dataLength);
	EXPECT_EQ(0x31, testFrame.data[0]);
	EXPECT_EQ(0xFF, testFrame.data[1]);
	EXPECT_EQ(0xFF, testFrame.data[2]);
	EXPECT_EQ(0xFF, testFrame.data[3]);
	EXPECT_EQ(0xFF, testFrame.data[4]);
	EXPECT_EQ(0xFF, testFrame.data[5]);
	EXPECT_EQ(0xFF, testFrame.data[6]);
	EXPECT_EQ(0xFF, testFrame.data[7]);

	// Make sure a valid label is echoed back
	server.testLocalizationLabel = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
	CANNetworkManager::CANNetwork.receive_can_message(test_helpers::create_message(5,
	                                                                               0xCB00,
	                                                                               internalECU,
	                                                                               partnerClient,
	                                                                               { 0x21,
	                                                                                 0x01,
	                                                                                 0x02,
	                                                                                 0x03,
	                                                                                 0x04,
	                                                                                 0x05,
	                                                                                 0x06,
	                                                                                 0x07 }));
	CANNetworkManager::CANNetwork.update();
	server.update();
	EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));
	EXPECT_EQ(testFrame.identifier, 0x14CB8887); // Priority 5, source 0x88, destination 0x87
	EXPECT_EQ(8, testFrame.dataLength);
	EXPECT_EQ(0x31, testFrame.data[0]);
	EXPECT_EQ(0x01, testFrame.data[1]);
	EXPECT_EQ(0x02, testFrame.data[2]);
	EXPECT_EQ(0x03, testFrame.data[3]);
	EXPECT_EQ(0x04, testFrame.data[4]);
	EXPECT_EQ(0x05, testFrame.data[5]);
	EXPECT_EQ(0x06, testFrame.data[6]);
	EXPECT_EQ(0x07, testFrame.data[7]);

	// Request to transfer object pool
	CANNetworkManager::CANNetwork.receive_can_message(test_helpers::create_message(5,
	                                                                               0xCB00,
	                                                                               internalECU,
	                                                                               partnerClient,
	                                                                               { 0x41,
	                                                                                 static_cast<std::uint32_t>(sizeof(testDDOP) & 0xFF),
	                                                                                 static_cast<std::uint32_t>((sizeof(testDDOP) >> 8) & 0xFF),
	                                                                                 static_cast<std::uint32_t>((sizeof(testDDOP) >> 16) & 0xFF),
	                                                                                 static_cast<std::uint32_t>((sizeof(testDDOP) >> 24) & 0xFF),
	                                                                                 0xFF,
	                                                                                 0xFF,
	                                                                                 0xFF }));
	CANNetworkManager::CANNetwork.update();
	server.update();
	EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));
	EXPECT_EQ(testFrame.identifier, 0x14CB8887); // Priority 5, source 0x88, destination 0x87
	EXPECT_EQ(8, testFrame.dataLength);
	EXPECT_EQ(0x51, testFrame.data[0]); // Request to transfer object pool response
	EXPECT_EQ(0x00, testFrame.data[1]); // 0 Means there's probably enough memory
	EXPECT_EQ(0xFF, testFrame.data[2]);
	EXPECT_EQ(0xFF, testFrame.data[3]);
	EXPECT_EQ(0xFF, testFrame.data[4]);
	EXPECT_EQ(0xFF, testFrame.data[5]);
	EXPECT_EQ(0xFF, testFrame.data[6]);
	EXPECT_EQ(0xFF, testFrame.data[7]);

	// Construct a message to transfer the object pool
	{
		CANMessage message(0);
		message.set_identifier(CANIdentifier(test_helpers::create_ext_can_id(5, 0xCB00, internalECU, partnerClient)));
		message.set_source_control_function(partnerClient);
		message.set_destination_control_function(internalECU);
		message.set_data_size(sizeof(testDDOP) + 1);
		message.set_data(0x61, 0);

		for (std::size_t i = 0; i < sizeof(testDDOP); i++)
		{
			message.set_data(testDDOP[i], i + 1);
		}
		CANNetworkManager::CANNetwork.receive_can_message(message);
		CANNetworkManager::CANNetwork.update();
		server.update();
		EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));
		EXPECT_EQ(testFrame.identifier, 0x14CB8887); // Priority 5, source 0x88, destination 0x87
		EXPECT_EQ(8, testFrame.dataLength);
		EXPECT_EQ(0x71, testFrame.data[0]);
		EXPECT_EQ(0x00, testFrame.data[1]); // Object pool should have been transferred ok
		EXPECT_EQ(static_cast<std::uint32_t>(sizeof(testDDOP) & 0xFF), testFrame.data[2]);
		EXPECT_EQ(static_cast<std::uint32_t>((sizeof(testDDOP) >> 8) & 0xFF), testFrame.data[3]);
		EXPECT_EQ(static_cast<std::uint32_t>((sizeof(testDDOP) >> 16) & 0xFF), testFrame.data[4]);
		EXPECT_EQ(static_cast<std::uint32_t>((sizeof(testDDOP) >> 24) & 0xFF), testFrame.data[5]);
		EXPECT_EQ(0xFF, testFrame.data[6]);
		EXPECT_EQ(0xFF, testFrame.data[7]);
	}

	CANHardwareInterface::stop();
}