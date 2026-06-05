//================================================================================================
/// @file test_fixture.hpp
///
/// @brief Provides common setup code for unit tests
/// @author Adrian Del Grosso
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef TEST_FIXTURE_HPP
#define TEST_FIXTURE_HPP

#include <gtest/gtest.h>

#include "isobus/utility/system_timing.hpp"
#include "test_time_source.hpp"

//namespace test_helpers

class AgIsoStackTestFixture : public ::testing::Test
{
public:
	void SetUp() override
	{
		// Code here runs BEFORE every TEST_F(MyTestFixture, ...)
		isobus::SystemTiming::override_time_source(&time_source);
	}

	void TearDown() override
	{
		isobus::SystemTiming::override_time_source(nullptr);
	}

	test_helpers::TestTimeSource time_source;
};

#endif // TEST_TIME_SOURCE_HPP
