//================================================================================================
/// @file can_NAME.hpp
///
/// @brief A class that represents a control function's NAME
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_NAME_HPP
#define CAN_NAME_HPP

#include <cstdint>
#include <tuple>

namespace isobus
{
    class NAME
    {
    public:
        enum class NAMEParameters
        {
            IdentityNumber,
            ManufacturerCode,
            EcuInstance,
            FunctionInstance,
            FunctionCode,
            DeviceClass,
            DeviceClassInstance,
            IndustryGroup,
            ArbitraryAddressCapable
        };

        typedef std::pair<const NAMEParameters, const std::uint32_t> NameParameterFilter;

        explicit NAME(std::uint64_t rawNAMEData);

        bool get_arbitrary_address_capable() const;
        void set_arbitrary_address_capable(bool value);

        std::uint8_t get_industry_group() const;
        void set_industry_group(std::uint8_t value);

        std::uint8_t get_device_class_instance() const;
        void set_device_class_instance(std::uint8_t value);

        std::uint8_t get_device_class() const;
        void set_device_class(std::uint8_t value);

        std::uint8_t get_function_code() const;
        void set_function_code(std::uint8_t value);

        std::uint8_t get_function_instance() const;
        void set_function_instance(std::uint8_t value);

        std::uint8_t get_ecu_instance() const;
        void set_ecu_instance(std::uint8_t value);

        std::uint16_t get_manufacturer_code() const;
        void set_manufacturer_code(std::uint16_t value);

        std::uint32_t get_identity_number() const;
        void set_identity_number(std::uint32_t value);

        std::uint64_t get_full_name() const;
        void set_full_name(std::uint64_t value);
    private:
        std::uint64_t rawName;
    };
} // namespace isobus

#endif // CAN_NAME_HPP
