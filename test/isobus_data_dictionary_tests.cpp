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

	auto testEntry3 = DataDictionary::get_entry(695);
	EXPECT_EQ(695, testEntry3.ddi);
	EXPECT_EQ("Mesh Total Used", testEntry3.name);
	EXPECT_NEAR(1.0f, testEntry3.resolution, 0.001);
	EXPECT_EQ("mm", testEntry3.unitSymbol);
	EXPECT_EQ("Length", testEntry3.unitDescription);
	EXPECT_NEAR(0.0f, testEntry3.displayRange.first, 0.001);
	EXPECT_NEAR(2147483647.0f, testEntry3.displayRange.second, 0.001);

	auto testEntry4 = DataDictionary::get_entry(505);
	EXPECT_EQ(505, testEntry4.ddi);
	EXPECT_EQ("Supported Track Control Levels", testEntry4.name);

	// The complete export includes entries that are not yet ISO-published; preserve them
	auto testEntry5 = DataDictionary::get_entry(730);
	EXPECT_EQ(730, testEntry5.ddi);
	EXPECT_EQ("Actual Tare Weight", testEntry5.name);

	auto testEntry6 = DataDictionary::get_entry(740);
	EXPECT_EQ(740, testEntry6.ddi);
	EXPECT_EQ("Maximum Mechanical Power", testEntry6.name);
	EXPECT_NEAR(0.1f, testEntry6.resolution, 0.001);
	EXPECT_EQ("W", testEntry6.unitSymbol);
	EXPECT_EQ("Mechanical Power", testEntry6.unitDescription);
	EXPECT_NEAR(-214748364.8f, testEntry6.displayRange.first, 0.001);
	EXPECT_NEAR(214748364.7f, testEntry6.displayRange.second, 0.001);

	// Test an invalid, random ddi
	auto testEntry7 = DataDictionary::get_entry(1957);
	EXPECT_EQ(65535, testEntry7.ddi);
	EXPECT_EQ("Unknown", testEntry7.name);
	EXPECT_EQ(0, testEntry7.resolution);
	EXPECT_EQ("Unknown", testEntry7.unitSymbol);
	EXPECT_EQ("Unknown", testEntry7.unitDescription);
	EXPECT_EQ(0.0f, testEntry7.displayRange.first);
	EXPECT_EQ(0.0f, testEntry7.displayRange.second);
}

TEST(DATA_DICTIONARY_TESTS, TrackControlLevelFormatting)
{
	EXPECT_EQ("No levels supported", DataDictionary::format_value_with_ddi(505, 0x00));
	EXPECT_EQ("Level 1", DataDictionary::format_value_with_ddi(505, 0x01));
	EXPECT_EQ("Level 2", DataDictionary::format_value_with_ddi(505, 0x02));
	EXPECT_EQ("Level 1, Level 2", DataDictionary::format_value_with_ddi(505, 0x03));
	EXPECT_EQ("Level 3", DataDictionary::format_value_with_ddi(505, 0x04));
	EXPECT_EQ("Level 1, Level 3", DataDictionary::format_value_with_ddi(505, 0x05));
	EXPECT_EQ("Level 2, Level 3", DataDictionary::format_value_with_ddi(505, 0x06));
	EXPECT_EQ("Level 1, Level 2, Level 3", DataDictionary::format_value_with_ddi(505, 0x07));
	EXPECT_EQ("Reserved", DataDictionary::format_value_with_ddi(505, 0x08));
	EXPECT_EQ("Reserved", DataDictionary::format_value_with_ddi(505, -1));

	EXPECT_EQ("No common level", DataDictionary::format_value_with_ddi(506, 0x00));
	EXPECT_EQ("Level 1", DataDictionary::format_value_with_ddi(506, 0x01));
	EXPECT_EQ("Level 2", DataDictionary::format_value_with_ddi(506, 0x02));
	EXPECT_EQ("Level 3", DataDictionary::format_value_with_ddi(506, 0x03));
	EXPECT_EQ("Reserved", DataDictionary::format_value_with_ddi(506, 0x04));
	EXPECT_EQ("Reserved", DataDictionary::format_value_with_ddi(506, -1));
}

TEST(DATA_DICTIONARY_TESTS, EnumeratedValueFormatting)
{
	EXPECT_EQ("Auto/on", DataDictionary::format_value_with_ddi(160, 0x01));
	EXPECT_EQ("Clear", DataDictionary::format_value_with_ddi(210, 0x20524C43));
	EXPECT_EQ("Function: Unknown, SubType: 0 (N/A)", DataDictionary::format_value_with_ddi(350, 0));
	EXPECT_EQ("Slightly moist", DataDictionary::format_value_with_ddi(469, 0x02));
	EXPECT_EQ("Not supported", DataDictionary::format_value_with_ddi(515, 0x03));
	EXPECT_EQ("no rain in the last week", DataDictionary::format_value_with_ddi(587, 0x02));
	EXPECT_EQ("Very fine", DataDictionary::format_value_with_ddi(36864, 0x02));
	EXPECT_EQ("No droplet size available", DataDictionary::format_value_with_ddi(36864, 0xFF));
}
