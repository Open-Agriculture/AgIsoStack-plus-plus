//================================================================================================
/// @file tc_server_tests.cpp
///
/// @brief Unit tests for the TaskControllerServer class.
/// @author Adrian Del Grosso
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_device_descriptor_object_pool_helpers.hpp"
#include "isobus/isobus/isobus_task_controller_server.hpp"
#include "isobus/utility/system_timing.hpp"

#include "helpers/control_function_helpers.hpp"
#include "helpers/messaging_helpers.hpp"

using namespace isobus;

// clang-format off
// array size is 2356
// This is the binary version of the example program's DDOP when it gets serialized
constexpr std::uint8_t testDDOP[]  = {
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
	                const TaskControllerOptions &options) :
	  TaskControllerServer(internalControlFunction,
	                       numberBoomsSupported,
	                       numberSectionsSupported,
	                       numberChannelsSupportedForPositionBasedControl,
	                       options)
	{
	}

	bool activate_object_pool(std::shared_ptr<ControlFunction>, ObjectPoolActivationError &activationError, ObjectPoolErrorCodes &poolError, std::uint16_t &parentObject, std::uint16_t &faultyObject) override
	{
		if (failActivations)
		{
			activationError = ObjectPoolActivationError::ThereAreErrorsInTheDDOP;
			poolError = ObjectPoolErrorCodes::UnknownObjectReference;
			parentObject = 1234;
			faultyObject = 789;
		}
		return !failActivations;
	}

	bool change_designator(std::shared_ptr<ControlFunction>, std::uint16_t, const std::vector<std::uint8_t> &) override
	{
		return true;
	}

	bool deactivate_object_pool(std::shared_ptr<ControlFunction>) override
	{
		return true;
	}

	bool delete_device_descriptor_object_pool(std::shared_ptr<ControlFunction>, ObjectPoolDeletionErrors &) override
	{
		return true;
	}

	bool get_is_stored_device_descriptor_object_pool_by_structure_label(std::shared_ptr<ControlFunction>, const std::vector<std::uint8_t> &, const std::vector<std::uint8_t> &) override
	{
		return !testStructureLabel.empty();
	}

	bool get_is_stored_device_descriptor_object_pool_by_localization_label(std::shared_ptr<ControlFunction>, const std::array<std::uint8_t, 7> &) override
	{
		return 0 != testLocalizationLabel.at(0);
	}

	bool get_is_enough_memory_available(std::uint32_t) override
	{
		return enoughMemory;
	}

	void identify_task_controller(std::uint8_t tcNumber) override
	{
		identifyTC = tcNumber;
	}

	void on_client_timeout(std::shared_ptr<ControlFunction>) override
	{
	}

	void on_process_data_acknowledge(std::shared_ptr<ControlFunction>, std::uint16_t, std::uint16_t, std::uint8_t, ProcessDataCommands) override
	{
	}

	bool on_value_command(std::shared_ptr<ControlFunction>, std::uint16_t, std::uint16_t, std::int32_t, std::uint8_t &) override
	{
		return true;
	}

	bool store_device_descriptor_object_pool(std::shared_ptr<ControlFunction>, const std::vector<std::uint8_t> &, bool) override
	{
		return true;
	}

	void test_receive_message(const CANMessage &message, void *parent)
	{
		TaskControllerServer::store_rx_message(message, parent);
	}

	std::uint32_t get_client_status() const
	{
		std::uint32_t retVal = 0;

		EXPECT_FALSE(activeClients.empty());
		if (!activeClients.empty())
		{
			retVal = activeClients.back()->statusBitfield;
		}
		return retVal;
	}

	bool send_status() const
	{
		return send_status_message();
	}

	std::vector<std::uint8_t> testStructureLabel;
	std::array<std::uint8_t, 7> testLocalizationLabel = { 0 };
	std::uint8_t identifyTC = 0xFF;
	bool failActivations = false;
	bool enoughMemory = true;
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
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(5,
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
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(5,
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

	DerivedTcServer server(internalECU,
	                       4,
	                       255,
	                       16,
	                       TaskControllerOptions()
	                         .with_documentation()
	                         .with_implement_section_control()
	                         .with_tc_geo_with_position_based_control());
	EXPECT_FALSE(server.get_initialized());
	server.initialize();
	EXPECT_TRUE(server.get_initialized());

	// Test language command interface was initialized
	auto &languageCommand = server.get_language_command_interface();
	EXPECT_TRUE(languageCommand.get_initialized());

	testPlugin.clear_queue();

	// Test that the server responds to requests for version information
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(5,
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
	EXPECT_EQ(0x15, testFrame.data[3]); // options
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
	testNackWrapper(testPlugin, server, testFrame, 0x10 | static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::DeviceDescriptor), internalECU, partnerClient); // Server message
	testNackWrapper(testPlugin, server, testFrame, 0x30 | static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::DeviceDescriptor), internalECU, partnerClient); // Server message
	testNackWrapper(testPlugin, server, testFrame, 0x50 | static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::DeviceDescriptor), internalECU, partnerClient); // Server message
	testNackWrapper(testPlugin, server, testFrame, 0x70 | static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::DeviceDescriptor), internalECU, partnerClient); // Server message
	testNackWrapper(testPlugin, server, testFrame, 0x90 | static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::DeviceDescriptor), internalECU, partnerClient); // Server message
	testNackWrapper(testPlugin, server, testFrame, 0xB0 | static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::DeviceDescriptor), internalECU, partnerClient); // Server message
	testNackWrapper(testPlugin, server, testFrame, 0xD0 | static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::DeviceDescriptor), internalECU, partnerClient); // Server message

	// Test PDNACKs
	testPDNackWrapper(testPlugin, server, testFrame, static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::MeasurementTimeInterval), internalECU, partnerClient);
	testPDNackWrapper(testPlugin, server, testFrame, static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::MeasurementDistanceInterval), internalECU, partnerClient);
	testPDNackWrapper(testPlugin, server, testFrame, static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::MeasurementMinimumWithinThreshold), internalECU, partnerClient);
	testPDNackWrapper(testPlugin, server, testFrame, static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::MeasurementMaximumWithinThreshold), internalECU, partnerClient);
	testPDNackWrapper(testPlugin, server, testFrame, static_cast<std::uint8_t>(TaskControllerServer::ProcessDataCommands::MeasurementChangeThreshold), internalECU, partnerClient);

	// Send working set master
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(6,
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
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(5,
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
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(5,
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
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(5,
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
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(5,
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

	// Send pool without a request, which is bad but we should tolerate it
	{
		std::vector<std::uint8_t> data;
		data.push_back(0x61);
		for (std::size_t i = 0; i < sizeof(testDDOP); i++)
		{
			data.push_back(testDDOP[i]);
		}

		CANMessage message(CANMessage::Type::Receive, CANIdentifier(test_helpers::create_ext_can_id(5, 0xCB00, internalECU, partnerClient)), data, partnerClient, internalECU, 0);
		server.test_receive_message(message, &server);
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

		// Test receiving messages without parent pointer is not allowed
		server.test_receive_message(message, nullptr);
		CANNetworkManager::CANNetwork.update();
		server.update();
		EXPECT_FALSE(readFrameFilterStatus(testPlugin, testFrame));
	}

	// Request to transfer object pool
	{
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(5,
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

		// Try a failing request
		server.enoughMemory = false;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(5,
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
		EXPECT_EQ(0x01, testFrame.data[1]); // 1 Means there's not enough memory
		EXPECT_EQ(0xFF, testFrame.data[2]);
		EXPECT_EQ(0xFF, testFrame.data[3]);
		EXPECT_EQ(0xFF, testFrame.data[4]);
		EXPECT_EQ(0xFF, testFrame.data[5]);
		EXPECT_EQ(0xFF, testFrame.data[6]);
		EXPECT_EQ(0xFF, testFrame.data[7]);
		server.enoughMemory = true;
	}

	// Construct a message to transfer the object pool
	{
		std::vector<std::uint8_t> data;
		data.push_back(0x61);
		for (std::size_t i = 0; i < sizeof(testDDOP); i++)
		{
			data.push_back(testDDOP[i]);
		}

		CANMessage message(CANMessage::Type::Receive, CANIdentifier(test_helpers::create_ext_can_id(5, 0xCB00, internalECU, partnerClient)), data, partnerClient, internalECU, 0);
		server.test_receive_message(message, &server);
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

	// Send a value request
	{
		EXPECT_TRUE(server.send_request_value(partnerClient, 1234, 456));
		CANNetworkManager::CANNetwork.update();
		EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));

		EXPECT_EQ(2, testFrame.data[0] & 0x0F); // Command
		EXPECT_EQ(456 & 0x0F, testFrame.data[0] >> 4); // Element
		EXPECT_EQ(456 >> 4, testFrame.data[1]); // Element
		EXPECT_EQ(1234 & 0xFF, testFrame.data[2]); // DDI
		EXPECT_EQ((1234 >> 8), testFrame.data[3]); // DDI
		EXPECT_EQ(0xFF, testFrame.data[4]);
		EXPECT_EQ(0xFF, testFrame.data[5]);
		EXPECT_EQ(0xFF, testFrame.data[6]);
		EXPECT_EQ(0xFF, testFrame.data[7]);
		EXPECT_EQ(8, testFrame.dataLength);
		EXPECT_EQ(0x14CB8887, testFrame.identifier);
	}

	// Send time interval measurement command
	{
		EXPECT_TRUE(server.send_time_interval_measurement_command(partnerClient, 6, 99, 1000));
		CANNetworkManager::CANNetwork.update();
		EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));

		EXPECT_EQ(4, testFrame.data[0] & 0x0F); // Command
		EXPECT_EQ(99 & 0x0F, testFrame.data[0] >> 4); // Element
		EXPECT_EQ(99 >> 4, testFrame.data[1]); // Element
		EXPECT_EQ(6 & 0xFF, testFrame.data[2]); // DDI
		EXPECT_EQ((6 >> 8), testFrame.data[3]); // DDI
		EXPECT_EQ(1000 & 0xFF, testFrame.data[4]);
		EXPECT_EQ((1000 >> 8) & 0xFF, testFrame.data[5]);
		EXPECT_EQ((1000 >> 16) & 0xFF, testFrame.data[6]);
		EXPECT_EQ((1000 >> 24) & 0xFF, testFrame.data[7]);
		EXPECT_EQ(8, testFrame.dataLength);
		EXPECT_EQ(0x14CB8887, testFrame.identifier);
	}

	// Send distance interval measurement command
	{
		EXPECT_TRUE(server.send_distance_interval_measurement_command(partnerClient, 654, 999, 65534));
		CANNetworkManager::CANNetwork.update();
		EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));

		EXPECT_EQ(5, testFrame.data[0] & 0x0F); // Command
		EXPECT_EQ(999 & 0x0F, testFrame.data[0] >> 4); // Element
		EXPECT_EQ(999 >> 4, testFrame.data[1]); // Element
		EXPECT_EQ(654 & 0xFF, testFrame.data[2]); // DDI
		EXPECT_EQ((654 >> 8), testFrame.data[3]); // DDI
		EXPECT_EQ(65534 & 0xFF, testFrame.data[4]);
		EXPECT_EQ((65534 >> 8) & 0xFF, testFrame.data[5]);
		EXPECT_EQ((65534 >> 16) & 0xFF, testFrame.data[6]);
		EXPECT_EQ((65534 >> 24) & 0xFF, testFrame.data[7]);
		EXPECT_EQ(8, testFrame.dataLength);
		EXPECT_EQ(0x14CB8887, testFrame.identifier);
	}

	// Send minimum threshold measurement command
	{
		EXPECT_TRUE(server.send_minimum_threshold_measurement_command(partnerClient, 445, 0, 0x00FFFFFF));
		CANNetworkManager::CANNetwork.update();
		EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));

		EXPECT_EQ(6, testFrame.data[0] & 0x0F); // Command
		EXPECT_EQ(0 & 0x0F, testFrame.data[0] >> 4); // Element
		EXPECT_EQ(0 >> 4, testFrame.data[1]); // Element
		EXPECT_EQ(445 & 0xFF, testFrame.data[2]); // DDI
		EXPECT_EQ((445 >> 8), testFrame.data[3]); // DDI
		EXPECT_EQ(0x00FFFFFF & 0xFF, testFrame.data[4]);
		EXPECT_EQ((0x00FFFFFF >> 8) & 0xFF, testFrame.data[5]);
		EXPECT_EQ((0x00FFFFFF >> 16) & 0xFF, testFrame.data[6]);
		EXPECT_EQ((0x00FFFFFF >> 24) & 0xFF, testFrame.data[7]);
		EXPECT_EQ(8, testFrame.dataLength);
		EXPECT_EQ(0x14CB8887, testFrame.identifier);
	}

	// Send maximum threshold measurement command
	{
		EXPECT_TRUE(server.send_maximum_threshold_measurement_command(partnerClient, 445, 0, 0xFFFFFFFF));
		CANNetworkManager::CANNetwork.update();
		EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));

		EXPECT_EQ(7, testFrame.data[0] & 0x0F); // Command
		EXPECT_EQ(0 & 0x0F, testFrame.data[0] >> 4); // Element
		EXPECT_EQ(0 >> 4, testFrame.data[1]); // Element
		EXPECT_EQ(445 & 0xFF, testFrame.data[2]); // DDI
		EXPECT_EQ((445 >> 8), testFrame.data[3]); // DDI
		EXPECT_EQ(0xFFFFFFFF & 0xFF, testFrame.data[4]);
		EXPECT_EQ((0xFFFFFFFF >> 8) & 0xFF, testFrame.data[5]);
		EXPECT_EQ((0xFFFFFFFF >> 16) & 0xFF, testFrame.data[6]);
		EXPECT_EQ((0xFFFFFFFF >> 24) & 0xFF, testFrame.data[7]);
		EXPECT_EQ(8, testFrame.dataLength);
		EXPECT_EQ(0x14CB8887, testFrame.identifier);
	}

	// Send change threshold measurement command
	{
		EXPECT_TRUE(server.send_change_threshold_measurement_command(partnerClient, 14, 0, 1));
		CANNetworkManager::CANNetwork.update();
		EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));

		EXPECT_EQ(8, testFrame.data[0] & 0x0F); // Command
		EXPECT_EQ(0 & 0x0F, testFrame.data[0] >> 4); // Element
		EXPECT_EQ(0 >> 4, testFrame.data[1]); // Element
		EXPECT_EQ(14 & 0xFF, testFrame.data[2]); // DDI
		EXPECT_EQ((14 >> 8), testFrame.data[3]); // DDI
		EXPECT_EQ(1, testFrame.data[4]);
		EXPECT_EQ(0, testFrame.data[5]);
		EXPECT_EQ(0, testFrame.data[6]);
		EXPECT_EQ(0, testFrame.data[7]);
		EXPECT_EQ(8, testFrame.dataLength);
		EXPECT_EQ(0x14CB8887, testFrame.identifier);
	}

	// Set value and ack
	{
		EXPECT_TRUE(server.send_set_value_and_acknowledge(partnerClient, 14, 0, 600));
		CANNetworkManager::CANNetwork.update();
		EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));

		EXPECT_EQ(10, testFrame.data[0] & 0x0F); // Command
		EXPECT_EQ(0 & 0x0F, testFrame.data[0] >> 4); // Element
		EXPECT_EQ(0 >> 4, testFrame.data[1]); // Element
		EXPECT_EQ(14 & 0xFF, testFrame.data[2]); // DDI
		EXPECT_EQ((14 >> 8), testFrame.data[3]); // DDI
		EXPECT_EQ(600 & 0xFF, testFrame.data[4]);
		EXPECT_EQ(600 >> 8, testFrame.data[5]);
		EXPECT_EQ(0, testFrame.data[6]);
		EXPECT_EQ(0, testFrame.data[7]);
		EXPECT_EQ(8, testFrame.dataLength);
		EXPECT_EQ(0x0ccb8887, testFrame.identifier); // Higher priority than the other messages
	}

	// Set value
	{
		EXPECT_TRUE(server.send_set_value(partnerClient, 2455, 0, 800));
		CANNetworkManager::CANNetwork.update();
		EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));

		EXPECT_EQ(3, testFrame.data[0] & 0x0F); // Command
		EXPECT_EQ(0 & 0x0F, testFrame.data[0] >> 4); // Element
		EXPECT_EQ(0 >> 4, testFrame.data[1]); // Element
		EXPECT_EQ(2455 & 0xFF, testFrame.data[2]); // DDI
		EXPECT_EQ((2455 >> 8), testFrame.data[3]); // DDI
		EXPECT_EQ(800 & 0xFF, testFrame.data[4]);
		EXPECT_EQ(800 >> 8, testFrame.data[5]);
		EXPECT_EQ(0, testFrame.data[6]);
		EXPECT_EQ(0, testFrame.data[7]);
		EXPECT_EQ(8, testFrame.dataLength);
		EXPECT_EQ(0x14CB8887, testFrame.identifier);
	}

	// Test task status
	{
		EXPECT_FALSE(server.get_task_totals_active());
		server.set_task_totals_active(true);
		EXPECT_TRUE(server.get_task_totals_active());
	}

	// Test identify TC
	{
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(5,
		                                                                                                   0xCB00,
		                                                                                                   internalECU,
		                                                                                                   partnerClient,
		                                                                                                   { 0x20,
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

		EXPECT_EQ(8, testFrame.dataLength);
		EXPECT_EQ(0x20, testFrame.data[0]); // Response to identify TC
		// All other bytes reserved, FFs
		EXPECT_EQ(0xFF, testFrame.data[1]);
		EXPECT_EQ(0xFF, testFrame.data[2]);
		EXPECT_EQ(0xFF, testFrame.data[3]);
		EXPECT_EQ(0xFF, testFrame.data[4]);
		EXPECT_EQ(0xFF, testFrame.data[5]);
		EXPECT_EQ(0xFF, testFrame.data[6]);
		EXPECT_EQ(0xFF, testFrame.data[7]);
		EXPECT_EQ(1, server.identifyTC);
		server.identifyTC = 45;

		// Try a global request as well
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(5,
		                                                                                                             0xCB00,
		                                                                                                             internalECU,
		                                                                                                             { 0x20,
		                                                                                                               0xFF,
		                                                                                                               0xFF,
		                                                                                                               0xFF,
		                                                                                                               0xFF,
		                                                                                                               0xFF,
		                                                                                                               0xFF,
		                                                                                                               0xFF }));
		CANNetworkManager::CANNetwork.update();
		server.update();
		EXPECT_EQ(1, server.identifyTC);
	}

	// Test activate object pool
	{
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(5,
		                                                                                                   0xCB00,
		                                                                                                   internalECU,
		                                                                                                   partnerClient,
		                                                                                                   { 0x81,
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

		if (0xEE == ((testFrame.identifier >> 16) & 0xFF))
		{
			// Filter out address violations
			EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));
		}

		EXPECT_EQ(0x91, testFrame.data[0]); // Response to activate object pool
		EXPECT_EQ(0x00, testFrame.data[1]); // No errors
		EXPECT_EQ(0xFF, testFrame.data[2]); // Parent object
		EXPECT_EQ(0xFF, testFrame.data[3]); // Parent object
		EXPECT_EQ(0xFF, testFrame.data[4]); // Faulting object ID
		EXPECT_EQ(0xFF, testFrame.data[5]); // Faulting object ID
		EXPECT_EQ(0x00, testFrame.data[6]); // Pool error codes (0 = none)
		EXPECT_EQ(0xFF, testFrame.data[7]); // reserved

		// test failing to activate returns reported faulty objects
		server.failActivations = true;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(5,
		                                                                                                   0xCB00,
		                                                                                                   internalECU,
		                                                                                                   partnerClient,
		                                                                                                   { 0x81,
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
		EXPECT_EQ(0x91, testFrame.data[0]); // Response to activate object pool
		EXPECT_EQ(0x01, testFrame.data[1]); // Errors in DDOP
		EXPECT_EQ(1234 & 0xFF, testFrame.data[2]); // Parent Object
		EXPECT_EQ(1234 >> 8, testFrame.data[3]); // Parent Object
		EXPECT_EQ(789 & 0xFF, testFrame.data[4]); // Parent Object
		EXPECT_EQ(789 >> 8, testFrame.data[5]); // Parent Object
		EXPECT_EQ(0x02, testFrame.data[6]); // Error code
		EXPECT_EQ(0xFF, testFrame.data[7]); // reserved

		// Deactivate object pool
		server.failActivations = false;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(5,
		                                                                                                   0xCB00,
		                                                                                                   internalECU,
		                                                                                                   partnerClient,
		                                                                                                   { 0x81,
		                                                                                                     0x00, // Deactivate. Oxff was activate
		                                                                                                     0xFF,
		                                                                                                     0xFF,
		                                                                                                     0xFF,
		                                                                                                     0xFF,
		                                                                                                     0xFF,
		                                                                                                     0xFF }));
		CANNetworkManager::CANNetwork.update();
		server.update();
		EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));
		EXPECT_EQ(0x91, testFrame.data[0]); // Response to deactivate object pool
		EXPECT_EQ(0x00, testFrame.data[1]); // No errors
		EXPECT_EQ(0xFF, testFrame.data[2]); // Parent object
		EXPECT_EQ(0xFF, testFrame.data[3]); // Parent object
		EXPECT_EQ(0xFF, testFrame.data[4]); // Faulting object ID
		EXPECT_EQ(0xFF, testFrame.data[5]); // Faulting object ID
		EXPECT_EQ(0x00, testFrame.data[6]); // Pool error codes (0 = none)
		EXPECT_EQ(0xFF, testFrame.data[7]); // reserved
	}

	// Delete object pool
	{
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(5,
		                                                                                                   0xCB00,
		                                                                                                   internalECU,
		                                                                                                   partnerClient,
		                                                                                                   { 0xA1,
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
		EXPECT_EQ(0xB1, testFrame.data[0]); // Response to deactivate object pool
		EXPECT_EQ(0x00, testFrame.data[1]); // No errors
		EXPECT_EQ(0xFF, testFrame.data[2]); // Error details not available
		EXPECT_EQ(0xFF, testFrame.data[3]); // reserved
		EXPECT_EQ(0xFF, testFrame.data[4]); // reserved
		EXPECT_EQ(0xFF, testFrame.data[5]); // reserved
		EXPECT_EQ(0xFF, testFrame.data[6]); // reserved
		EXPECT_EQ(0xFF, testFrame.data[7]); // reserved
	}

	// test change designator
	{
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(5,
		                                                                                                   0xCB00,
		                                                                                                   internalECU,
		                                                                                                   partnerClient,
		                                                                                                   { 0xC1,
		                                                                                                     0x01, // ID
		                                                                                                     0x00, // ID
		                                                                                                     0x02, // Length
		                                                                                                     'A',
		                                                                                                     'B',
		                                                                                                     0xFF,
		                                                                                                     0xFF }));
		CANNetworkManager::CANNetwork.update();
		server.update();
		EXPECT_FALSE(readFrameFilterStatus(testPlugin, testFrame)); // We'd ignore this message ideally

		// Now try with the pool activated
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(5,
		                                                                                                   0xCB00,
		                                                                                                   internalECU,
		                                                                                                   partnerClient,
		                                                                                                   { 0x81,
		                                                                                                     0xFF,
		                                                                                                     0xFF,
		                                                                                                     0xFF,
		                                                                                                     0xFF,
		                                                                                                     0xFF,
		                                                                                                     0xFF,
		                                                                                                     0xFF }));
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(5,
		                                                                                                   0xCB00,
		                                                                                                   internalECU,
		                                                                                                   partnerClient,
		                                                                                                   { 0xC1,
		                                                                                                     0x01, // ID
		                                                                                                     0x00, // ID
		                                                                                                     0x02, // Length
		                                                                                                     'A',
		                                                                                                     'B',
		                                                                                                     0xFF,
		                                                                                                     0xFF }));
		CANNetworkManager::CANNetwork.update();
		server.update();
		EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));
		EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));
		EXPECT_EQ(8, testFrame.dataLength);
		EXPECT_EQ(0xD1, testFrame.data[0]); // Response to change designator
		EXPECT_EQ(0x01, testFrame.data[1]); // ID
		EXPECT_EQ(0x00, testFrame.data[2]); // ID
		EXPECT_EQ(0x00, testFrame.data[3]); // Error code
		EXPECT_EQ(0xFF, testFrame.data[4]); // reserved
		EXPECT_EQ(0xFF, testFrame.data[5]); // reserved
		EXPECT_EQ(0xFF, testFrame.data[6]); // reserved
		EXPECT_EQ(0xFF, testFrame.data[7]); // reserved
	}

	// Test value command and acknowledge works
	{
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(5,
		                                                                                                   0xCB00,
		                                                                                                   internalECU,
		                                                                                                   partnerClient,
		                                                                                                   { 0x4A, // Element 4 set and ack
		                                                                                                     0x00,
		                                                                                                     0x07, //DDI LSB
		                                                                                                     0x00,
		                                                                                                     0x01, // Value LSB
		                                                                                                     0x02,
		                                                                                                     0x03,
		                                                                                                     0x04 }));
		CANNetworkManager::CANNetwork.update();
		server.update();
		EXPECT_TRUE(readFrameFilterStatus(testPlugin, testFrame));

		// Expect PDACK
		EXPECT_EQ(8, testFrame.dataLength);
		EXPECT_EQ(0x4D, testFrame.data[0]); // PDACK, element 4
		EXPECT_EQ(0x00, testFrame.data[1]); // Element
		EXPECT_EQ(0x07, testFrame.data[2]); // DDI
		EXPECT_EQ(0x00, testFrame.data[3]); // DDI
		EXPECT_EQ(0x00, testFrame.data[4]); // Error codes
		EXPECT_EQ(0xFA, testFrame.data[5]); // Command
		EXPECT_EQ(0xFF, testFrame.data[6]); // reserved
		EXPECT_EQ(0xFF, testFrame.data[7]); // reserved
	}

	// Test client task message populated the client's state
	{
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(5,
		                                                                                                   0xCB00,
		                                                                                                   internalECU,
		                                                                                                   partnerClient,
		                                                                                                   { 0xFF, // Client task
		                                                                                                     0xFF, // N/A
		                                                                                                     0xFF, // DDI N/A
		                                                                                                     0xFF, // DDI N/A
		                                                                                                     0x01, // Status (Task active)
		                                                                                                     0x00,
		                                                                                                     0x00,
		                                                                                                     0x00 }));
		CANNetworkManager::CANNetwork.update();
		server.update();
		EXPECT_EQ(server.get_client_status(), 1);
	}

	// Test status message
	{
		EXPECT_TRUE(server.send_status());
		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		EXPECT_EQ(8, testFrame.dataLength);
		EXPECT_EQ(0xFE, testFrame.data[0]);
		EXPECT_EQ(0xFF, testFrame.data[1]);
		EXPECT_EQ(0xFF, testFrame.data[2]);
		EXPECT_EQ(0xFF, testFrame.data[3]);
		EXPECT_EQ(0x01, testFrame.data[4]); // Task active bit
		EXPECT_EQ(0xFE, testFrame.data[5]); // Address of client with executing command
		EXPECT_EQ(0x00, testFrame.data[6]); // Executing command
		EXPECT_EQ(0xFF, testFrame.data[7]); // Address of client with executing command

		// Disable task active
		server.set_task_totals_active(false);
		EXPECT_TRUE(server.send_status());
		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		EXPECT_EQ(8, testFrame.dataLength);
		EXPECT_EQ(0xFE, testFrame.data[0]);
		EXPECT_EQ(0xFF, testFrame.data[1]);
		EXPECT_EQ(0xFF, testFrame.data[2]);
		EXPECT_EQ(0xFF, testFrame.data[3]);
		EXPECT_EQ(0x00, testFrame.data[4]); // Task active bit
		EXPECT_EQ(0xFE, testFrame.data[5]); // Address of client with executing command
		EXPECT_EQ(0x00, testFrame.data[6]); // Executing command
		EXPECT_EQ(0xFF, testFrame.data[7]); // Address of client with executing command
	}
	CANHardwareInterface::stop();
}

TEST(TASK_CONTROLLER_SERVER_TESTS, DDOPHelper_SeederExample)
{
	DeviceDescriptorObjectPool ddop(3);
	ddop.deserialize_binary_object_pool(testDDOP, sizeof(testDDOP));

	auto implement = DeviceDescriptorObjectPoolHelper::get_implement_geometry(ddop);

	ASSERT_EQ(1, implement.booms.size());
	ASSERT_EQ(16, implement.booms.at(0).sections.size());
	ASSERT_EQ(1, implement.booms.at(0).rates.size());
	EXPECT_TRUE(implement.booms.at(0).subBooms.empty());

	EXPECT_TRUE(implement.booms.at(0).xOffset_mm);
	EXPECT_TRUE(implement.booms.at(0).yOffset_mm);
	EXPECT_TRUE(implement.booms.at(0).zOffset_mm);

	EXPECT_EQ(1, implement.booms.at(0).rates.at(0).rateSetpoint.dataDictionaryIdentifier); // Setpoint Application Rate specified as volume per area
	EXPECT_EQ(2, implement.booms.at(0).rates.at(0).rateActual.dataDictionaryIdentifier); // Actual Application Rate specified as volume per area
	EXPECT_TRUE(implement.booms.at(0).rates.at(0).rateSetpoint.editable());

	for (std::size_t i = 0; i < 16; i++)
	{
		EXPECT_TRUE(implement.booms.at(0).sections.at(i).width_mm);
		EXPECT_TRUE(implement.booms.at(0).sections.at(i).xOffset_mm);
		EXPECT_TRUE(implement.booms.at(0).sections.at(i).yOffset_mm);
		EXPECT_FALSE(implement.booms.at(0).sections.at(i).zOffset_mm);

		EXPECT_EQ(2286, implement.booms.at(0).sections.at(i).width_mm.get());
		EXPECT_EQ((2286 * i) - ((8 * 2286) - 1143), implement.booms.at(0).sections.at(i).yOffset_mm.get());
		EXPECT_EQ(-20, implement.booms.at(0).sections.at(i).xOffset_mm.get());
	}
}

TEST(TASK_CONTROLLER_SERVER_TESTS, DDOPHelper_SubBooms)
{
	DeviceDescriptorObjectPool ddop(3);
	ddop.add_device("TEST", "123", "123", "1234567", { 1, 2, 3, 4, 5, 6, 7 }, {}, 0);
	ddop.add_device_element("Device", 0, 0, isobus::task_controller_object::DeviceElementObject::Type::Device, 1);
	ddop.add_device_element("MainBoom", 0, 1, isobus::task_controller_object::DeviceElementObject::Type::Function, 11);
	ddop.add_device_element("SubBoom1", 0, 11, isobus::task_controller_object::DeviceElementObject::Type::Function, 2);
	ddop.add_device_element("SubBoom2", 0, 11, isobus::task_controller_object::DeviceElementObject::Type::Function, 3);
	ddop.add_device_element("Section1", 0, 2, isobus::task_controller_object::DeviceElementObject::Type::Section, 4);
	ddop.add_device_element("Section2", 0, 3, isobus::task_controller_object::DeviceElementObject::Type::Section, 5);
	ddop.add_device_element("SubBoomProduct", 0, 2, isobus::task_controller_object::DeviceElementObject::Type::Bin, 40);
	ddop.add_device_property("Xoffset", 2000, static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetX), 0xFFFF, 6);
	ddop.add_device_property("yoffset", 3000, static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetY), 0xFFFF, 7);
	ddop.add_device_property("zoffset", 4000, static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetZ), 0xFFFF, 8);
	ddop.add_device_property("width1", 5000, static_cast<std::uint16_t>(DataDescriptionIndex::ActualWorkingWidth), 0xFFFF, 9);
	ddop.add_device_property("width2", 6000, static_cast<std::uint16_t>(DataDescriptionIndex::ActualWorkingWidth), 0xFFFF, 10);
	ddop.add_device_property("SBzoffset", 7000, static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetZ), 0xFFFF, 12);
	ddop.add_device_process_data("SBxoffset", static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetX), 0xFFFF, 0, 0, 13);
	ddop.add_device_process_data("secTestDPD", static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetX), 0xFFFF, 0, 0, 14);
	ddop.add_device_process_data("SBRate", static_cast<std::uint16_t>(DataDescriptionIndex::ActualApplicationRateOfPhosphor), 0xFFFF, 0, 0, 41);

	auto section1 = std::static_pointer_cast<isobus::task_controller_object::DeviceElementObject>(ddop.get_object_by_id(4));
	auto section2 = std::static_pointer_cast<isobus::task_controller_object::DeviceElementObject>(ddop.get_object_by_id(5));
	auto subBoom1 = std::static_pointer_cast<isobus::task_controller_object::DeviceElementObject>(ddop.get_object_by_id(2));
	auto bin1 = std::static_pointer_cast<isobus::task_controller_object::DeviceElementObject>(ddop.get_object_by_id(40));
	ASSERT_NE(nullptr, section1);
	ASSERT_NE(nullptr, section2);

	section1->add_reference_to_child_object(6);
	section1->add_reference_to_child_object(7);
	section1->add_reference_to_child_object(8);
	section1->add_reference_to_child_object(9);
	section2->add_reference_to_child_object(14);
	section2->add_reference_to_child_object(7);
	section2->add_reference_to_child_object(8);
	section2->add_reference_to_child_object(10);
	subBoom1->add_reference_to_child_object(12);
	subBoom1->add_reference_to_child_object(13);
	subBoom1->add_reference_to_child_object(40);
	bin1->add_reference_to_child_object(41);

	auto implement = DeviceDescriptorObjectPoolHelper::get_implement_geometry(ddop);

	ASSERT_EQ(1, implement.booms.size());
	ASSERT_EQ(0, implement.booms.at(0).sections.size());
	ASSERT_EQ(2, implement.booms.at(0).subBooms.size());
	ASSERT_EQ(1, implement.booms.at(0).subBooms.at(0).sections.size());
	ASSERT_EQ(1, implement.booms.at(0).subBooms.at(1).sections.size());
	ASSERT_EQ(1, implement.booms.at(0).subBooms.at(0).rates.size());

	EXPECT_FALSE(implement.booms.at(0).xOffset_mm);
	EXPECT_FALSE(implement.booms.at(0).yOffset_mm);
	EXPECT_FALSE(implement.booms.at(0).zOffset_mm);
	EXPECT_FALSE(implement.booms.at(0).subBooms.at(0).xOffset_mm);
	EXPECT_FALSE(implement.booms.at(0).subBooms.at(0).xOffset_mm.editable()); // Settable bit is unset
	EXPECT_FALSE(implement.booms.at(0).subBooms.at(0).yOffset_mm);
	EXPECT_FALSE(implement.booms.at(0).subBooms.at(0).yOffset_mm.editable());
	EXPECT_TRUE(implement.booms.at(0).subBooms.at(0).zOffset_mm);
	EXPECT_EQ(7000, implement.booms.at(0).subBooms.at(0).zOffset_mm.get());

	EXPECT_TRUE(implement.booms.at(0).subBooms.at(0).sections.at(0).width_mm);
	EXPECT_TRUE(implement.booms.at(0).subBooms.at(0).sections.at(0).xOffset_mm);
	EXPECT_TRUE(implement.booms.at(0).subBooms.at(0).sections.at(0).yOffset_mm);
	EXPECT_TRUE(implement.booms.at(0).subBooms.at(0).sections.at(0).zOffset_mm);
	EXPECT_TRUE(implement.booms.at(0).subBooms.at(1).sections.at(0).width_mm);
	EXPECT_FALSE(implement.booms.at(0).subBooms.at(1).sections.at(0).xOffset_mm);
	EXPECT_FALSE(implement.booms.at(0).subBooms.at(1).sections.at(0).xOffset_mm.editable()); // Settable bit is unset
	EXPECT_TRUE(implement.booms.at(0).subBooms.at(1).sections.at(0).yOffset_mm);
	EXPECT_TRUE(implement.booms.at(0).subBooms.at(1).sections.at(0).zOffset_mm);

	EXPECT_EQ(5000, implement.booms.at(0).subBooms.at(0).sections.at(0).width_mm.get());
	EXPECT_EQ(2000, implement.booms.at(0).subBooms.at(0).sections.at(0).xOffset_mm.get());
	EXPECT_EQ(3000, implement.booms.at(0).subBooms.at(0).sections.at(0).yOffset_mm.get());
	EXPECT_EQ(4000, implement.booms.at(0).subBooms.at(0).sections.at(0).zOffset_mm.get());
	EXPECT_EQ(6000, implement.booms.at(0).subBooms.at(1).sections.at(0).width_mm.get());
	EXPECT_EQ(3000, implement.booms.at(0).subBooms.at(1).sections.at(0).yOffset_mm.get());
	EXPECT_EQ(4000, implement.booms.at(0).subBooms.at(1).sections.at(0).zOffset_mm.get());
}

TEST(TASK_CONTROLLER_SERVER_TESTS, DDOPHelper_NoFunctions)
{
	DeviceDescriptorObjectPool ddop(3);

	// validate that an empty DDOP returns an empty implement
	auto emptyImplement = DeviceDescriptorObjectPoolHelper::get_implement_geometry(ddop);
	EXPECT_EQ(0, emptyImplement.booms.size());

	// Test that a DDOP with no device elements returns an empty implement
	ddop.add_device_element("Device", 0, 0, isobus::task_controller_object::DeviceElementObject::Type::Device, 1);
	auto emptyImplement2 = DeviceDescriptorObjectPoolHelper::get_implement_geometry(ddop);
	EXPECT_EQ(0, emptyImplement2.booms.size());

	ddop.add_device("TEST", "123", "123", "1234567", { 1, 2, 3, 4, 5, 6, 7 }, {}, 0);
	ddop.add_device_element("Section1", 0, 1, isobus::task_controller_object::DeviceElementObject::Type::Section, 4);
	ddop.add_device_element("Section2", 1, 1, isobus::task_controller_object::DeviceElementObject::Type::Section, 5);
	ddop.add_device_element("Product", 2, 1, isobus::task_controller_object::DeviceElementObject::Type::Bin, 45);
	ddop.add_device_property("Xoffset", 2000, static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetX), 0xFFFF, 6);
	ddop.add_device_property("yoffset", 3000, static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetY), 0xFFFF, 7);
	ddop.add_device_property("zoffset", 4000, static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetZ), 0xFFFF, 8);
	ddop.add_device_property("width1", 5000, static_cast<std::uint16_t>(DataDescriptionIndex::ActualWorkingWidth), 0xFFFF, 9);
	ddop.add_device_property("width2", 6000, static_cast<std::uint16_t>(DataDescriptionIndex::ActualWorkingWidth), 0xFFFF, 10);
	ddop.add_device_property("Rate Setpoint", 7000, static_cast<std::uint16_t>(DataDescriptionIndex::SetpointMassPerAreaApplicationRate), 0xFFFF, 46);
	ddop.add_device_property("Rate Default", 8000, static_cast<std::uint16_t>(DataDescriptionIndex::DefaultMassPerAreaApplicationRate), 0xFFFF, 47);
	ddop.add_device_property("Rate Max", 9000, static_cast<std::uint16_t>(DataDescriptionIndex::MaximumVolumePerMassApplicationRate), 0xFFFF, 48);
	ddop.add_device_property("Rate Min", 0, static_cast<std::uint16_t>(DataDescriptionIndex::MinimumVolumePerMassApplicationRate), 0xFFFF, 49);

	auto section1 = std::static_pointer_cast<isobus::task_controller_object::DeviceElementObject>(ddop.get_object_by_id(4));
	auto section2 = std::static_pointer_cast<isobus::task_controller_object::DeviceElementObject>(ddop.get_object_by_id(5));
	auto product = std::static_pointer_cast<isobus::task_controller_object::DeviceElementObject>(ddop.get_object_by_id(45));
	ASSERT_NE(nullptr, section1);
	ASSERT_NE(nullptr, section2);
	ASSERT_NE(nullptr, product);

	section1->add_reference_to_child_object(6);
	section1->add_reference_to_child_object(7);
	section1->add_reference_to_child_object(8);
	section1->add_reference_to_child_object(9);
	section2->add_reference_to_child_object(6);
	section2->add_reference_to_child_object(7);
	section2->add_reference_to_child_object(8);
	section2->add_reference_to_child_object(10);
	product->add_reference_to_child_object(46);
	product->add_reference_to_child_object(47);
	product->add_reference_to_child_object(48);
	product->add_reference_to_child_object(49);

	auto implement = DeviceDescriptorObjectPoolHelper::get_implement_geometry(ddop);

	ASSERT_EQ(1, implement.booms.size());
	ASSERT_EQ(2, implement.booms.at(0).sections.size());
	ASSERT_EQ(0, implement.booms.at(0).subBooms.size());
	ASSERT_EQ(1, implement.booms.at(0).rates.size());
	ASSERT_EQ(2, implement.booms.at(0).rates.at(0).elementNumber);

	EXPECT_EQ(7000, implement.booms.at(0).rates.at(0).rateSetpoint.get());
	EXPECT_EQ(8000, implement.booms.at(0).rates.at(0).rateDefault.get());
	EXPECT_EQ(6, implement.booms.at(0).rates.at(0).rateSetpoint.dataDictionaryIdentifier);
	EXPECT_EQ(8, implement.booms.at(0).rates.at(0).rateDefault.dataDictionaryIdentifier);
	EXPECT_FALSE(implement.booms.at(0).rates.at(0).rateSetpoint.editable());
	EXPECT_FALSE(implement.booms.at(0).rates.at(0).rateDefault.editable());

	EXPECT_FALSE(implement.booms.at(0).xOffset_mm);
	EXPECT_FALSE(implement.booms.at(0).yOffset_mm);
	EXPECT_FALSE(implement.booms.at(0).zOffset_mm);

	EXPECT_TRUE(implement.booms.at(0).sections.at(0).width_mm);
	EXPECT_TRUE(implement.booms.at(0).sections.at(0).xOffset_mm);
	EXPECT_TRUE(implement.booms.at(0).sections.at(0).yOffset_mm);
	EXPECT_TRUE(implement.booms.at(0).sections.at(0).zOffset_mm);
	EXPECT_TRUE(implement.booms.at(0).sections.at(0).width_mm);
	EXPECT_TRUE(implement.booms.at(0).sections.at(0).xOffset_mm);
	EXPECT_TRUE(implement.booms.at(0).sections.at(0).yOffset_mm);
	EXPECT_TRUE(implement.booms.at(0).sections.at(0).zOffset_mm);

	EXPECT_EQ(5000, implement.booms.at(0).sections.at(0).width_mm.get());
	EXPECT_EQ(2000, implement.booms.at(0).sections.at(0).xOffset_mm.get());
	EXPECT_EQ(3000, implement.booms.at(0).sections.at(0).yOffset_mm.get());
	EXPECT_EQ(4000, implement.booms.at(0).sections.at(0).zOffset_mm.get());
	EXPECT_EQ(6000, implement.booms.at(0).sections.at(1).width_mm.get());
	EXPECT_EQ(2000, implement.booms.at(0).sections.at(0).xOffset_mm.get());
	EXPECT_EQ(3000, implement.booms.at(0).sections.at(0).yOffset_mm.get());
	EXPECT_EQ(4000, implement.booms.at(0).sections.at(0).zOffset_mm.get());
}
