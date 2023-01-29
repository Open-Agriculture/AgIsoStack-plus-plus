#include <gtest/gtest.h>

#include "isobus/isobus/can_NAME_filter.hpp"
#include "isobus/isobus/can_constants.hpp"

#include <chrono>
#include <thread>

using namespace isobus;

TEST(CAN_NAME_TESTS, NAMEProperties)
{
	NAME TestDeviceNAME;
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(1);
	TestDeviceNAME.set_device_class(2);
	TestDeviceNAME.set_function_code(3);
	TestDeviceNAME.set_identity_number(4);
	TestDeviceNAME.set_ecu_instance(5);
	TestDeviceNAME.set_function_instance(6);
	TestDeviceNAME.set_device_class_instance(7);
	TestDeviceNAME.set_manufacturer_code(8);

	EXPECT_EQ(TestDeviceNAME.get_arbitrary_address_capable(), true);
	EXPECT_EQ(TestDeviceNAME.get_industry_group(), 1);
	EXPECT_EQ(TestDeviceNAME.get_device_class(), 2);
	EXPECT_EQ(TestDeviceNAME.get_function_code(), 3);
	EXPECT_EQ(TestDeviceNAME.get_identity_number(), 4);
	EXPECT_EQ(TestDeviceNAME.get_ecu_instance(), 5);
	EXPECT_EQ(TestDeviceNAME.get_function_instance(), 6);
	EXPECT_EQ(TestDeviceNAME.get_device_class_instance(), 7);
	EXPECT_EQ(TestDeviceNAME.get_manufacturer_code(), 8);
	EXPECT_EQ(TestDeviceNAME.get_full_name(), 10881826125818888196U);
}

TEST(CAN_NAME_TESTS, NAMEPropertiesOutOfRange)
{
	NAME TestDeviceNAME;
	TestDeviceNAME.set_industry_group(8);
	TestDeviceNAME.set_device_class_instance(16);
	TestDeviceNAME.set_device_class(128);
	TestDeviceNAME.set_identity_number(2097152);
	TestDeviceNAME.set_ecu_instance(8);
	TestDeviceNAME.set_function_instance(32);
	TestDeviceNAME.set_manufacturer_code(2048);

	EXPECT_NE(TestDeviceNAME.get_industry_group(), 8);
	EXPECT_NE(TestDeviceNAME.get_device_class_instance(), 16);
	EXPECT_NE(TestDeviceNAME.get_device_class(), 128);
	EXPECT_NE(TestDeviceNAME.get_identity_number(), 2097151);
	EXPECT_NE(TestDeviceNAME.get_ecu_instance(), 8);
	EXPECT_NE(TestDeviceNAME.get_function_instance(), 32);
	EXPECT_NE(TestDeviceNAME.get_manufacturer_code(), 2048);
}

TEST(CAN_NAME_TESTS, NAMEEquals)
{
	NAME firstNAME(10376445291390828545U);
	NAME secondNAME(10376445291390828545U);
	EXPECT_EQ(firstNAME, secondNAME);
}

TEST(CAN_NAME_TESTS, FilterProperties)
{
	NAMEFilter TestFilter(NAME::NAMEParameters::IdentityNumber, 69);
	EXPECT_EQ(TestFilter.get_parameter(), NAME::NAMEParameters::IdentityNumber);
	EXPECT_EQ(TestFilter.get_value(), 69);
}

TEST(CAN_NAME_TESTS, FilterMatches)
{
	NAMEFilter filterIdentityNumber(NAME::NAMEParameters::IdentityNumber, 1);
	NAME TestDeviceNAME(0);
	TestDeviceNAME.set_identity_number(1);
	EXPECT_TRUE(filterIdentityNumber.check_name_matches_filter(TestDeviceNAME));

	NAMEFilter filterManufacturerCode(NAME::NAMEParameters::ManufacturerCode, 2);
	TestDeviceNAME.set_manufacturer_code(2);
	EXPECT_TRUE(filterManufacturerCode.check_name_matches_filter(TestDeviceNAME));

	NAMEFilter filterECUInstance(NAME::NAMEParameters::EcuInstance, 3);
	TestDeviceNAME.set_ecu_instance(3);
	EXPECT_TRUE(filterECUInstance.check_name_matches_filter(TestDeviceNAME));

	NAMEFilter filterFunctionInstance(NAME::NAMEParameters::FunctionInstance, 4);
	TestDeviceNAME.set_function_instance(4);
	EXPECT_TRUE(filterFunctionInstance.check_name_matches_filter(TestDeviceNAME));

	NAMEFilter filterFunctionCode(NAME::NAMEParameters::FunctionCode, 5);
	TestDeviceNAME.set_function_code(5);
	EXPECT_TRUE(filterFunctionCode.check_name_matches_filter(TestDeviceNAME));

	NAMEFilter filterDeviceClass(NAME::NAMEParameters::DeviceClass, 6);
	TestDeviceNAME.set_device_class(6);
	EXPECT_TRUE(filterDeviceClass.check_name_matches_filter(TestDeviceNAME));

	NAMEFilter filterIndustryGroup(NAME::NAMEParameters::IndustryGroup, 7);
	TestDeviceNAME.set_industry_group(7);
	EXPECT_TRUE(filterIndustryGroup.check_name_matches_filter(TestDeviceNAME));

	NAMEFilter filterDeviceClassInstance(NAME::NAMEParameters::DeviceClassInstance, 8);
	TestDeviceNAME.set_device_class_instance(8);
	EXPECT_TRUE(filterDeviceClassInstance.check_name_matches_filter(TestDeviceNAME));

	NAMEFilter filterArbitraryAddressCapable(NAME::NAMEParameters::ArbitraryAddressCapable, true);
	TestDeviceNAME.set_arbitrary_address_capable(true);
	EXPECT_TRUE(filterArbitraryAddressCapable.check_name_matches_filter(TestDeviceNAME));
}