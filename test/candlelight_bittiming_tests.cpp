// Copyright 2026 Mauricio. Distributed under the MIT license.
#include <gtest/gtest.h>
#include "candlelight_bittiming.hpp"
#include "isobus/hardware_integration/available_can_drivers.hpp"

#ifdef ISOBUS_WINDOWSCANDLELIGHT_AVAILABLE
// Exercise the public header, exported availability flag and WinUSB linkage
// without opening an adapter or sending a CAN frame.
TEST(CANDLELIGHT_DRIVER, DisconnectedLifecycle)
{
	isobus::CandleLightWindowsPlugin plugin;
	EXPECT_FALSE(plugin.get_is_valid());
	EXPECT_FALSE(plugin.get_name().empty());
	isobus::CANMessageFrame frame{};
	frame.identifier = 0x123;
	EXPECT_FALSE(plugin.read_frame(frame));
	EXPECT_EQ(0x123u, frame.identifier);
	EXPECT_FALSE(plugin.write_frame(frame));
	plugin.close();
	plugin.close();
	EXPECT_FALSE(plugin.get_is_valid());
}
#endif

namespace
{
	void check_timing(const isobus::detail::BitTimingConst &caps, std::uint32_t bitrate)
	{
		isobus::detail::BitTiming timing{};
		ASSERT_TRUE(isobus::detail::calculate_bit_timing(caps, bitrate, timing));
		const auto tseg1 = timing.propSeg + timing.phaseSeg1;
		EXPECT_GE(tseg1, caps.tseg1Min);
		EXPECT_LE(tseg1, caps.tseg1Max);
		EXPECT_GE(timing.phaseSeg2, caps.tseg2Min);
		EXPECT_LE(timing.phaseSeg2, caps.tseg2Max);
		EXPECT_GE(timing.brp, caps.brpMin);
		EXPECT_LE(timing.brp, caps.brpMax);
		EXPECT_EQ(0u, (timing.brp - caps.brpMin) % caps.brpInc);
		EXPECT_GT(timing.sjw, 0u);
		EXPECT_LE(timing.sjw, caps.sjwMax);
		EXPECT_EQ(caps.fclkCan, static_cast<std::uint64_t>(bitrate) * timing.brp * (1 + tseg1 + timing.phaseSeg2));
	}
}

TEST(CANDLELIGHT_TIMING, SupportedClocksAndSamplePoint)
{
	isobus::detail::BitTimingConst caps{ 0, 48000000, 1, 16, 1, 8, 4, 1, 1024, 1 };
	isobus::detail::BitTiming timing{};
	ASSERT_TRUE(isobus::detail::calculate_bit_timing(caps, 250000, timing));
	EXPECT_EQ(7 * (1 + timing.propSeg + timing.phaseSeg1 + timing.phaseSeg2),
	          8 * (1 + timing.propSeg + timing.phaseSeg1));
	for (auto clock : { 42000000u, 48000000u, 80000000u, 120000000u, 170000000u })
	{
		caps.fclkCan = clock;
		check_timing(caps, 250000);
	}
}

TEST(CANDLELIGHT_TIMING, PrescalerStepsAndImpossibleRates)
{
	isobus::detail::BitTimingConst caps{ 0, 48000000, 1, 16, 1, 8, 4, 2, 1024, 2 };
	isobus::detail::BitTiming timing{};
	check_timing(caps, 250000);
	EXPECT_FALSE(isobus::detail::calculate_bit_timing(caps, 0, timing));
	EXPECT_FALSE(isobus::detail::calculate_bit_timing(caps, 333333, timing));
	caps.brpMin = 7;
	caps.brpMax = 7;
	EXPECT_FALSE(isobus::detail::calculate_bit_timing(caps, 250000, timing));
}

TEST(CANDLELIGHT_TIMING, RejectInvalidCapabilities)
{
	const isobus::detail::BitTimingConst valid{ 0, 48000000, 1, 16, 1, 8, 4, 1, 1024, 1 };
	isobus::detail::BitTiming timing{};
	for (auto value : { 0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u })
	{
		auto caps = valid;
		switch (value)
		{
			case 0:
				caps.fclkCan = 0;
				break;
			case 1:
				caps.brpMin = 0;
				break;
			case 2:
				caps.brpInc = 0;
				break;
			case 3:
				caps.sjwMax = 0;
				break;
			case 4:
				caps.tseg1Min = 0;
				break;
			case 5:
				caps.tseg2Min = 0;
				break;
			case 6:
				caps.brpMin = caps.brpMax + 1;
				break;
			case 7:
				caps.tseg1Min = caps.tseg1Max + 1;
				break;
		}
		EXPECT_FALSE(isobus::detail::calculate_bit_timing(caps, 250000, timing));
	}
	auto caps = valid;
	caps.tseg2Min = caps.tseg2Max + 1;
	EXPECT_FALSE(isobus::detail::calculate_bit_timing(caps, 250000, timing));
}
