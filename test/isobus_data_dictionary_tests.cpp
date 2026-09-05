#include <gtest/gtest.h>

#include "isobus/isobus/isobus_data_dictionary.hpp"

using namespace isobus;

TEST(DATA_DICTIONARY_TESTS, DDI_Lookups)
{
	// Test "actual net weight"
	auto testEntry = DataDictionary::get_entry(229);

	EXPECT_EQ(229, testEntry.ddi);
	EXPECT_EQ("Actual Net Weight", testEntry.name);
	EXPECT_EQ(1, testEntry.resolution);
	EXPECT_EQ("g", testEntry.unitSymbol);
	EXPECT_EQ("Mass large", testEntry.unitDescription);
	EXPECT_NEAR(-2147483648.0f, testEntry.displayRange.first, 0.001);
	EXPECT_NEAR(2147483647.0f, testEntry.displayRange.second, 0.001);

	// Test  40962 - Default Crop Grade Length
	auto testEntry2 = DataDictionary::get_entry(40962);
	EXPECT_EQ(40962, testEntry2.ddi);
	EXPECT_EQ("Default Crop Grade Length", testEntry2.name);
	EXPECT_NEAR(0.001, testEntry2.resolution, 0.001);
	EXPECT_EQ("mm", testEntry2.unitSymbol);
	EXPECT_EQ("Length", testEntry2.unitDescription);
	EXPECT_NEAR(0.0f, testEntry2.displayRange.first, 0.001);
	EXPECT_NEAR(2147483647.0f, testEntry2.displayRange.second, 0.001);

	// Test a DDI published since the previous snapshot
	auto testEntry3 = DataDictionary::get_entry(695);
	EXPECT_EQ(695, testEntry3.ddi);
	EXPECT_EQ("Mesh Total Used", testEntry3.name);
	EXPECT_NEAR(1.0f, testEntry3.resolution, 0.001);
	EXPECT_EQ("mm", testEntry3.unitSymbol);
	EXPECT_EQ("Length", testEntry3.unitDescription);
	EXPECT_NEAR(0.0f, testEntry3.displayRange.first, 0.001);
	EXPECT_NEAR(2147483647.0f, testEntry3.displayRange.second, 0.001);

	// Test a DDI renamed in the latest ISO database revision
	auto testEntry4 = DataDictionary::get_entry(505);
	EXPECT_EQ(505, testEntry4.ddi);
	EXPECT_EQ("Supported Track Control Levels", testEntry4.name);

	// The complete export includes entries that are not yet ISO-published; preserve them
	auto testEntry5 = DataDictionary::get_entry(730);
	EXPECT_EQ(730, testEntry5.ddi);
	EXPECT_EQ("Actual Tare Weight", testEntry5.name);

	// Test an invalid, random ddi
	auto testEntry6 = DataDictionary::get_entry(1957);
	EXPECT_EQ(65535, testEntry6.ddi);
	EXPECT_EQ("Unknown", testEntry6.name);
	EXPECT_EQ(0, testEntry6.resolution);
	EXPECT_EQ("Unknown", testEntry6.unitSymbol);
	EXPECT_EQ("Unknown", testEntry6.unitDescription);
	EXPECT_EQ(0.0f, testEntry6.displayRange.first);
	EXPECT_EQ(0.0f, testEntry6.displayRange.second);
}
