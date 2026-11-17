// Copyright 2026 Mauricio. Distributed under the MIT license.
/// @cond INTERNAL_CANDLELIGHT_TIMING
#ifndef CANDLELIGHT_BITTIMING_HPP
#define CANDLELIGHT_BITTIMING_HPP

#include <algorithm>
#include <cstdint>
#include <limits>

namespace isobus
{
	namespace detail
	{
		struct BitTiming
		{
			std::uint32_t propSeg, phaseSeg1, phaseSeg2, sjw, brp;
		};
		struct BitTimingConst
		{
			std::uint32_t feature, fclkCan, tseg1Min, tseg1Max, tseg2Min, tseg2Max;
			std::uint32_t sjwMax, brpMin, brpMax, brpInc;
		};

		// Find an exact bitrate within device limits, preferring an 87.5% sample point.
		inline bool calculate_bit_timing(const BitTimingConst &caps, std::uint32_t bitrate, BitTiming &result)
		{
			if (!bitrate || !caps.fclkCan || caps.fclkCan % bitrate ||
			    !caps.brpMin || !caps.brpInc || !caps.sjwMax || !caps.tseg1Min || !caps.tseg2Min ||
			    caps.brpMin > caps.brpMax || caps.tseg1Min > caps.tseg1Max || caps.tseg2Min > caps.tseg2Max)
			{
				return false;
			}
			const std::uint64_t ticks = caps.fclkCan / bitrate;
			const std::uint64_t maxBrp = std::min<std::uint64_t>(caps.brpMax, ticks / 3);
			std::uint64_t bestError = std::numeric_limits<std::uint64_t>::max();
			std::uint64_t bestTotal = 1;
			bool found = false;
			for (std::uint64_t brp = caps.brpMin; brp <= maxBrp; brp += caps.brpInc)
			{
				if (ticks % brp)
				{
					continue;
				}
				const std::uint64_t total = ticks / brp;
				if (total < 1ull + caps.tseg1Min + caps.tseg2Min ||
				    total > 1ull + caps.tseg1Max + caps.tseg2Max)
				{
					continue;
				}
				const auto minTseg2 = std::max<std::uint64_t>(caps.tseg2Min,
				                                              total > 1ull + caps.tseg1Max ? total - 1 - caps.tseg1Max : 0);
				const auto maxTseg2 = std::min<std::uint64_t>(caps.tseg2Max, total - 1 - caps.tseg1Min);
				const auto tseg2 = std::max(minTseg2, std::min((total + 4) / 8, maxTseg2));
				const auto error = tseg2 * 8 > total ? tseg2 * 8 - total : total - tseg2 * 8;
				if (!found || error * bestTotal < bestError * total ||
				    (error * bestTotal == bestError * total && total > bestTotal))
				{
					const auto tseg1 = static_cast<std::uint32_t>(total - 1 - tseg2);
					result = { 1, tseg1 - 1, static_cast<std::uint32_t>(tseg2), 1, static_cast<std::uint32_t>(brp) };
					bestError = error;
					bestTotal = total;
					found = true;
				}
			}
			return found;
		}
	}
}
#endif
/// @endcond
