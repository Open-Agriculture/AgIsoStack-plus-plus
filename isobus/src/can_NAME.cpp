#include "can_NAME.hpp"

namespace isobus
{
	NAME::NAME(std::uint64_t rawNAMEData) :
	  rawName(rawNAMEData)
	{
	}

	bool NAME::get_arbitrary_address_capable() const
	{
		return (0 != (rawName >> 63));
	}

	void NAME::set_arbitrary_address_capable(bool value)
	{
		uint64_t tempNAME = rawName;
		tempNAME &= ~(static_cast<std::uint64_t>(1) << 63);
		tempNAME |= (static_cast<std::uint64_t>(value) << 63);
		set_full_name(tempNAME);
	}

	std::uint8_t NAME::get_industry_group() const
	{
		return ((rawName >> 60) & 0x07);
	}

	void NAME::set_industry_group(std::uint8_t value)
	{
		uint64_t tempNAME = rawName;
		tempNAME &= ~static_cast<std::uint64_t>(0x7000000000000000);
		tempNAME |= (static_cast<std::uint64_t>(value & 0x07) << 60);
		set_full_name(tempNAME);
	}

	std::uint8_t NAME::get_device_class_instance() const
	{
		return ((rawName >> 56) & 0x0F);
	}

	void NAME::set_device_class_instance(std::uint8_t value)
	{
		uint64_t tempNAME = rawName;
		tempNAME &= ~static_cast<std::uint64_t>(0xF00000000000000);
		tempNAME |= (static_cast<std::uint64_t>(value & 0x0F) << 56);
		set_full_name(tempNAME);
	}

	std::uint8_t NAME::get_device_class() const
	{
		return ((rawName >> 49) & 0x7F);
	}

	void NAME::set_device_class(std::uint8_t value)
	{
		uint64_t tempNAME = rawName;
		tempNAME &= ~static_cast<std::uint64_t>(0xFE000000000000);
		tempNAME |= (static_cast<std::uint64_t>(value & 0x7F) << 49);
		set_full_name(tempNAME);
	}

	std::uint8_t NAME::get_function_code() const
	{
		return ((rawName >> 40) & 0xFF);
	}

	void NAME::set_function_code(std::uint8_t value)
	{
		uint64_t tempNAME = rawName;
		tempNAME &= ~static_cast<std::uint64_t>(0xFF0000000000);
		tempNAME |= (static_cast<std::uint64_t>(value & 0xFF) << 40);
		set_full_name(tempNAME);
	}

	std::uint8_t NAME::get_function_instance() const
	{
		return ((rawName >> 35) & 0x1F);
	}

	void NAME::set_function_instance(std::uint8_t value)
	{
		uint64_t tempNAME = rawName;
		tempNAME &= ~static_cast<std::uint64_t>(0xF800000000);
		tempNAME |= (static_cast<std::uint64_t>(value & 0x1F) << 35);
		set_full_name(tempNAME);
	}

	std::uint8_t NAME::get_ecu_instance() const
	{
		return ((rawName >> 32) & 0x07);
	}

	void NAME::set_ecu_instance(std::uint8_t value)
	{
		uint64_t tempNAME = rawName;
		tempNAME &= ~static_cast<std::uint64_t>(0x700000000);
		tempNAME |= (static_cast<std::uint64_t>(value & 0x07) << 32);
		set_full_name(tempNAME);
	}

	std::uint16_t NAME::get_manufacturer_code() const
	{
		return ((rawName >> 21) & 0x07FF);
	}

	void NAME::set_manufacturer_code(std::uint16_t value)
	{
		uint64_t tempNAME = rawName;
		tempNAME &= ~static_cast<std::uint64_t>(0xFFE00000);
		tempNAME |= (static_cast<std::uint64_t>(value & 0x07FF) << 21);
		set_full_name(tempNAME);
	}

	std::uint32_t NAME::get_identity_number() const
	{
		return (rawName & 0x001FFFFF);
	}

	void NAME::set_identity_number(uint32_t value)
	{
		uint64_t tempNAME = rawName;
		tempNAME &= ~static_cast<std::uint64_t>(0x1FFFFF);
		tempNAME |= static_cast<std::uint64_t>(value & 0x1FFFFF);
		set_full_name(tempNAME);
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
