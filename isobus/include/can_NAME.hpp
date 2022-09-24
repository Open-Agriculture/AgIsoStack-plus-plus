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
	//================================================================================================
	/// @class NAME
	///
	/// @brief A class that represents an ISO11783 control function NAME from an address claim.
	//================================================================================================
    class NAME
    {
    public:
		/// @brief The encoded components that comprise a NAME
        enum class NAMEParameters
        {
            IdentityNumber, ///< Usually the serial number of the ECU, unique for all similar control functions
            ManufacturerCode, ///< The J1939/ISO11783 manufacturer code of the ECU with this NAME
            EcuInstance, ///< The ECU instance of the ECU with this NAME. Usually increments in NAME order with similar CFs
            FunctionInstance, ///< The function instance of the ECU. Similar to Virtual Terminal number.
            FunctionCode, ///< The function of the ECU, as defined by ISO11783
            DeviceClass, ///< Also known as the vehicle system from J1939, describes general ECU type
            DeviceClassInstance, ///< The instance number of this device class
            IndustryGroup, ///< The industry group associated with this ECU, such as "agricultural"
            ArbitraryAddressCapable ///< Defines if this ECU supports address arbitration
        };

        /// @brief A structure that tracks the pair of a NAME parameter and associated value
        typedef std::pair<const NAMEParameters, const std::uint32_t> NameParameterFilter;

        /// @brief Constructor for a NAME
        /// @param[in] rawNAMEData The raw 64 bit NAME of an ECU
        explicit NAME(std::uint64_t rawNAMEData);

        /// @brief Returns if the ECU is capable of address arbitration
        /// @returns true if the ECU can arbitrate addresses
        bool get_arbitrary_address_capable() const;

        /// @brief Sets the data in the NAME that corresponds to the arbitration capability of the ECU
        /// @param[in] value True if the ECU supports arbitration, false if not
        void set_arbitrary_address_capable(bool value);

        /// @brief Returns the industry group encoded in the NAME
        /// @returns The industry group encoded in the NAME
        std::uint8_t get_industry_group() const;

        /// @brief Sets the industry group encoded in the NAME
        /// @param[in] value The industry group to encode in the NAME
        void set_industry_group(std::uint8_t value);

        /// @brief Returns the device class (vehicle system) encoded in the NAME
        /// @returns The device class (vehicle system) encoded in the NAME
        std::uint8_t get_device_class_instance() const;

        /// @brief Sets the device class instance (vehicle system) to be encoded in the NAME
        /// @param[in] value The device class instance (vehicle system) to be encoded in the NAME
        void set_device_class_instance(std::uint8_t value);

        /// @brief Returns the device class (vehicle system) encoded in the NAME
		/// @returns The device class (vehicle system) encoded in the NAME
        std::uint8_t get_device_class() const;

        /// @brief Sets the device class (vehicle system) to be encoded in the NAME
		/// @param[in] value The device class (vehicle system) to be encoded in the NAME
        void set_device_class(std::uint8_t value);

        /// @brief Gets the function code encoded in the NAME
        /// @returns The function code encoded in the NAME
        std::uint8_t get_function_code() const;

        /// @brief Sets the function code encoded in the NAME
		/// @param[in] value The function code to be encoded in the NAME
        void set_function_code(std::uint8_t value);

        /// @brief Gets the function instance encoded in the NAME
		/// @returns The function instance encoded in the NAME
        std::uint8_t get_function_instance() const;

        /// @brief Sets the function instance encoded in the NAME
		/// @param[in] value The function instance to be encoded in the NAME
        void set_function_instance(std::uint8_t value);

        /// @brief Gets the ecu instance encoded in the NAME
		/// @returns The ecu instance encoded in the NAME
        std::uint8_t get_ecu_instance() const;

        /// @brief Sets the ecu instance encoded in the NAME
		/// @param[in] value The ecu instance to be encoded in the NAME
        void set_ecu_instance(std::uint8_t value);

        /// @brief Gets the manufacturer code encoded in the NAME
		/// @returns The manufacturer code encoded in the NAME
        std::uint16_t get_manufacturer_code() const;

        /// @brief Sets the manufacturer code encoded in the NAME
		/// @param[in] value The manufacturer code to be encoded in the NAME
        void set_manufacturer_code(std::uint16_t value);

        /// @brief Gets the identity number encoded in the NAME
		/// @returns The identity number encoded in the NAME
        std::uint32_t get_identity_number() const;

        /// @brief Sets the identity number encoded in the NAME
		/// @param[in] value The identity number to be encoded in the NAME
        void set_identity_number(std::uint32_t value);

        /// @brief Gets the raw 64 bit NAME
		/// @returns The raw 64 bit NAME
        std::uint64_t get_full_name() const;

        /// @brief Sets the raw, encoded 64 bit NAME
		/// @param[in] value The raw, encoded 64 bit NAME
        void set_full_name(std::uint64_t value);
    private:
        std::uint64_t rawName; ///< A raw, 64 bit NAME encoded with all NAMEParameters
    };
} // namespace isobus

#endif // CAN_NAME_HPP
