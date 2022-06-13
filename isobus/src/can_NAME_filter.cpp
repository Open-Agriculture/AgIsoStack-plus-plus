#include "can_NAME_filter.hpp"

namespace isobus
{
	NAMEFilter::NAMEFilter()
	{
	}

	NAME::NAMEParameters NAMEFilter::get_parameter() const
	{
		return parameter;
	}

	std::uint32_t NAMEFilter::get_value() const
	{
		return value;
	}

} // namespace isobus