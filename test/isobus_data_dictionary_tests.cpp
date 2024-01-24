#include <gtest/gtest.h>

#include "isobus/isobus/isobus_data_dictionary.hpp"

using namespace isobus;

TEST(DATA_DICTIONARY_TESTS, DDI_Lookups)
{
	auto testEntry = DataDictionary::get_entry(229); // Test "actual net weight"

	EXPECT_EQ(229, testEntry.ddi);
	EXPECT_EQ("Actual Net Weight", testEntry.name);
	EXPECT_EQ(1, testEntry.resolution);
	EXPECT_EQ("g - Mass large", testEntry.units);

	auto testEntry2 = DataDictionary::get_entry(40962); // Test  40962 - Default Crop Grade Length

	EXPECT_EQ(40962, testEntry2.ddi);
	EXPECT_EQ("Default Crop Grade Length", testEntry2.name);
	EXPECT_NEAR(0.001, testEntry2.resolution, 0.001);
	EXPECT_EQ("mm - Length", testEntry2.units);

	// Test an invalid, random ddi
	auto testEntry3 = DataDictionary::get_entry(1957);
	EXPECT_EQ(65535, testEntry3.ddi);
	EXPECT_EQ("Unknown", testEntry3.name);
	EXPECT_EQ(0, testEntry3.resolution);
	EXPECT_EQ("Unknown", testEntry3.units);
}
