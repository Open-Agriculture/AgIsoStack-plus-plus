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
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#ifndef NMEA2000_MESSAGE_DEFINITIONS_HPP
#define NMEA2000_MESSAGE_DEFINITIONS_HPP

#include "isobus/isobus/can_internal_control_function.hpp"

#include <string>

namespace isobus
{
	/// @brief A namespace for generic NMEA2000 message definitions
	namespace NMEA2000Messages
	{
		constexpr std::uint8_t MAX_SEQUENCE_ID = 252; ///< The max non-special allowable value of a NMEA2K sequence ID

		/// @brief Represents the data sent in the NMEA2K PGN 127250 (0x1F112)
		class VesselHeading
		{
		public:
			/// @brief The reference which the vessel heading is relative to
			enum class HeadingSensorReference : std::uint8_t
			{
				True = 0, ///< True North
				Magnetic = 1, ///< Magnetic North
				Error = 2,
				NotApplicableOrNull = 3
			};

			/// @brief Constructor for a VesselHeading message data object
			/// @param[in] source The control function sending this message
			explicit VesselHeading(std::shared_ptr<ControlFunction> source);

			/// @brief Returns the control function sending this instance of this message
			/// @returns The control function sending this instance of this message
			std::shared_ptr<ControlFunction> get_control_function() const;

			/// @brief Returns a timestamp in milliseconds corresponding to when the message was last sent or received
			/// @returns A timestamp in milliseconds corresponding to when the message was last sent or received
			std::uint32_t get_timestamp() const;

			/// @brief Sets the time in milliseconds when the message was last sent or received
			/// @param[in] timestamp The timestamp (in milliseconds) to set for when this message was sent or received
			/// @returns true if the value that was set was different from the stored value
			bool set_timestamp(std::uint32_t timestamp);

			/// @brief Returns the vessel heading in units of 0.0001 radians, which are the message's base units
			/// @returns Vessel heading in units of 0.0001 radians
			std::uint16_t get_raw_heading() const;

			/// @brief Returns the vessel heading in radians
			/// @returns Vessel heading in radians
			float get_heading() const;

			/// @brief Sets the vessel heading
			/// @param[in] heading The heading to set in 0.0001 radians
			/// @returns True if the value that was set was different from the stored value
			bool set_heading(std::uint16_t heading);

			/// @brief Returns the magnetic deviation in 0.0001 radians
			/// @returns The magnetic deviation in 0.0001 radians
			std::int16_t get_raw_magnetic_deviation() const;

			/// @brief Returns the magnetic deviation in radians
			/// @returns The magnetic deviation in radians
			float get_magnetic_deviation() const;

			/// @brief Sets the magnetic deviation in 0.0001 radians
			/// @param[in] deviation The magnetic deviation to set, in units of 0.0001 radians
			/// @returns true if the value that was set was different from the stored value
			bool set_magnetic_deviation(std::int16_t deviation);

			/// @brief Returns the magnetic variation in units of 0.0001 radians
			/// @returns The magnetic variation in units of 0.0001 radians
			std::int16_t get_raw_magnetic_variation() const;

			/// @brief Returns the magnetic variation in units of radians
			/// @returns The magnetic variation in units of radians
			float get_magnetic_variation() const;

			/// @brief Sets the magnetic variation, in units of 0.0001 radians
			/// @param[in] variation The magnetic variation to set, in units of 0.0001 radians
			/// @returns true if the value that was set was different from the stored value
			bool set_magnetic_variation(std::int16_t variation);

			/// @brief Returns the sequence ID. This is used to associate data within other PGNs with this message.
			/// @returns The sequence ID for this message
			std::uint8_t get_sequence_id() const;

			/// @brief Sets the sequence ID for this message.
			/// @param[in] sequenceNumber The sequence number to set. Max value is 252.
			/// @returns true if the value that was set was different from the stored value
			bool set_sequence_id(std::uint8_t sequenceNumber);

			/// @brief Returns the reference to which the reported heading is relative to
			/// @returns The reference to which the reported heading is relative to
			HeadingSensorReference get_sensor_reference() const;

			/// @brief Sets the reference to which the reported heading is relative to
			/// @param[in] reference The reference to set
			/// @returns true if the value that was set was different from the stored value
			bool set_sensor_reference(HeadingSensorReference reference);

			/// @brief Takes the current state of the object and serializes it into a buffer to be sent.
			/// @param[in] buffer A vector to populate with the message data
			void serialize(std::vector<std::uint8_t> &buffer) const;

			/// @brief Deserializes a CAN message to populate this object's contents. Updates the timestamp when called.
			/// @param[in] receivedMessage The CAN message to parse when deserializing
			/// @returns True if the message was successfully deserialized and the data content was different than the stored content.
			bool deserialize(const CANMessage &receivedMessage);

			/// @brief Returns the timeout (the sending interval) for this message in milliseconds
			/// @returns This message's timeout (the sending interval) in milliseconds
			static std::uint32_t get_timeout();

		private:
			static constexpr std::uint32_t CYCLIC_MESSAGE_RATE_MS = 100; ///< The interval in milliseconds on which this message should be sent/received

			std::shared_ptr<ControlFunction> senderControlFunction; ///< The sender of the message data
			std::uint32_t messageTimestamp_ms = 0; ///< A timestamp in milliseconds when this message was last sent or received
			std::uint16_t headingReading = 0; ///< The raw heading in 0.0001 radians, relative to the indicated HeadingSensorReference.
			std::int16_t magneticDeviation = 0; ///< The magnetic deviation if not included in the reading in 0.0001 radians. Positive values are easterly.
			std::int16_t magneticVariation = 0; ///< The magnetic variation if applicable in 0.0001 radians. Positive values are easterly. If the reference is magnetic, you can add this to the heading to get data relative to true north.
			std::uint8_t sequenceID = 0; ///< The sequence identifier field is used to tie related PGNs together. Somewhat arbitrary.
			HeadingSensorReference sensorReference = HeadingSensorReference::NotApplicableOrNull; ///< Indicates what the heading is relative to, ie true or magnetic north
		};

		/// @brief Represents the data sent in the NMEA2K PGN 127251 (0x1F113)
		class RateOfTurn
		{
		public:
			/// @brief Constructor for a RateOfTurn message data object
			/// @param[in] source The control function sending the message
			explicit RateOfTurn(std::shared_ptr<ControlFunction> source);

			/// @brief Returns the control function sending this instance of this message
			/// @returns The control function sending this instance of this message
			std::shared_ptr<ControlFunction> get_control_function() const;

			/// @brief Returns a timestamp in milliseconds corresponding to when the message was last sent or received
			/// @returns A timestamp in milliseconds corresponding to when the message was last sent or received
			std::uint32_t get_timestamp() const;

			/// @brief Sets the time in milliseconds when the message was last sent or received
			/// @param[in] timestamp The timestamp (in milliseconds) to set for when this message was sent or received
			/// @returns true if the value that was set was different from the stored value
			bool set_timestamp(std::uint32_t timestamp);

			/// @brief Returns the rate of turn of the vessel/vehicle in units of 1/32 x 10E-6 rad/s
			/// @returns The rate of turn of the vessel/vehicle in units of 1/32 x 10E-6 rad/s
			std::int32_t get_raw_rate_of_turn() const;

			/// @brief Returns the rate of turn of the vessel/vehicle in rad/s
			/// @returns The rate of turn of the vessel/vehicle in rad/s
			double get_rate_of_turn() const;

			/// @brief Sets the rate of turn in units of 1/32 x 10E-6 rad/s
			/// @param[in] turnRate The rate of turn to set, in units of 1/32 x 10E-6 rad/s
			/// @returns true if the value that was set was different from the stored value
			bool set_rate_of_turn(std::int32_t turnRate);

			/// @brief Returns the sequence ID. This is used to associate data within other PGNs with this message.
			/// @returns The sequence ID for this message
			std::uint8_t get_sequence_id() const;

			/// @brief Sets the sequence ID for this message.
			/// @param[in] sequenceNumber The sequence number to set. Max value is 252.
			/// @returns true if the value that was set was different from the stored value
			bool set_sequence_id(std::uint8_t sequenceNumber);

			/// @brief Serializes the current state of this object into a buffer to be sent on the CAN bus
			/// @param[in] buffer A buffer to serialize the message data into
			void serialize(std::vector<std::uint8_t> &buffer) const;

			/// @brief Deserializes a CAN message to populate this object's contents. Updates the timestamp when called.
			/// @param[in] receivedMessage The CAN message to parse when deserializing
			/// @returns True if the message was successfully deserialized and the data content was different than the stored content.
			bool deserialize(const CANMessage &receivedMessage);

			/// @brief Returns the timeout (the sending interval) for this message in milliseconds
			/// @returns This message's timeout (the sending interval) in milliseconds
			static std::uint32_t get_timeout();

		private:
			static constexpr std::uint32_t CYCLIC_MESSAGE_RATE_MS = 100; ///< The interval in milliseconds on which this message should be sent/received

			std::shared_ptr<ControlFunction> senderControlFunction; ///< The sender of the message data
			std::uint32_t messageTimestamp_ms = 0; ///< A timestamp in milliseconds when this message was last sent or received
			std::int32_t rateOfTurn = 0; ///< The rate of turn in 1/32 * 10e-6 rad/s. Positive values indicate turning right (starboard) relative to the vehicle's reference point.
			std::uint8_t sequenceID = 0; ///< The sequence identifier field is used to tie related PGNs together. Somewhat arbitrary.
		};

		/// @brief Represents the data sent in the NMEA2K PGN 129025 (0x1F801)
		class PositionRapidUpdate
		{
		public:
			/// @brief Constructor for a PositionRapidUpdate message data object
			/// @param[in] source The control function sending this message
			explicit PositionRapidUpdate(std::shared_ptr<ControlFunction> source);

			static constexpr std::int32_t NOT_AVAILABLE = 0x7FFFFFFF; ///< A generic value that may be reported if the position solution is invalid

			/// @brief Returns the control function sending this instance of this message
			/// @returns The control function sending this instance of this message
			std::shared_ptr<ControlFunction> get_control_function() const;

			/// @brief Returns a timestamp in milliseconds corresponding to when the message was last sent or received
			/// @returns A timestamp in milliseconds corresponding to when the message was last sent or received
			std::uint32_t get_timestamp() const;

			/// @brief Sets the time in milliseconds when the message was last sent or received
			/// @param[in] timestamp The timestamp (in milliseconds) to set for when this message was sent or received
			/// @returns true if the value that was set was different from the stored value
			bool set_timestamp(std::uint32_t timestamp);

			/// @attention This is MUCH less accurate than the position in PGN 1F805 (129029). Use that instead if present.
			/// @returns The current vessel/vehicle latitude in 1*10E-7 degrees
			std::int32_t get_raw_latitude() const;

			/// @attention This is MUCH less accurate than the position in PGN 1F805 (129029). Use that instead if present.
			/// @returns The current vessel/vehicle longitude in 1*10E-7 degrees
			double get_latitude() const;

			/// @brief Sets the current latitude in units of 1*10E-7 degrees
			/// @param[in] latitudeToSet The latitude to set in units of 1*10E-7 degrees
			/// @returns true if the value that was set was different from the stored value
			bool set_latitude(std::int32_t latitudeToSet);

			/// @attention This is MUCH less accurate than the position in PGN 1F805 (129029). Use that instead if present.
			/// @returns The current vessel/vehicle longitude in 1*10E-7 degrees
			std::int32_t get_raw_longitude() const;

			/// @attention This is MUCH less accurate than the position in PGN 1F805 (129029). Use that instead if present.
			/// @returns The current vessel/vehicle longitude in 1*10E-7 degrees
			double get_longitude() const;

			/// @brief Sets the current longitude in units of 1*10E-7 degrees
			/// @param[in] longitudeToSet The latitude to set in units of 1*10E-7 degrees
			/// @returns true if the value that was set was different from the stored value
			bool set_longitude(std::int32_t longitudeToSet);

			/// @brief Serializes the current state of this object into a buffer to be sent on the CAN bus
			/// @param[in] buffer A buffer to serialize the message data into
			void serialize(std::vector<std::uint8_t> &buffer) const;

			/// @brief Deserializes a CAN message to populate this object's contents. Updates the timestamp when called.
			/// @param[in] receivedMessage The CAN message to parse when deserializing
			/// @returns True if the message was successfully deserialized and the data content was different than the stored content.
			bool deserialize(const CANMessage &receivedMessage);

			/// @brief Returns the timeout (the sending interval) for this message in milliseconds
			/// @returns This message's timeout (the sending interval) in milliseconds
			static std::uint32_t get_timeout();

		private:
			static constexpr std::uint32_t CYCLIC_MESSAGE_RATE_MS = 100; ///< The transmit interval for this message as specified in NMEA2000

			std::shared_ptr<ControlFunction> senderControlFunction; ///< The sender of the message data
			std::int32_t latitude = NOT_AVAILABLE; ///< The latitude in 1*10E-7 degrees. Negative values indicate south latitudes.
			std::int32_t longitude = NOT_AVAILABLE; ///< The longitude in 1*10E-7 degrees. Negative values indicate west longitudes.
			std::uint32_t messageTimestamp_ms = 0; ///< A timestamp in milliseconds when this message was last sent or received
		};

		/// @brief Represents the data sent in the NMEA2K PGN 129026 (0x1F802)
		class CourseOverGroundSpeedOverGroundRapidUpdate
		{
		public:
			/// @brief Enumerates the references to which the course may be relative to
			enum class CourseOverGroundReference : std::uint8_t
			{
				True = 0, ///< True north
				Magnetic = 1, ///< Magnetic North
				Error = 2,
				NotApplicableOrNull = 3
			};

			/// @brief Constructor for a CourseOverGroundSpeedOverGroundRapidUpdate message data object
			/// @param[in] source The control function sending/receiving this message
			explicit CourseOverGroundSpeedOverGroundRapidUpdate(std::shared_ptr<ControlFunction> source);

			/// @brief Returns the control function sending this instance of this message
			/// @returns The control function sending this instance of this message
			std::shared_ptr<ControlFunction> get_control_function() const;

			/// @brief Returns a timestamp in milliseconds corresponding to when the message was last sent or received
			/// @returns A timestamp in milliseconds corresponding to when the message was last sent or received
			std::uint32_t get_timestamp() const;

			/// @brief Sets the time in milliseconds when the message was last sent or received
			/// @param[in] timestamp The timestamp (in milliseconds) to set for when this message was sent or received
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_timestamp(std::uint32_t timestamp);

			/// @brief Returns the course over ground in its base units of 0.0001 radians (between 0 and 2 pi radians)
			/// @returns The course over ground in its base units of 0.0001 radians
			std::uint16_t get_raw_course_over_ground() const;

			/// @brief Returns the course over ground in units of radians
			/// @returns Course over ground in units of radians
			float get_course_over_ground() const;

			/// @brief Sets the course over ground in units of 0.0001 radians
			/// @param[in] course The course to set, in 0.0001 radians
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_course_over_ground(std::uint16_t course);

			/// @brief Returns the speed over ground in units of 0.01 meters per second
			/// @returns The speed over ground in units of 0.01 meters per second
			std::uint16_t get_raw_speed_over_ground() const;

			/// @brief Returns the speed over ground in units of meters per second
			/// @returns The speed over ground in units of meters per second
			float get_speed_over_ground() const;

			/// @brief Sets the speed over ground in units of 0.01 meters per second
			/// @param[in] speed The speed to set, in 0.01 m/s
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_speed_over_ground(std::uint16_t speed);

			/// @brief Returns the sequence ID. This is used to associate data within other PGNs with this message.
			/// @returns The sequence ID for this message
			std::uint8_t get_sequence_id() const;

			/// @brief Sets the sequence ID for this message.
			/// @param[in] sequenceNumber The sequence number to set. Max value is 252.
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_sequence_id(std::uint8_t sequenceNumber);

			/// @brief Returns the reference to which the course over ground is relative
			/// @returns The reference to which the course is relative
			CourseOverGroundReference get_course_over_ground_reference() const;

			/// @brief Sets the reference to which the course over ground is relative
			/// @param[in] reference The reference to set (as enumerated in CourseOverGroundReference)
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_course_over_ground_reference(CourseOverGroundReference reference);

			/// @brief Serializes the current state of this object into a buffer to be sent on the CAN bus
			/// @param[in] buffer A buffer to serialize the message data into
			void serialize(std::vector<std::uint8_t> &buffer) const;

			/// @brief Deserializes a CAN message to populate this object's contents. Updates the timestamp when called.
			/// @param[in] receivedMessage The CAN message to parse when deserializing
			/// @returns True if the message was successfully deserialized and the data content was different than the stored content.
			bool deserialize(const CANMessage &receivedMessage);

			/// @brief Returns the timeout (the sending interval) for this message in milliseconds
			/// @returns This message's timeout (the sending interval) in milliseconds
			static std::uint32_t get_timeout();

		private:
			static constexpr std::uint32_t CYCLIC_MESSAGE_RATE_MS = 250; ///< The transmit interval for this message as specified in NMEA2000

			std::shared_ptr<ControlFunction> senderControlFunction; ///< The sender of the message data
			std::uint32_t messageTimestamp_ms = 0; ///< A timestamp in milliseconds when this message was last sent or received
			std::uint16_t courseOverGround = 0; ///< This field contains the direction of the path over ground actually followed by the vessel in 0.0001 radians between 0 and 2pi rad.
			std::uint16_t speedOverGround = 0; ///< This field contains the speed of the vessel in 0.01 m/s
			std::uint8_t sequenceID = 0; ///< The sequence identifier field is used to tie related PGNs together. Somewhat arbitrary.
			CourseOverGroundReference cogReference = CourseOverGroundReference::NotApplicableOrNull; ///< Used to indicate the reference for the course over ground, ie true or magnetic north
		};

		/// @brief This message is a way for a GNSS receiver to provide a current position without using fast packet based on
		/// The content of the last position data combined from the GNSS Position Data message and any prior position delta messages.
		/// This PGN provides latitude and longitude referenced to WGS84.
		class PositionDeltaHighPrecisionRapidUpdate
		{
		public:
			/// @brief Constructor for a PositionDeltaHighPrecisionRapidUpdate message data object
			/// @param[in] source The control function sending this message
			explicit PositionDeltaHighPrecisionRapidUpdate(std::shared_ptr<ControlFunction> source);

			/// @brief Returns the control function sending this instance of this message
			/// @returns The control function sending this instance of this message
			std::shared_ptr<ControlFunction> get_control_function() const;

			/// @brief Returns a timestamp in milliseconds corresponding to when the message was last sent or received
			/// @returns A timestamp in milliseconds corresponding to when the message was last sent or received
			std::uint32_t get_timestamp() const;

			/// @brief Sets the time in milliseconds when the message was last sent or received
			/// @param[in] timestamp The timestamp (in milliseconds) to set for when this message was sent or received
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_timestamp(std::uint32_t timestamp);

			/// @brief Returns the latitude delta relative to our last position in 1x10E-16 degrees
			/// @returns Latitude delta relative to our last position in 1x10E-16 degrees
			std::int32_t get_raw_latitude_delta() const;

			/// @brief Returns the latitude delta relative to our last position in degrees
			/// @returns Latitude delta relative to our last position in degrees
			double get_latitude_delta() const;

			/// @brief Sets the current latitude delta in units of 1x10E-16 degrees
			/// @param[in] delta Latitude delta to set in units of 1x10E-16 degrees
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_latitude_delta(std::int32_t delta);

			/// @brief Returns the longitude delta relative to our last position in 1x10E-16 degrees
			/// @returns Longitude delta relative to our last position in 1x10E-16 degrees
			std::int32_t get_raw_longitude_delta() const;

			/// @brief Returns the longitude delta relative to our last position in degrees
			/// @returns Longitude delta relative to our last position in degrees
			double get_longitude_delta() const;

			/// @brief Sets the current longitude delta relative to our last position in 1x10E-16 degrees
			/// @param[in] delta Longitude delta to set in units of 1x10E-16 degrees
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_longitude_delta(std::int32_t delta);

			/// @brief Returns the sequence ID. This is used to associate data within other PGNs with this message.
			/// @returns The sequence ID for this message
			std::uint8_t get_sequence_id() const;

			/// @brief Sets the sequence ID for this message.
			/// @param[in] sequenceNumber The sequence number to set. Max value is 252.
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_sequence_id(std::uint8_t sequenceNumber);

			/// @brief Returns the raw time delta since the last reported time in 5x10e-3 seconds
			/// @returns Time delta in units of 5x10e-3 seconds
			std::uint8_t get_raw_time_delta() const;

			/// @brief Returns the raw time delta since the last reported time in seconds
			/// @returns Time delta in units of seconds
			double get_time_delta() const;

			/// @brief Sets the time delta, in units of 5x10e-3 seconds
			/// @param[in] delta The time delta to set in units of 5x10e-3 seconds
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_time_delta(std::uint8_t delta);

			/// @brief Serializes the current state of this object into a buffer to be sent on the CAN bus
			/// @param[in] buffer A buffer to serialize the message data into
			void serialize(std::vector<std::uint8_t> &buffer) const;

			/// @brief Deserializes a CAN message to populate this object's contents. Updates the timestamp when called.
			/// @param[in] receivedMessage The CAN message to parse when deserializing
			/// @returns True if the message was successfully deserialized and the data content was different than the stored content.
			bool deserialize(const CANMessage &receivedMessage);

			/// @brief Returns the timeout (the sending interval) for this message in milliseconds
			/// @returns This message's timeout (the sending interval) in milliseconds
			static std::uint32_t get_timeout();

		private:
			static constexpr std::uint32_t CYCLIC_MESSAGE_RATE_MS = 250; ///< The transmit interval for this message as specified in NMEA2000

			std::shared_ptr<ControlFunction> senderControlFunction; ///< The sender of the message data
			std::uint32_t messageTimestamp_ms = 0; ///< A timestamp in milliseconds when this message was last sent or received
			std::int32_t latitudeDelta = 0; ///< The latitude delta in 1x10E-16 degrees
			std::int32_t longitudeDelta = 0; ///< The longitude delta in 1x10E-16 degrees
			std::uint8_t sequenceID = 0; ///< The sequence identifier field is used to tie related PGNs together. In this case, ties back to GNSS Position Data sequence ID most likely.
			std::uint8_t timeDelta = 0; ///< The time delta in 5x10e-3 seconds
		};

		/// @brief Represents the data sent in the NMEA2K PGN 129029 (0x1F805)
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

			/// @brief Enumerates the GNSS methods that can be reported in this message
			enum class GNSSMethod
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

			/// @brief Constructor for a GNSSPositionData message data object
			/// @param[in] source The control function sending this message
			explicit GNSSPositionData(std::shared_ptr<ControlFunction> source);

			/// @brief Returns the control function sending this instance of this message
			/// @returns The control function sending this instance of this message
			std::shared_ptr<ControlFunction> get_control_function() const;

			/// @brief Returns the altitude portion of the position fix in its base units of 1x10E-6 meters. Range is +/- 9.223 x 10E+12 meters
			/// @returns Altitude portion of the position fix in its base units of 1x10E-6 meters. Range is +/- 9.223 x 10E+12 meters
			std::int64_t get_raw_altitude() const;

			/// @brief Returns the altitude portion of the position fix in scaled units of meters. Range is +/- 9.223 x 10E+12 meters
			/// @returns Altitude portion of the position fix in scaled units of meters. Range is +/- 9.223 x 10E+12 meters
			double get_altitude() const;

			/// @brief Sets the reported altitude in units of 1x10E-6 meters. Range is +/- 9.223 x 10E+12 meters
			/// @param[in] altitudeToSet Altitude to set in units of 1x10E-6 meters. Range is +/- 9.223 x 10E+12 meters
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_altitude(std::int64_t altitudeToSet);

			/// @brief Returns our current position's latitude in its base units of 1x10E-16 degrees
			/// @returns Current position's latitude in units of 1x10E-16 degrees
			std::int64_t get_raw_latitude() const;

			/// @brief Returns our current position's latitude in units of degrees
			/// @returns Current position's latitude in units of degrees
			double get_latitude() const;

			/// @brief Sets the reported latitude in its base units of 1x10E-16 degrees
			/// @param[in] latitudeToSet The latitude to set in 1x10E-16 degrees
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_latitude(std::int64_t latitudeToSet);

			/// @brief Returns our current position's longitude in its base units of 1x10E-16 degrees
			/// @returns Current position's longitude in units of 1x10E-16 degrees
			std::int64_t get_raw_longitude() const;

			/// @brief Returns our current position's longitude in units of degrees
			/// @returns Current position's longitude in units of degrees
			double get_longitude() const;

			/// @brief Sets the reported longitude in its base units of 1x10E-16 degrees
			/// @param[in] longitudeToSet The longitude to set in 1x10E-16 degrees
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_longitude(std::int64_t longitudeToSet);

			/// @brief Returns the geoidal separation in units of 0.01 meters
			/// @details This returns the difference between the earth ellipsoid and mean-sea-level (geoid) defined by the reference datum
			/// @returns The geoidal separation in units of 0.01m
			std::int32_t get_raw_geoidal_separation() const;

			/// @brief Returns the geoidal separation in units of meters
			/// @details This returns the difference between the earth ellipsoid and mean-sea-level (geoid) defined by the reference datum
			/// @returns The geoidal separation in units of meters
			float get_geoidal_separation() const;

			/// @brief Sets the geoidal separation
			/// @details This value is the difference between the earth ellipsoid and mean-sea-level (geoid) defined by the reference datum
			/// @param[in] separation The geoidal separation to set
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_geoidal_separation(std::int32_t separation);

			/// @brief Returns a timestamp in milliseconds corresponding to when the message was last sent or received
			/// @returns A timestamp in milliseconds corresponding to when the message was last sent or received
			std::uint32_t get_timestamp() const;

			/// @brief Sets the time in milliseconds when the message was last sent or received
			/// @param[in] timestamp The timestamp (in milliseconds) to set for when this message was sent or received
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_timestamp(std::uint32_t timestamp);

			/// @brief Returns the sequence ID. This is used to associate data within other PGNs with this message.
			/// @returns The sequence ID for this message
			std::uint8_t get_sequence_id() const;

			/// @brief Sets the sequence ID for this message.
			/// @param[in] sequenceNumber The sequence number to set. Max value is 252.
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_sequence_id(std::uint8_t sequenceNumber);

			/// @brief Returns the reported type of GNSS system that produced this position solution
			/// @returns The type of GNSS system that produced this position solution
			TypeOfSystem get_type_of_system() const;

			/// @brief Sets the reported type of GNSS system that produced this position solution
			/// @param[in] type The type of system to set
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_type_of_system(TypeOfSystem type);

			/// @brief Returns the GNSS method being reported as part of this position solution, such as RTK Float or DGNSS
			/// @returns The GNSS method being reported as part of this position solution, such as RTK Float or DGNSS
			GNSSMethod get_gnss_method() const;

			/// @brief Sets the GNSS method to report as the source of this position solution, such as RTK float or DGNSS
			/// @param[in] gnssFixMethod The GNSS method to report as the source of this position solution
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_gnss_method(GNSSMethod gnssFixMethod);

			/// @brief Sets the integrity being reported for this position solution if applicable
			/// @returns The integrity being reported for this position solution
			Integrity get_integrity() const;

			/// @brief Sets the integrity reported for this position solution
			/// @param[in] integrity The integrity to report
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_integrity(Integrity integrity);

			/// @brief Returns the number of space vehicles used in this position solution
			/// @returns The number of space vehicles used in this position solution
			std::uint8_t get_number_of_space_vehicles() const;

			/// @brief Sets the number of space vehicles in view and used in this position solution
			/// @param[in] numberOfSVs The number of space vehicles to set
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_number_of_space_vehicles(std::uint8_t numberOfSVs);

			/// @brief Returns the HDOP for this solution.
			/// This Indicates the contribution of satellite configuration geometry to positioning error. Lower is better.
			/// @returns The horizontal dilution of precision (HDOP)
			std::int16_t get_raw_horizontal_dilution_of_precision() const;

			/// @brief Returns the HDOP for this solution.
			/// This Indicates the contribution of satellite configuration geometry to positioning error. Lower is better.
			/// @returns The horizontal dilution of precision (HDOP)
			float get_horizontal_dilution_of_precision() const;

			/// @brief Sets the horizontal dilution of precision (HDOP)
			/// @param[in] hdop The positional dilution of precision to set
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_horizontal_dilution_of_precision(std::int16_t hdop);

			/// @brief Returns the PDOP for this solution.
			/// This Indicates the contribution of satellite configuration geometry to positioning error. Lower is better.
			/// @returns The positional dilution of precision (PDOP)
			std::int16_t get_raw_positional_dilution_of_precision() const;

			/// @brief Returns the PDOP for this solution.
			/// This Indicates the contribution of satellite configuration geometry to positioning error. Lower is better.
			/// @returns The positional dilution of precision (PDOP)
			float get_positional_dilution_of_precision() const;

			/// @brief Sets the positional dilution of precision (PDOP)
			/// @param[in] pdop The positional dilution of precision to set
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_positional_dilution_of_precision(std::int16_t pdop);

			/// @brief Returns the number of reference stations used in this position solution (if applicable to GNSS method)
			/// @returns The number of reference stations used in this position solution
			std::uint8_t get_number_of_reference_stations() const;

			/// @brief Sets the number of reference stations used in this position solution
			/// @param[in] stations The number of reference stations to set (if applicable to GNSS method, otherwise should be zero)
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_number_of_reference_stations(std::uint8_t stations);

			/// @brief Returns the specified reference station's ID by index
			/// @param[in] index The index of the reference station to get the ID of
			/// @returns Reference station's ID by index
			std::uint16_t get_reference_station_id(std::size_t index) const;

			/// @brief Returns the specified reference station's DGNSS corrections age by index
			/// @param[in] index The index of the reference station to get the DGNSS corrections age for
			/// @returns Reference station's DGNSS corrections age by index
			std::uint16_t get_raw_reference_station_corrections_age(std::size_t index) const;

			/// @brief Returns the specified reference station's DGNSS corrections age by index
			/// @param[in] index The index of the reference station to get the DGNSS corrections age for
			/// @returns Reference station's DGNSS corrections age by index
			float get_reference_station_corrections_age(std::size_t index) const;

			/// @brief Returns the specified reference station's system type by index
			/// @param[in] index The index of the reference station to get the system type for
			/// @returns The specified reference station's system type by index
			TypeOfSystem get_reference_station_system_type(std::size_t index) const;

			/// @brief Sets the data for the specified reference station by index
			/// @param[in] index The index of the reference station to set
			/// @param[in] ID The station ID to set
			/// @param[in] type The type of reference station
			/// @param[in] ageOfCorrections Age of the DGNSS corrections in units of 0.01 seconds
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_reference_station(std::size_t index, std::uint16_t ID, TypeOfSystem type, std::uint16_t ageOfCorrections);

			/// @brief Returns the date associated with the current position.
			/// @returns Number of days relative to UTC since Jan 1 1970 (0 is equal to Jan 1, 1970). Max value is 65532 days.
			std::uint16_t get_position_date() const;

			/// @brief Sets the date to report relative to UTC since Jan 1 1970. Max normal value is 65532
			/// @param[in] dateToSet Date to report relative to UTC since Jan 1 1970. Max normal value is 65532
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_position_date(std::uint16_t dateToSet);

			/// @brief Returns the number of seconds since midnight
			/// @returns Number of seconds since midnight (0 == midnight), range allows for up to two leap seconds per day
			std::uint32_t get_raw_position_time() const;

			/// @brief Returns the number of seconds since midnight
			/// @returns Number of seconds since midnight (0 == midnight), range allows for up to two leap seconds per day, in units of 0.0001 seconds
			double get_position_time() const;

			/// @brief Sets the number of seconds since midnight
			/// @param[in] timeToSet Seconds since midnight (0 == midnight), range allows for up to two leap seconds per day, in units of 0.0001 seconds
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_position_time(std::uint32_t timeToSet);

			/// @brief Serializes the current state of this object into a buffer to be sent on the CAN bus
			/// @param[in] buffer A buffer to serialize the message data into
			void serialize(std::vector<std::uint8_t> &buffer) const;

			/// @brief Deserializes a CAN message to populate this object's contents. Updates the timestamp when called.
			/// @param[in] receivedMessage The CAN message to parse when deserializing
			/// @returns True if the message was successfully deserialized and the data content was different than the stored content.
			bool deserialize(const CANMessage &receivedMessage);

			/// @brief Returns the timeout (the sending interval) for this message in milliseconds
			/// @returns This message's timeout (the sending interval) in milliseconds
			static std::uint32_t get_timeout();

		private:
			/// @brief Used to group related reference station data together
			class ReferenceStationData
			{
			public:
				/// @brief Default constructor for a ReferenceStationData with default values
				ReferenceStationData() = default;

				/// @brief Constructor for ReferenceStationData that initializes all values to provided values
				/// @param[in] id The station ID to set
				/// @param[in] type The station system type to set
				/// @param[in] age The age of DGNSS corrections to set
				ReferenceStationData(std::uint16_t id, TypeOfSystem type, std::uint16_t age);
				std::uint16_t stationID = 0; ///< The station ID of this reference. Can sometimes be used to infer your correction source.
				TypeOfSystem stationType = TypeOfSystem::Null; ///< The type of reference station
				std::uint16_t ageOfDGNSSCorrections = 0xFFFF; ///< Stores the age of the corrections from this reference
			};

			static constexpr std::uint32_t CYCLIC_MESSAGE_RATE_MS = 1000; ///< The transmit interval for this message as specified in NMEA2000
			static constexpr std::uint8_t MINIMUM_LENGTH_BYTES = 43; ///< The minimum size of this message in bytes

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
			GNSSMethod method = GNSSMethod::NoGNSS; ///< Stores the method used to provide the GNSS fix
			Integrity integrityChecking = Integrity::NoIntegrityChecking; ///< Stores the integrity of the values in the message
		};

		/// @brief A NMEA2000 message that describes datum (reference frame) information. PGN 129044 (0x1F814)
		/// A common one might be the WGS84 datum or the NSRS, for example.
		/// @details This provides local geodetic datum and datum offsets from a reference datum.
		/// This PGN is used to define the datum to which a position location output by the same device in other PGNs is referenced.
		class Datum
		{
		public:
			/// @brief Constructor for a Datum message data object
			/// @param[in] source The control function sending the message
			explicit Datum(std::shared_ptr<ControlFunction> source);

			/// @brief Returns the control function sending this instance of this message
			/// @returns The control function sending this instance of this message
			std::shared_ptr<ControlFunction> get_control_function() const;

			/// @brief Returns the 4 character ascii datum code
			/// @returns The 4 character ascii datum code
			std::string get_local_datum() const;

			/// @brief Sets the local datum's 4 character ascii datum code
			/// @param[in] datum The datum code to set
			/// @returns True if the value that was set differed from the stored value
			bool set_local_datum(const std::string &datum);

			/// @brief Returns the 4 character ascii datum code that identifies the reference datum
			/// @returns The 4 character ascii datum code that identifies the reference datum
			std::string get_reference_datum() const;

			/// @brief Sets the 4 character ascii datum code that identifies the reference datum
			/// @param[in] datum The datum code to set, must be 4 characters
			/// @returns True if the value that was set differed from the stored value
			bool set_reference_datum(const std::string &datum);

			/// @brief Returns latitude offset of position in the local datum from the position in the reference datum. In units of 1x10E-7 degrees
			/// @returns Latitude offset of position in the local datum from the position in the reference datum. In units of 1x10E-7 degrees
			std::int32_t get_raw_delta_latitude() const;

			/// @brief Returns latitude offset of position in the local datum from the position in the reference datum. In units of degrees
			/// @returns Latitude offset of position in the local datum from the position in the reference datum. In units of degrees
			double get_delta_latitude() const;

			/// @brief Sets latitude offset of position in the local datum from the position in the reference datum in units of 1x10E-7 degrees.
			/// @param[in] delta The latitude offset to set in units of 1x10E-7 degrees
			/// @returns True if the value that was set differed from the stored value
			bool set_delta_latitude(std::int32_t delta);

			/// @brief Returns longitude offset of position in the local datum from the position in the reference datum. In units of 1x10E-7 degrees
			/// @returns Longitude offset of position in the local datum from the position in the reference datum. In units of 1x10E-7 degrees
			std::int32_t get_raw_delta_longitude() const;

			/// @brief Returns longitude offset of position in the local datum from the position in the reference datum. In units of degrees
			/// @returns Longitude offset of position in the local datum from the position in the reference datum. In units of degrees
			double get_delta_longitude() const;

			/// @brief Sets longitude offset of position in the local datum from the position in the reference datum in units of 1x10E-7 degrees.
			/// @param[in] delta The longitude offset to set in units of 1x10E-7 degrees
			/// @returns True if the value that was set differed from the stored value
			bool set_delta_longitude(std::int32_t delta);

			/// @brief Returns the altitude offset of position in the local datum relative to the position in the reference datum in units of The altitude delta in units of 0.01 meters.
			/// @returns The altitude offset of position in the local datum relative to the position in the reference datum in units of The altitude delta in units of 0.01 meters.
			std::int32_t get_raw_delta_altitude() const;

			/// @brief Returns the altitude offset of position in the local datum relative to the position in the reference datum in units of The altitude delta in units of meters.
			/// @returns The altitude offset of position in the local datum relative to the position in the reference datum in units of The altitude delta in units of meters.
			float get_delta_altitude() const;

			/// @brief Sets the altitude offset of position in the local datum relative to the position in the reference datum in units of The altitude delta in units of 0.01 meters.
			/// @param[in] delta The altitude delta to set in units of 0.01 meters
			/// @returns True if the value that was set differed from the stored value
			bool set_delta_altitude(std::int32_t delta);

			/// @brief Returns a timestamp in milliseconds corresponding to when the message was last sent or received
			/// @returns A timestamp in milliseconds corresponding to when the message was last sent or received
			std::uint32_t get_timestamp() const;

			/// @brief Sets the time in milliseconds when the message was last sent or received
			/// @param[in] timestamp The timestamp (in milliseconds) to set for when this message was sent or received
			/// @returns True if the value that was set differed from the stored value, otherwise false
			bool set_timestamp(std::uint32_t timestamp);

			/// @brief Serializes the current state of this object into a buffer to be sent on the CAN bus
			/// @param[in] buffer A buffer to serialize the message data into
			void serialize(std::vector<std::uint8_t> &buffer) const;

			/// @brief Deserializes a CAN message to populate this object's contents. Updates the timestamp when called.
			/// @param[in] receivedMessage The CAN message to parse when deserializing
			/// @returns True if the message was successfully deserialized and the data content was different than the stored content.
			bool deserialize(const CANMessage &receivedMessage);

			/// @brief Returns the timeout (the sending interval) for this message in milliseconds
			/// @returns This message's timeout (the sending interval) in milliseconds
			static std::uint32_t get_timeout();

		private:
			static constexpr std::uint32_t CYCLIC_MESSAGE_RATE_MS = 10000; ///< The transmit interval for this message as specified in NMEA2000
			static constexpr std::uint8_t LENGTH_BYTES = 20; ///< The size of this message in bytes
			static constexpr std::uint8_t DATUM_STRING_LENGTHS = 4; ///< The size of the datum codes in bytes

			std::shared_ptr<ControlFunction> senderControlFunction; ///< The sender of the message data
			std::string localDatum; ///< A 4 character ascii datum code. The first three chars are the datum ID.The fourth char is the local datum subdivision code or a null character if it is unknown or unused.
			std::string referenceDatum; ///< A 4 character ascii datum code that identifies the reference datum.
			std::int32_t deltaLatitude = 0; ///< Position in the local datum is offset from the position in the reference datum as indicated by this latitude delta. In units of 1x10E-7 degrees.
			std::int32_t deltaLongitude = 0; ///< Position in the local datum is offset from the position in the reference datum as indicated by this longitude delta. In units of 1x10E-7 degrees.
			std::int32_t deltaAltitude = 0; ///< The altitude delta in units of 0.01 meters. Positive values indicate Up.
			std::uint32_t messageTimestamp_ms = 0; ///< A timestamp in milliseconds when this message was last sent or received
		};
	} // namespace NMEA2000Messages
} // namespace isobus
#endif // NMEA2000_MESSAGE_DEFINITIONS_HPP
