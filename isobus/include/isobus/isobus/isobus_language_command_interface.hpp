//================================================================================================
/// @file isobus_language_command_interface.hpp
///
/// @brief Defines a set of values found in the isobus language command message from
/// ISO11783-7 commonly used in VT and TC communication
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef ISOBUS_LANGUAGE_COMMAND_INTERFACE_HPP
#define ISOBUS_LANGUAGE_COMMAND_INTERFACE_HPP

#include "isobus/isobus/can_message.hpp"

#include <memory>

namespace isobus
{
	class InternalControlFunction;
	class PartneredControlFunction;
	class ControlFunction;

	/// @brief An interface for requesting and parsing the ISO11783 language
	/// command PGN, 0xFE0F.
	/// @details This is a class that's meant to provide an easy interface for
	/// dealing with the ISOBUS language command message.
	/// This is meant for use inside the VT Client and TC Client classes, however
	/// you can also use it standalone if you want.
	class LanguageCommandInterface
	{
	public:
		/// @brief Command sent to all CFs that determines whether a point or a comma will be
		/// displayed as the decimal symbol. (SPN 2411)
		enum class DecimalSymbols : std::uint8_t
		{
			Comma = 0, ///< A comma ',' is used
			Point = 1, ///< A decimal point '.' is used
			Reserved = 2, ///< Reserved
			NoAction = 3 ///< Take No Action
		};

		/// @brief Command sent to all CFs specifying the displayed format of the date. (SPN 2412)
		enum class DateFormats : std::uint8_t
		{
			ddmmyyyy = 0, ///< 31/12/2023
			ddyyyymm = 1, ///<  31/2023/12
			mmyyyydd = 2, ///< 12/2023/31
			mmddyyyy = 3, ///< 12/31/2023
			yyyymmdd = 4, ///< 2023/12/31
			yyyyddmm = 5, ///< 2023/31/12

			ReservedStart = 6, ///< Reserved range begins here
			ReservedEnd = 250 ///< Reserved range ends here
		};

		/// @brief Command sent to all CFs specifying the displayed format of the time. (SPN 2413)
		enum class TimeFormats : std::uint8_t
		{
			TwentyFourHour = 0, ///< 24 h
			TwelveHourAmPm = 1, ///< 12 h (am/pm)
			Reserved = 2, ///< Reserved
			NoAction = 3 ///< Take No Action
		};

		/// @brief Command specifying a distance unit. (SPN 2414)
		enum class DistanceUnits : std::uint8_t
		{
			Metric = 0, ///< Kilometers, meters
			ImperialUS = 1, ///< Miles, feet
			Reserved = 2, ///< Reserved
			NoAction = 3 ///< Take No Action
		};

		/// @brief Command specifying an area unit. (SPN 2415)
		enum class AreaUnits : std::uint8_t
		{
			Metric = 0, ///< Hectares or m^2
			ImperialUS = 1, ///< Acres or ft^2
			Reserved = 2, ///< Reserved
			NoAction = 3 ///< Take No Action
		};

		/// @brief Command specifying a volume unit. (SPN 2416)
		enum class VolumeUnits : std::uint8_t
		{
			Metric = 0, ///< Litre
			Imperial = 1, ///< Imperial Gallon
			US = 2, ///< US Gallon
			NoAction = 3 ///< Take No Action
		};

		/// @brief Command specifying a mass unit. (SPN 2417)
		enum class MassUnits : std::uint8_t
		{
			Metric = 0, ///< Tonnes, Kilograms
			Imperial = 1, ///< Long Tons, Pounds
			US = 2, ///< Short Tons, Pounds
			NoAction = 3 ///< Take No Action
		};

		/// @brief Command specifying a temperature unit. (SPN 5194)
		enum class TemperatureUnits : std::uint8_t
		{
			Metric = 0, ///< Degrees Celsius, Degrees Kelvin
			ImperialUS = 1, ///< Degrees Fahrenheit
			Reserved = 2, ///< Reserved
			NoAction = 3 ///< Take No Action
		};

		/// @brief Command specifying a pressure unit (SPN 5195)
		enum class PressureUnits : std::uint8_t
		{
			Metric = 0, ///< Kilopascals, pascals
			ImperialUS = 1, ///< Pounds per square inch
			Reserved = 2, ///< Reserved
			NoAction = 3 ///< Take No Action
		};

		/// @brief Command specifying a force unit (SPN 5196)
		enum class ForceUnits : std::uint8_t
		{
			Metric = 0, ///< Newtons
			ImperialUS = 1, ///< Pounds force
			Reserved = 2, ///< Reserved
			NoAction = 3 ///< Take No Action
		};

		/// @brief May be used for the display of any unit, or a unit other than
		/// those explicitly specified (SPN 5197)
		enum class UnitSystem : std::uint8_t
		{
			Metric = 0, ///< Generic metric
			Imperial = 1, ///< Generic imperial
			US = 2, ///< Generis US
			NoAction = 3 ///< Take No Action
		};

		/// @brief Constructor for a LanguageCommandInterface
		/// @details This constructor will make a version of the class that will accept the message from any source
		/// @param sourceControlFunction The internal control function that the interface should communicate from
		explicit LanguageCommandInterface(std::shared_ptr<InternalControlFunction> sourceControlFunction);

		/// @brief Constructor for a LanguageCommandInterface
		/// @details This constructor will make a version of the class that will filter the message to be
		/// only from the specified control function.
		/// @param sourceControlFunction The internal control function that the interface should communicate from
		/// @param filteredControlFunction The control function you want to explicitly communicate with
		LanguageCommandInterface(std::shared_ptr<InternalControlFunction> sourceControlFunction, std::shared_ptr<PartneredControlFunction> filteredControlFunction);

		/// @brief Deleted copy constructor for LanguageCommandInterface
		LanguageCommandInterface(LanguageCommandInterface &) = delete;

		/// @brief Destructor for the LanguageCommandInterface
		~LanguageCommandInterface();

		/// @brief Initializes the interface.
		/// @details This needs to be called before the interface is useable.
		/// It registers its PGN callback and sets up the PGN request interface
		/// if needed.
		void initialize();

		/// @brief Changes the partner being used by the interface to a new partner
		/// @param[in] filteredControlFunction The new partner to communicate with
		void set_partner(std::shared_ptr<PartneredControlFunction> filteredControlFunction);

		/// @brief Returns if initialize has been called yet
		/// @return `true` if initialize has been called, otherwise false
		bool get_initialized() const;

		/// @brief Sends a PGN request for the language command PGN to the interface's partner, or the global address
		/// depending on if you set a partner when constructing the object
		/// @return `true` if the message was sent, otherwise `false`
		bool send_request_language_command() const;

		/// @brief Returns the commanded language code parsed from the last language command
		/// @note If you do not support the returned language, your default shall be used
		/// @return The commanded language code (usually 2 characters length)
		std::string get_language_code() const;

		/// @brief Returns a timestamp (in ms) corresponding to when the interface last received a language command
		/// @return timestamp in milliseconds corresponding to when the interface last received a language command
		std::uint32_t get_language_command_timestamp() const;

		/// @brief Returns the commanded decimal symbol parsed from the last language command
		/// @return The decimal symbol that was last commanded
		DecimalSymbols get_commanded_decimal_symbol() const;

		/// @brief Returns the commanded time format parsed from the last language command
		/// @return The time format that was last commanded
		TimeFormats get_commanded_time_format() const;

		/// @brief Returns the commanded date format parsed from the last language command
		/// @return The date format that was last commanded
		DateFormats get_commanded_date_format() const;

		/// @brief Returns the commanded distance units parsed from the last language command
		/// @return The distance units that were last commanded
		DistanceUnits get_commanded_distance_units() const;

		/// @brief Returns the commanded area units parsed from the last received language command
		/// @return The area units that were last commanded
		AreaUnits get_commanded_area_units() const;

		/// @brief Returns the commanded volume units parsed from the last received language command
		/// @return The volume units that were last commanded
		VolumeUnits get_commanded_volume_units() const;

		/// @brief Returns the commanded mass units parsed from the last received language command
		/// @return The mass units that were last commanded
		MassUnits get_commanded_mass_units() const;

		/// @brief Returns the commanded temperature units parsed from the last received language command
		/// @return The temperature units that were last commanded
		TemperatureUnits get_commanded_temperature_units() const;

		/// @brief Returns the commanded pressure units parsed from the last received language command
		/// @return The pressure units that were last commanded
		PressureUnits get_commanded_pressure_units() const;

		/// @brief Returns the commanded force units parsed from the last received language command
		/// @return The force units that were last commanded
		ForceUnits get_commanded_force_units() const;

		/// @brief Returns the commanded "unit system" generic value that was parsed from the last received language command
		/// @return The commanded unit system
		UnitSystem get_commanded_generic_units() const;

		/// @brief Returns The raw bytes that comprise the current localization data as defined in ISO11783-7
		/// @returns The raw bytes that comprise the current localization data
		const std::array<std::uint8_t, 7> get_localization_raw_data() const;

		/// @brief Parses incoming CAN messages into usable unit and language settings
		/// @param message The CAN message to parse
		/// @param parentPointer A generic context variable, usually the `this` pointer for this interface instance
		static void process_rx_message(CANMessage *message, void *parentPointer);

	private:
		std::shared_ptr<InternalControlFunction> myControlFunction; ///< The control function to send messages as
		std::shared_ptr<PartneredControlFunction> myPartner; ///< The partner to talk to, or nullptr to listen to all CFs
		std::string languageCode; ///< The last received language code, such as "en", "es", "de", etc.
		std::uint32_t languageCommandTimestamp_ms = 0; ///< A millisecond timestamp correlated to the last received language command message
		DecimalSymbols decimalSymbol = DecimalSymbols::Point; ///< The decimal symbol that was commanded by the last language command message
		TimeFormats timeFormat = TimeFormats::TwelveHourAmPm; ///< The time format that was commanded by the last language command message
		DateFormats dateFormat = DateFormats::mmddyyyy; ///< The date format that was commanded by the last language command message
		DistanceUnits distanceUnitSystem = DistanceUnits::Metric; ///< The distance units that were commanded by the last language command message
		AreaUnits areaUnitSystem = AreaUnits::Metric; ///< The area units that were commanded by the last language command message
		VolumeUnits volumeUnitSystem = VolumeUnits::Metric; ///< The volume units that were commanded by the last language command message
		MassUnits massUnitSystem = MassUnits::Metric; ///< The mass units that were commanded by the last language command message
		TemperatureUnits temperatureUnitSystem = TemperatureUnits::Metric; ///< The temperature units that were commanded by the last language command message
		PressureUnits pressureUnitSystem = PressureUnits::Metric; ///< The pressure units that were commanded by the last language command message
		ForceUnits forceUnitSystem = ForceUnits::Metric; ///< The force units that were commanded by the last language command message
		UnitSystem genericUnitSystem = UnitSystem::Metric; ///< The "unit system" that was commanded by the last language command message
		bool initialized = false; ///< Tracks if initialize has been called yet for this interface
	};
} // namespace isobus

#endif // ISOBUS_LANGUAGE_COMMAND_INTERFACE_HPP
