//================================================================================================
/// @file can_NAME.cpp
///
/// @brief A class that represents a control function's NAME
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/can_NAME.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

namespace isobus
{
	NAME::NAME(std::uint64_t rawNAMEData) :
	  rawName(rawNAMEData)
	{
	}

	bool NAME::operator==(const NAME &obj) const
	{
		return this->rawName == obj.rawName;
	}

	bool NAME::get_arbitrary_address_capable() const
	{
		return (0 != (rawName >> 63));
	}

	void NAME::set_arbitrary_address_capable(bool value)
	{
		rawName &= ~(static_cast<std::uint64_t>(1) << 63);
		rawName |= (static_cast<std::uint64_t>(value) << 63);
	}

	std::uint8_t NAME::get_industry_group() const
	{
		return ((rawName >> 60) & 0x07);
	}

	void NAME::set_industry_group(std::uint8_t value)
	{
		if (value > 0x07)
		{
			CANStackLogger::error("[NAME]: Industry group out of range, must be between 0 and 7");
		}
		rawName &= ~static_cast<std::uint64_t>(0x7000000000000000);
		rawName |= (static_cast<std::uint64_t>(value & 0x07) << 60);
	}

	std::uint8_t NAME::get_device_class_instance() const
	{
		return ((rawName >> 56) & 0x0F);
	}

	void NAME::set_device_class_instance(std::uint8_t value)
	{
		if (value > 0x0F)
		{
			CANStackLogger::error("[NAME]: Device class instance out of range, must be between 0 and 15");
		}
		rawName &= ~static_cast<std::uint64_t>(0xF00000000000000);
		rawName |= (static_cast<std::uint64_t>(value & 0x0F) << 56);
	}

	std::uint8_t NAME::get_device_class() const
	{
		return ((rawName >> 49) & 0x7F);
	}

	void NAME::set_device_class(std::uint8_t value)
	{
		if (value > 0x7F)
		{
			CANStackLogger::error("[NAME]: Device class out of range, must be between 0 and 127");
		}
		rawName &= ~static_cast<std::uint64_t>(0xFE000000000000);
		rawName |= (static_cast<std::uint64_t>(value & 0x7F) << 49);
	}

	std::uint8_t NAME::get_function_code() const
	{
		return ((rawName >> 40) & 0xFF);
	}

	void NAME::set_function_code(std::uint8_t value)
	{
		rawName &= ~static_cast<std::uint64_t>(0xFF0000000000);
		rawName |= (static_cast<std::uint64_t>(value & 0xFF) << 40);
	}

	std::uint8_t NAME::get_function_instance() const
	{
		return ((rawName >> 35) & 0x1F);
	}

	void NAME::set_function_instance(std::uint8_t value)
	{
		if (value > 0x1F)
		{
			CANStackLogger::error("[NAME]: Function instance out of range, must be between 0 and 31");
		}
		rawName &= ~static_cast<std::uint64_t>(0xF800000000);
		rawName |= (static_cast<std::uint64_t>(value & 0x1F) << 35);
	}

	std::uint8_t NAME::get_ecu_instance() const
	{
		return ((rawName >> 32) & 0x07);
	}

	void NAME::set_ecu_instance(std::uint8_t value)
	{
		if (value > 0x07)
		{
			CANStackLogger::error("[NAME]: ECU instance out of range, must be between 0 and 7");
		}
		rawName &= ~static_cast<std::uint64_t>(0x700000000);
		rawName |= (static_cast<std::uint64_t>(value & 0x07) << 32);
	}

	std::uint16_t NAME::get_manufacturer_code() const
	{
		return ((rawName >> 21) & 0x07FF);
	}

	void NAME::set_manufacturer_code(std::uint16_t value)
	{
		if (value > 0x07FF)
		{
			CANStackLogger::error("[NAME]: Manufacturer code out of range, must be between 0 and 2047");
		}
		rawName &= ~static_cast<std::uint64_t>(0xFFE00000);
		rawName |= (static_cast<std::uint64_t>(value & 0x07FF) << 21);
	}

	std::uint32_t NAME::get_identity_number() const
	{
		return (rawName & 0x001FFFFF);
	}

	void NAME::set_identity_number(uint32_t value)
	{
		if (value > 0x001FFFFF)
		{
			CANStackLogger::error("[NAME]: Identity number out of range, must be between 0 and 2097151");
		}
		rawName &= ~static_cast<std::uint64_t>(0x1FFFFF);
		rawName |= static_cast<std::uint64_t>(value & 0x1FFFFF);
	}

	std::uint64_t NAME::get_full_name() const
	{
		return rawName;
	}

	void NAME::set_full_name(std::uint64_t value)
	{
		rawName = value;
	}

} // namespace isobus
