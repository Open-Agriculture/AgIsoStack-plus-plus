#include <gtest/gtest.h>

#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"

#include <memory>

using namespace isobus;

TEST(CORE_TESTS, TestCreateAndDestroyPartners)
{
	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));

	isobus::PartneredControlFunction TestPartner1(0, vtNameFilters);
	isobus::PartneredControlFunction *TestPartner2 = new isobus::PartneredControlFunction(0, vtNameFilters);
	delete TestPartner2;
	auto TestPartner3 = std::make_shared<isobus::PartneredControlFunction>(0, vtNameFilters);
}

TEST(CORE_TESTS, TestCreateAndDestroyICFs)
{
	isobus::NAME TestDeviceNAME(0);
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(0);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(64);

	isobus::InternalControlFunction TestIcf1(TestDeviceNAME, 0x1C, 0);
	auto TestIcf2 = new isobus::InternalControlFunction(TestDeviceNAME, 0x80, 0);
	delete TestIcf2;
	auto TestIcf3 = std::make_shared<isobus::InternalControlFunction>(TestDeviceNAME, 0x81, 0);
}
