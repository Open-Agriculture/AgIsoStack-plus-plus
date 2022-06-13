#pragma once

#include "can_NAME.hpp"

namespace isobus
{

class NAMEFilter
{
public:

    NAMEFilter();

    NAME::NAMEParameters get_parameter() const;

    std::uint32_t get_value() const;

private:
    NAME::NAMEParameters parameter;
    std::uint32_t value;
};

} // namespace isobus
