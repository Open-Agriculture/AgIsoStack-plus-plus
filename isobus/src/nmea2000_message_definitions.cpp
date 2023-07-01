//================================================================================================
/// @file nmea2000_message_definitions.cpp
///
/// @brief This file contains class implementation will comprise the individual components
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
#include "isobus/isobus/nmea2000_message_definitions.hpp"

namespace isobus
{
	namespace NMEA2000Messages
	{
		VesselHeading::VesselHeading(std::shared_ptr<ControlFunction> source) :
		  senderControlFunction(source)
		{
		}

		std::shared_ptr<ControlFunction> VesselHeading::get_control_function() const
		{
			return senderControlFunction;
		}

		std::uint32_t VesselHeading::get_timestamp() const
		{
			return messageTimestamp_ms;
		}

		bool VesselHeading::set_timestamp(std::uint32_t timestamp)
		{
			bool retVal = (timestamp != messageTimestamp_ms);
			messageTimestamp_ms = timestamp;
			return retVal;
		}

		std::uint16_t VesselHeading::get_heading() const
		{
			return headingReading;
		}

		bool VesselHeading::set_heading(std::uint16_t heading)
		{
			bool retVal = (heading != headingReading);
			headingReading = heading;
			return retVal;
		}

		std::int16_t VesselHeading::get_magnetic_deviation() const
		{
			return magneticDeviation;
		}

		bool VesselHeading::set_magnetic_deviation(std::uint16_t deviation)
		{
			bool retVal = (deviation != magneticDeviation);
			magneticDeviation = deviation;
			return retVal;
		}

		std::int16_t VesselHeading::get_magnetic_variation() const
		{
			return magneticVariation;
		}

		bool VesselHeading::set_magnetic_variation(std::int16_t variation)
		{
			bool retVal = (variation != magneticVariation);
			magneticVariation = variation;
			return retVal;
		}

		std::uint8_t VesselHeading::get_sequence_id() const
		{
			return sequenceID;
		}

		bool VesselHeading::set_sequence_id(std::uint8_t sequenceNumber)
		{
			bool retVal = (sequenceNumber != sequenceID);
			sequenceID = sequenceNumber;
			return retVal;
		}

		VesselHeading::HeadingSensorReference VesselHeading::get_sensor_reference() const
		{
			return sensorReference;
		}

		bool VesselHeading::set_sensor_reference(HeadingSensorReference reference)
		{
			bool retVal = (sensorReference != reference);
			sensorReference = reference;
			return retVal;
		}

		RateOfTurn::RateOfTurn(std::shared_ptr<ControlFunction> source) :
		  senderControlFunction(source)
		{
		}

		std::shared_ptr<ControlFunction> RateOfTurn::get_control_function() const
		{
			return senderControlFunction;
		}

		std::uint32_t RateOfTurn::get_timestamp() const
		{
			return messageTimestamp_ms;
		}

		bool RateOfTurn::set_timestamp(std::uint32_t timestamp)
		{
			bool retVal = (messageTimestamp_ms != timestamp);
			messageTimestamp_ms = timestamp;
			return retVal;
		}

		std::int32_t RateOfTurn::get_rate_of_turn() const
		{
			return rateOfTurn;
		}

		bool RateOfTurn::set_rate_of_turn(std::int32_t turnRate)
		{
			bool retVal = (rateOfTurn != turnRate);
			rateOfTurn = turnRate;
			return retVal;
		}

		std::uint8_t RateOfTurn::get_sequence_id() const
		{
			return sequenceID;
		}

		bool RateOfTurn::set_sequence_id(std::uint8_t sequenceNumber)
		{
			bool retVal = (sequenceID != sequenceNumber);
			sequenceID = sequenceNumber;
			return retVal;
		}

		PositionRapidUpdate::PositionRapidUpdate(std::shared_ptr<ControlFunction> source) :
		  senderControlFunction(source)
		{
		}

		std::shared_ptr<ControlFunction> PositionRapidUpdate::get_control_function() const
		{
			return senderControlFunction;
		}

		std::uint32_t PositionRapidUpdate::get_timestamp() const
		{
			return messageTimestamp_ms;
		}

		bool PositionRapidUpdate::set_timestamp(std::uint32_t timestamp)
		{
			bool retVal = (messageTimestamp_ms != timestamp);
			messageTimestamp_ms = timestamp;
			return retVal;
		}

		std::int32_t PositionRapidUpdate::get_raw_latitude() const
		{
			return latitude;
		}

		double PositionRapidUpdate::get_latitude() const
		{
			return (latitude * 10E-7);
		}

		double PositionRapidUpdate::get_longitude() const
		{
			return (longitude * 10E-7);
		}

		std::int32_t PositionRapidUpdate::get_raw_longitude() const
		{
			return longitude;
		}

		bool PositionRapidUpdate::set_latitude(std::int32_t latitudeToSet)
		{
			bool retVal = (latitude != latitudeToSet);
			latitude = latitudeToSet;
			return retVal;
		}

		bool PositionRapidUpdate::set_longitude(std::int32_t longitudeToSet)
		{
			bool retVal = (longitude != longitudeToSet);
			longitude = longitudeToSet;
			return retVal;
		}

		CourseOverGroundSpeedOverGroundRapidUpdate::CourseOverGroundSpeedOverGroundRapidUpdate(std::shared_ptr<ControlFunction> source) :
		  senderControlFunction(source)
		{
		}

		std::shared_ptr<ControlFunction> CourseOverGroundSpeedOverGroundRapidUpdate::get_control_function() const
		{
			return senderControlFunction;
		}

		std::uint32_t CourseOverGroundSpeedOverGroundRapidUpdate::get_timestamp() const
		{
			return messageTimestamp_ms;
		}

		bool CourseOverGroundSpeedOverGroundRapidUpdate::set_timestamp(std::uint32_t timestamp)
		{
			bool retVal = (messageTimestamp_ms != timestamp);
			messageTimestamp_ms = timestamp;
			return retVal;
		}

		std::uint16_t CourseOverGroundSpeedOverGroundRapidUpdate::get_course_over_ground() const
		{
			return courseOverGround;
		}

		bool CourseOverGroundSpeedOverGroundRapidUpdate::set_course_over_ground(std::uint16_t course)
		{
			bool retVal = (courseOverGround != course);
			courseOverGround = course;
			return retVal;
		}

		std::uint16_t CourseOverGroundSpeedOverGroundRapidUpdate::get_speed_over_ground() const
		{
			return speedOverGround;
		}

		bool CourseOverGroundSpeedOverGroundRapidUpdate::set_speed_over_ground(std::uint16_t speed)
		{
			bool retVal = (speedOverGround != speed);
			speedOverGround = speed;
			return retVal;
		}

		std::uint8_t CourseOverGroundSpeedOverGroundRapidUpdate::get_sequence_id() const
		{
			return sequenceID;
		}

		bool CourseOverGroundSpeedOverGroundRapidUpdate::set_sequence_id(std::uint8_t sequenceNumber)
		{
			bool retVal = (sequenceID != sequenceNumber);
			sequenceID = sequenceNumber;
			return retVal;
		}

		CourseOverGroundSpeedOverGroundRapidUpdate::CourseOverGroudReference CourseOverGroundSpeedOverGroundRapidUpdate::get_course_over_ground_reference() const
		{
			return cogReference;
		}

		bool CourseOverGroundSpeedOverGroundRapidUpdate::set_course_over_ground_reference(CourseOverGroudReference reference)
		{
			bool retVal = (cogReference != reference);
			cogReference = reference;
			return retVal;
		}

		PositionDeltaHighPrecisionRapidUpdate::PositionDeltaHighPrecisionRapidUpdate(std::shared_ptr<ControlFunction> source) :
		  senderControlFunction(source)
		{
		}

		std::shared_ptr<ControlFunction> PositionDeltaHighPrecisionRapidUpdate::get_control_function() const
		{
			return senderControlFunction;
		}

		std::uint32_t PositionDeltaHighPrecisionRapidUpdate::get_timestamp() const
		{
			return messageTimestamp_ms;
		}

		bool PositionDeltaHighPrecisionRapidUpdate::set_timestamp(std::uint32_t timestamp)
		{
			bool retVal = (messageTimestamp_ms != timestamp);
			messageTimestamp_ms = timestamp;
			return retVal;
		}

		std::int32_t PositionDeltaHighPrecisionRapidUpdate::get_raw_latitude_delta() const
		{
			return latitudeDelta;
		}

		double PositionDeltaHighPrecisionRapidUpdate::get_latitude_delta() const
		{
			return (latitudeDelta * 10E-16);
		}

		bool PositionDeltaHighPrecisionRapidUpdate::set_latitude_delta(std::int32_t delta)
		{
			bool retVal = (latitudeDelta != delta);
			latitudeDelta = delta;
			return retVal;
		}

		std::int32_t PositionDeltaHighPrecisionRapidUpdate::get_raw_longitude_delta() const
		{
			return longitudeDelta;
		}

		double PositionDeltaHighPrecisionRapidUpdate::get_longitude_delta() const
		{
			return (longitudeDelta * 10E-16);
		}

		bool PositionDeltaHighPrecisionRapidUpdate::set_longitude_delta(std::int32_t delta)
		{
			bool retVal = (longitudeDelta != delta);
			longitudeDelta = delta;
			return retVal;
		}

		std::uint8_t PositionDeltaHighPrecisionRapidUpdate::get_sequence_id() const
		{
			return sequenceID;
		}

		bool PositionDeltaHighPrecisionRapidUpdate::set_sequence_id(std::uint8_t sequenceNumber)
		{
			bool retVal = (sequenceNumber != sequenceID);
			sequenceID = sequenceNumber;
			return retVal;
		}

		std::uint8_t PositionDeltaHighPrecisionRapidUpdate::get_raw_time_delta() const
		{
			return timeDelta;
		}

		double PositionDeltaHighPrecisionRapidUpdate::get_time_delta() const
		{
			return (timeDelta * (5 * 10E-3));
		}

		bool PositionDeltaHighPrecisionRapidUpdate::set_time_delta(std::uint8_t delta)
		{
			bool retVal = (timeDelta != delta);
			timeDelta = delta;
			return retVal;
		}

		std::shared_ptr<ControlFunction> GNSSPositionData::get_control_function() const
		{
			return senderControlFunction;
		}

		std::int64_t GNSSPositionData::get_raw_latitude() const
		{
			return latitude;
		}

		double GNSSPositionData::get_latitude() const
		{
			return (latitude * 10E-16);
		}

		std::int64_t GNSSPositionData::get_raw_longitude() const
		{
			return std::int64_t();
		}

		double GNSSPositionData::get_longitude() const
		{
			return (longitude * 10E-16);
		}

		std::int32_t GNSSPositionData::get_geoidal_separation() const
		{
			return std::int32_t();
		}

		bool GNSSPositionData::set_geoidal_separation(std::int32_t separation)
		{
			return false;
		}

		std::uint32_t GNSSPositionData::get_timestamp() const
		{
			return std::uint32_t();
		}

		bool GNSSPositionData::set_timestamp(std::uint32_t timestamp)
		{
			bool retVal = (messageTimestamp_ms != timestamp);
			messageTimestamp_ms = timestamp;
			return retVal;
		}

		std::uint8_t GNSSPositionData::get_sequence_id() const
		{
			return sequenceID;
		}

		bool GNSSPositionData::set_sequence_id(std::uint8_t sequenceNumber)
		{
			bool retVal = (sequenceNumber != sequenceID);
			sequenceID = sequenceNumber;
			return retVal;
		}

		GNSSPositionData::TypeOfSystem GNSSPositionData::get_type_of_system() const
		{
			return systemType;
		}

		bool GNSSPositionData::set_type_of_system(TypeOfSystem type)
		{
			bool retVal = (systemType != type);
			systemType = type;
			return retVal;
		}

		GNSSPositionData::GNSSMEthod GNSSPositionData::get_gnss_method() const
		{
			return method;
		}

		bool GNSSPositionData::set_gnss_method(GNSSMEthod gnssFixMethod)
		{
			bool retVal = (method != gnssFixMethod);
			method = gnssFixMethod;
			return retVal;
		}

		GNSSPositionData::Integrity GNSSPositionData::get_integrity() const
		{
			return integrityChecking;
		}

		bool GNSSPositionData::set_integrity(Integrity integrity)
		{
			bool retVal = (integrityChecking != integrity);
			integrityChecking = integrity;
			return retVal;
		}

		std::uint8_t GNSSPositionData::get_number_of_space_vehicles() const
		{
			return numberOfSpaceVehicles;
		}

		bool GNSSPositionData::set_number_of_space_vehicles(std::uint8_t numberOfSVs)
		{
			bool retVal = (numberOfSpaceVehicles != numberOfSVs);
			numberOfSpaceVehicles = numberOfSVs;
			return retVal;
		}

		std::int16_t GNSSPositionData::get_horizontal_dilution_of_precision() const
		{
			return horizontalDilutionOfPrecision;
		}

		bool GNSSPositionData::set_horizontal_dilution_of_precision(std::int16_t hdop)
		{
			bool retVal = (horizontalDilutionOfPrecision != hdop);
			horizontalDilutionOfPrecision = hdop;
			return retVal;
		}

		std::int16_t GNSSPositionData::get_positional_dilution_of_precision() const
		{
			return positionalDilutionOfPrecision;
		}

		bool GNSSPositionData::set_positional_dilution_of_precision(std::int16_t pdop)
		{
			bool retVal = (positionalDilutionOfPrecision != pdop);
			positionalDilutionOfPrecision = pdop;
			return retVal;
		}

		std::uint8_t GNSSPositionData::get_number_of_reference_stations() const
		{
			return referenceStations.size();
		}

		bool GNSSPositionData::set_number_of_reference_stations(std::uint8_t stations)
		{
			bool retVal = (referenceStations.size() != stations);
			referenceStations.resize(stations);
			return retVal;
		}

		std::shared_ptr<ControlFunction> Datum::get_control_function() const
		{
			return senderControlFunction;
		}

		std::uint32_t Datum::get_timestamp() const
		{
			return messageTimestamp_ms;
		}

		bool Datum::set_timestamp(std::uint32_t timestamp)
		{
			bool retVal = (messageTimestamp_ms != timestamp);
			messageTimestamp_ms = timestamp;
			return retVal;
		}

		std::string Datum::get_local_datum() const
		{
			return localDatum;
		}

		bool Datum::set_local_datum(const std::string &datum)
		{
			bool retVal = (datum != localDatum);
			localDatum = datum;
			return retVal;
		}

		std::int32_t Datum::get_raw_delta_latitude() const
		{
			return deltaLatitude;
		}

		double Datum::get_delta_latitude() const
		{
			return (deltaLatitude * 10E-7);
		}

		bool Datum::set_delta_latitude(std::int32_t delta)
		{
			bool retVal = (deltaLatitude != delta);
			deltaLatitude = delta;
			return retVal;
		}

		double Datum::get_delta_longitude() const
		{
			return (deltaLongitude * 10E-7);
		}

		std::int32_t Datum::get_raw_delta_longitude() const
		{
			return deltaLongitude;
		}

		bool Datum::set_delta_longitude(std::int32_t delta)
		{
			bool retVal = (deltaLongitude != delta);
			deltaLongitude = delta;
			return retVal;
		}

		std::int32_t Datum::get_raw_delta_altitude() const
		{
			return deltaAltitude;
		}

		double Datum::get_delta_altitude() const
		{
			return (0.02 * deltaAltitude);
		}

		bool Datum::set_delta_altitude(std::int32_t delta)
		{
			bool retVal = (deltaAltitude != delta);
			deltaAltitude = delta;
			return retVal;
		}
	}
}
