//================================================================================================
/// @file nmea2000_message_definitions.hpp
///
/// @brief This file contains class definitions that will comprise the individual components
/// of the NMEA2000 message interface for the stack. Generally this separation exists to keep
/// the file size of nmea2000_message_interface.hpp/cpp smaller.
///
/// @note This library and its authors are not affiliated with the National Marine
/// Electronics Association in any way.
///
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef NMEA2000_MESSAGE_DEFINITIONS_HPP
#define NMEA2000_MESSAGE_DEFINITIONS_HPP

#include "isobus/isobus/can_internal_control_function.hpp"

namespace isobus
{
	namespace NMEA2000Messages
	{
		class VesselHeading
		{
		public:
			enum class HeadingSensorReference : std::uint8_t
			{
				True = 0,
				Magnetic = 1,
				Error = 2,
				NotApplicableOrNull = 3
			};

			explicit VesselHeading(std::shared_ptr<ControlFunction> source);

			std::shared_ptr<ControlFunction> get_control_function() const;

			std::uint32_t get_timestamp() const;

			bool set_timestamp(std::uint32_t timestamp);

			std::uint16_t get_heading() const;

			bool set_heading(std::uint16_t heading);

			std::int16_t get_magnetic_deviation() const;

			bool set_magnetic_deviation(std::uint16_t deviation);

			std::int16_t get_magnetic_variation() const;

			bool set_magnetic_variation(std::int16_t variation);

			std::uint8_t get_sequence_id() const;

			bool set_sequence_id(std::uint8_t sequenceNumber);

			HeadingSensorReference get_sensor_reference() const;

			bool set_sensor_reference(HeadingSensorReference reference);

		private:
			static constexpr std::uint32_t CYCLIC_MESSAGE_RATE_MS = 100;

			std::shared_ptr<ControlFunction> senderControlFunction; ///< The sender of the message data
			std::uint32_t messageTimestamp_ms = 0; ///< A timestamp in milliseconds when this message was last sent or received
			std::uint16_t headingReading = 0; ///< The raw heading in 0.0001 radians, relative to the indicated HeadingSensorReference.
			std::int16_t magneticDeviation = 0; ///< The magnetic deviation if not included in the reading in 0.0001 radians. Positive values are easterly.
			std::int16_t magneticVariation = 0; ///< The magnetic variation if applicable in 0.0001 radians. Positive values are easterly. If the reference is magnetic, you can add this to the heading to get data relative to true north.
			std::uint8_t sequenceID = 0; ///< The sequence identifier field is used to tie related PGNs together. Somewhat arbitrary.
			HeadingSensorReference sensorReference = HeadingSensorReference::NotApplicableOrNull; ///< Indicates what the heading is relative to, ie true or magnetic north
		};

		class RateOfTurn
		{
		public:
			explicit RateOfTurn(std::shared_ptr<ControlFunction> source);

			std::shared_ptr<ControlFunction> get_control_function() const;

			std::uint32_t get_timestamp() const;

			bool set_timestamp(std::uint32_t timestamp);

			std::int32_t get_rate_of_turn() const;

			bool set_rate_of_turn(std::int32_t turnRate);

			std::uint8_t get_sequence_id() const;

			bool set_sequence_id(std::uint8_t sequenceNumber);

		private:
			static constexpr std::uint32_t CYCLIC_MESSAGE_RATE_MS = 100;

			std::shared_ptr<ControlFunction> senderControlFunction; ///< The sender of the message data
			std::uint32_t messageTimestamp_ms = 0; ///< A timestamp in milliseconds when this message was last sent or received
			std::int32_t rateOfTurn = 0; ///< The rate of turn in 1/32 * 10e-6 rad/s. Positive values indicate turning right (starboard) relative to the vehicle's reference point.
			std::uint8_t sequenceID = 0; ///< The sequence identifier field is used to tie related PGNs together. Somewhat arbitrary.
		};

		class PositionRapidUpdate
		{
		public:
			explicit PositionRapidUpdate(std::shared_ptr<ControlFunction> source);

			static constexpr std::int32_t NOT_AVAILABLE = 0x7FFFFFFF;

			std::shared_ptr<ControlFunction> get_control_function() const;

			std::uint32_t get_timestamp() const;

			bool set_timestamp(std::uint32_t timestamp);

			/// @attention This is MUCH less accurate than the position in PGN 1F805 (129029). Use that instead if present.
			std::int32_t get_raw_latitude() const;

			/// @attention This is MUCH less accurate than the position in PGN 1F805 (129029). Use that instead if present.
			double get_latitude() const;

			bool set_latitude(std::int32_t latitudeToSet);

			/// @attention This is MUCH less accurate than the position in PGN 1F805 (129029). Use that instead if present.
			std::int32_t get_raw_longitude() const;

			/// @attention This is MUCH less accurate than the position in PGN 1F805 (129029). Use that instead if present.
			double get_longitude() const;

			bool set_longitude(std::int32_t longitudeToSet);

		private:
			std::shared_ptr<ControlFunction> senderControlFunction; ///< The sender of the message data
			std::int32_t latitude = NOT_AVAILABLE; ///< The latitude in 1*10E-7 degrees. Negative values indicate south latitudes.
			std::int32_t longitude = NOT_AVAILABLE; ///< The longitude in 1*10E-7 degrees. Negative values indicate west longitudes.
			std::uint32_t messageTimestamp_ms = 0; ///< A timestamp in milliseconds when this message was last sent or received
		};

		class CourseOverGroundSpeedOverGroundRapidUpdate
		{
		public:
			enum class CourseOverGroudReference : std::uint8_t
			{
				True = 0,
				Magnetic = 1,
				Error = 2,
				NotApplicableOrNull = 3
			};

			explicit CourseOverGroundSpeedOverGroundRapidUpdate(std::shared_ptr<ControlFunction> source);

			std::shared_ptr<ControlFunction> get_control_function() const;

			std::uint32_t get_timestamp() const;

			bool set_timestamp(std::uint32_t timestamp);

			std::uint16_t get_course_over_ground() const;

			bool set_course_over_ground(std::uint16_t course);

			std::uint16_t get_speed_over_ground() const;

			bool set_speed_over_ground(std::uint16_t speed);

			std::uint8_t get_sequence_id() const;

			bool set_sequence_id(std::uint8_t sequenceNumber);

			CourseOverGroudReference get_course_over_ground_reference() const;

			bool set_course_over_ground_reference(CourseOverGroudReference reference);

		private:
			std::shared_ptr<ControlFunction> senderControlFunction; ///< The sender of the message data
			std::uint32_t messageTimestamp_ms = 0; ///< A timestamp in milliseconds when this message was last sent or received
			std::uint16_t courseOverGround = 0; ///< This field contains the direction of the path over ground actually followed by the vessel in 0.0001 radians between 0 and 2pi rad.
			std::uint16_t speedOverGround = 0; ///< This field contains the speed of the vessel in 0.01 m/s
			std::uint8_t sequenceID = 0; ///< The sequence identifier field is used to tie related PGNs together. Somewhat arbitrary.
			CourseOverGroudReference cogReference = CourseOverGroudReference::NotApplicableOrNull; ///< Used to indicate the reference for the course over ground, ie true or magnetic north
		};

		/// @brief This message is a way for a GNSS receiver to provide a current position without using fast packet based on
		/// The content of the last position data combined from the GNSS Position Data message and any prior position delta messages.
		class PositionDeltaHighPrecisionRapidUpdate
		{
		public:
			explicit PositionDeltaHighPrecisionRapidUpdate(std::shared_ptr<ControlFunction> source);

			std::shared_ptr<ControlFunction> get_control_function() const;

			std::uint32_t get_timestamp() const;

			bool set_timestamp(std::uint32_t timestamp);

			std::int32_t get_raw_latitude_delta() const;

			double get_latitude_delta() const;

			bool set_latitude_delta(std::int32_t delta);

			std::int32_t get_raw_longitude_delta() const;

			double get_longitude_delta() const;

			bool set_longitude_delta(std::int32_t delta);

			std::uint8_t get_sequence_id() const;

			bool set_sequence_id(std::uint8_t sequenceNumber);

			std::uint8_t get_raw_time_delta() const;

			double get_time_delta() const;

			bool set_time_delta(std::uint8_t delta);

		private:
			std::shared_ptr<ControlFunction> senderControlFunction; ///< The sender of the message data
			std::uint32_t messageTimestamp_ms = 0; ///< A timestamp in milliseconds when this message was last sent or received
			std::int32_t latitudeDelta = 0; ///< The latitude delta in 1x10E-16 degrees
			std::int32_t longitudeDelta = 0; ///< The longitude delta in 1x10E-16 degrees
			std::uint8_t sequenceID = 0; ///< The sequence identifier field is used to tie related PGNs together. In this case, ties back to GNSS Position Data sequence ID most likely.
			std::uint8_t timeDelta = 0; ///< The time delta in 5x10e-3 seconds
		};

		class GNSSPositionData
		{
		public:
			/// @brief Enumerates the different GNSS systems that can be reported in this message
			enum class TypeOfSystem
			{
				GPS = 0x00, ///< A GNSS system operated by the United States military
				GLONASS = 0x01, ///< A Russian state operated alternative to GPS
				GPSPlusGLONASS = 0x02, ///< A system using both GPS and GLONASS
				GPSPlusSBAS = 0x03, ///< Satellite Based Augmentation System (WAAS) enhanced GPS (Run by the US Federal Aviation Administration)
				GPSPlusSBASPlusGLONASS = 0x04, ///< A system using SBAS augmented GPS as well as GLONASS
				Chayka = 0x05, ///< A Russian Hyperbolic Radio Navigation System similar to Loran-C
				Integrated = 0x06, ///< Using internally integrated solution (maybe digital dead reckoning)
				Surveyed = 0x07,
				Galileo = 0x08, ///< A GNSS system operated by the European Space Agency
				Null = 0x0F
			};

			enum class GNSSMEthod
			{
				NoGNSS = 0x00, ///< Either there is not enough data to compute a navigation solution, or the computed solution is outside of the acceptable error criteria
				GNSSFix = 0x01, ///< Position solution has been achieved
				DGNSSFix = 0x02, ///< Differential solution achieved based on deviation from a well known reference point
				PreciseGNSS = 0x03, ///< Solution achieved using Precise Point Positioning (PPP)
				RTKFixedInteger = 0x04, ///< Solution achieved using radio corrections (from an RTK base station)
				RTKFloat = 0x05, ///< Solution achieved using radio corrections (from an RTK base station) but using floating point instead of fixed integers
				EstimatedMode = 0x06, ///< Dead reckoning
				ManualInput = 0x07,
				SimulateMode = 0x08,
				Null = 0x0F
			};

			/// @brief Enumerates the integrity checking modes that can be reported in this message. You will most often see "NoIntegrityChecking" in reality.
			enum class Integrity
			{
				NoIntegrityChecking = 0x00,
				Safe = 0x01,
				Caution = 0x02,
				Unsafe = 0x03
			};

			explicit GNSSPositionData(std::shared_ptr<ControlFunction> source);

			std::shared_ptr<ControlFunction> get_control_function() const;

			std::int64_t get_raw_latitude() const;

			double get_latitude() const;

			std::int64_t get_raw_longitude() const;

			double get_longitude() const;

			std::int32_t get_geoidal_separation() const;

			bool set_geoidal_separation(std::int32_t separation);

			std::uint32_t get_timestamp() const;

			bool set_timestamp(std::uint32_t timestamp);

			std::uint8_t get_sequence_id() const;

			bool set_sequence_id(std::uint8_t sequenceNumber);

			TypeOfSystem get_type_of_system() const;

			bool set_type_of_system(TypeOfSystem type);

			GNSSMEthod get_gnss_method() const;

			bool set_gnss_method(GNSSMEthod gnssFixMethod);

			Integrity get_integrity() const;

			bool set_integrity(Integrity integrity);

			std::uint8_t get_number_of_space_vehicles() const;

			bool set_number_of_space_vehicles(std::uint8_t numberOfSVs);

			std::int16_t get_horizontal_dilution_of_precision() const;

			bool set_horizontal_dilution_of_precision(std::int16_t hdop);

			std::int16_t get_positional_dilution_of_precision() const;

			bool set_positional_dilution_of_precision(std::int16_t pdop);

			std::uint8_t get_number_of_reference_stations() const;

			bool set_number_of_reference_stations(std::uint8_t stations);

		private:
			class ReferenceStationData
			{
			public:
				std::uint16_t stationID = 0; ///< The station ID of this reference. Can sometimes be used to infer your correction source.
				TypeOfSystem stationType = TypeOfSystem::Null; ///< The type of reference station
				std::uint16_t ageOfDGNSSCorrections = 0xFFFF; ///< Stores the age of the corrections from this reference
			};

			std::shared_ptr<ControlFunction> senderControlFunction; ///< The sender of the message data
			std::vector<ReferenceStationData> referenceStations; ///< Stores data about the reference stations used to generate this position solution.
			std::int64_t altitude = 0; ///< The current altitude in 1x10E-6 meters. Range is +/- 9.223 x 10E+12 meters
			std::int64_t latitude = 0; ///< The current latitude in 1x10E-16 degrees. Range is -90 to 90 degrees. Negative values are south latitudes.
			std::int64_t longitude = 0; ///< The current longitude in 1x10E-16 degrees. Range is -90 to 90 degrees. Negative values are west longitudes.
			std::uint32_t messageTimestamp_ms = 0; ///< A timestamp in milliseconds when this message was last sent or received
			std::uint32_t positionTime = 0; ///< The number of seconds since midnight on the current day. Allows for up to 2 leap seconds per day. Max value is 86401 seconds.
			std::int32_t geoidalSeparation = 0; ///< The difference between the earth ellipsoid and mean-sea-level (geoid) defined by the reference datum used in the position solution.
			std::uint16_t positionDate = 0; ///< Number of days relative to UTC since Jan 1 1970 (so 0 is equal to Jan 1, 1970). Max value is 65532 days.
			std::int16_t horizontalDilutionOfPrecision = 0; ///< Indicates the contribution of satellite configuration geometry to positioning error. Lower is better.
			std::int16_t positionalDilutionOfPrecision = 0; ///< Indicates the contribution of satellite configuration geometry to positioning error. Lower is better.
			std::uint8_t numberOfSpaceVehicles = 0; ///< Number of GPS satellites in view.
			std::uint8_t sequenceID = 0; ///< The sequence identifier field is used to tie related PGNs together. Somewhat arbitrary.
			TypeOfSystem systemType = TypeOfSystem::Null; ///< The type of GNSS system used when generating this message
			GNSSMEthod method = GNSSMEthod::NoGNSS; ///< Stores the method used to provide the GNSS fix
			Integrity integrityChecking = Integrity::NoIntegrityChecking; ///< Stores the integrity of the values in the message
		};

		/// @brief A NMEA2000 message that describes datum (reference frame) information.
		/// A common one might be the WGS84 datum or the NSRS, for example.
		class Datum
		{
		public:
			explicit Datum(std::shared_ptr<ControlFunction> source);

			std::shared_ptr<ControlFunction> get_control_function() const;

			std::string get_local_datum() const;

			bool set_local_datum(const std::string &datum);

			std::string get_reference_datum() const;

			bool set_reference_datum(const std::string &datum);

			std::int32_t get_raw_delta_latitude() const;

			double get_delta_latitude() const;

			bool set_delta_latitude(std::int32_t delta);

			std::int32_t get_raw_delta_longitude() const;

			double get_delta_longitude() const;

			bool set_delta_longitude(std::int32_t delta);

			std::int32_t get_raw_delta_altitude() const;

			double get_delta_altitude() const;

			bool set_delta_altitude(std::int32_t delta);

			std::uint32_t get_timestamp() const;

			bool set_timestamp(std::uint32_t timestamp);

		private:
			std::shared_ptr<ControlFunction> senderControlFunction; ///< The sender of the message data
			std::string localDatum = 0; ///< A 4 character ascii datum code. The first three chars are the datum ID.The fourth char is the local datum subdivision code or a null character if it is unknown or unused.
			std::string referenceDatum = 0; ///< A 4 character ascii datum code that identifies the reference datum.
			std::int32_t deltaLatitude = 0; ///< Position in the local datum is offset from the position in the reference datum as indicated by this latitude delta. In units of 1x10E-7 degrees.
			std::int32_t deltaLongitude = 0; ///< Position in the local datum is offset from the position in the reference datum as indicated by this longitude delta. In units of 1x10E-7 degrees.
			std::int32_t deltaAltitude = 0; ///< The altitude delta in units of 0.02 meters. Positive values indicate Up.
			std::uint32_t messageTimestamp_ms = 0; ///< A timestamp in milliseconds when this message was last sent or received
		};
	} // namespace NMEA2000Messages
} // namespace isobus
#endif // NMEA2000_MESSAGE_DEFINITIONS_HPP
