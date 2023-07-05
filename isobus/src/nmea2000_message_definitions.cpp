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

		std::uint16_t VesselHeading::get_raw_heading() const
		{
			return headingReading;
		}

		float VesselHeading::get_heading() const
		{
			return (headingReading * 0.0001f);
		}

		bool VesselHeading::set_heading(std::uint16_t heading)
		{
			bool retVal = (heading != headingReading);
			headingReading = heading;
			return retVal;
		}

		std::int16_t VesselHeading::get_raw_magnetic_deviation() const
		{
			return magneticDeviation;
		}

		float VesselHeading::get_magnetic_deviation() const
		{
			return (magneticDeviation * 0.0001f);
		}

		bool VesselHeading::set_magnetic_deviation(std::uint16_t deviation)
		{
			bool retVal = (deviation != magneticDeviation);
			magneticDeviation = deviation;
			return retVal;
		}

		std::int16_t VesselHeading::get_raw_magnetic_variation() const
		{
			return magneticVariation;
		}

		float VesselHeading::get_magnetic_variation() const
		{
			return (magneticVariation * 0.0001f);
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

		void VesselHeading::serialize(std::vector<std::uint8_t> &buffer)
		{
			buffer.resize(CAN_DATA_LENGTH);
			buffer.at(0) = (sequenceID <= MAX_SEQUENCE_ID) ? sequenceID : 0xFF;
			buffer.at(1) = static_cast<std::uint8_t>(headingReading & 0xFF);
			buffer.at(2) = static_cast<std::uint8_t>((headingReading >> 8) & 0xFF);
			buffer.at(3) = static_cast<std::uint8_t>(magneticDeviation & 0xFF);
			buffer.at(4) = static_cast<std::uint8_t>((magneticDeviation >> 8) & 0xFF);
			buffer.at(5) = static_cast<std::uint8_t>(magneticVariation & 0xFF);
			buffer.at(6) = static_cast<std::uint8_t>((magneticVariation >> 8) & 0xFF);
			buffer.at(7) = static_cast<std::uint8_t>(sensorReference) & 0x03;
			buffer.at(7) |= 0xFC;
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

		std::int32_t RateOfTurn::get_raw_rate_of_turn() const
		{
			return rateOfTurn;
		}

		double RateOfTurn::get_rate_of_turn() const
		{
			return (rateOfTurn * (1.0 / 32.0 * 10E-6));
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

		void RateOfTurn::serialize(std::vector<std::uint8_t> &buffer)
		{
			buffer.resize(CAN_DATA_LENGTH);

			buffer.at(0) = (sequenceID <= MAX_SEQUENCE_ID) ? sequenceID : 0xFF;
			buffer.at(1) = static_cast<std::uint8_t>(rateOfTurn & 0xFF);
			buffer.at(2) = static_cast<std::uint8_t>((rateOfTurn >> 8) & 0xFF);
			buffer.at(3) = static_cast<std::uint8_t>((rateOfTurn >> 16) & 0xFF);
			buffer.at(4) = static_cast<std::uint8_t>((rateOfTurn >> 24) & 0xFF);
			buffer.at(5) = 0xFF; // Reserved bytes
			buffer.at(6) = 0xFF;
			buffer.at(7) = 0xFF;
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

		void PositionRapidUpdate::serialize(std::vector<std::uint8_t> &buffer)
		{
			buffer.resize(CAN_DATA_LENGTH);

			buffer.at(0) = static_cast<std::uint8_t>(latitude & 0xFF);
			buffer.at(1) = static_cast<std::uint8_t>((latitude >> 8) & 0xFF);
			buffer.at(2) = static_cast<std::uint8_t>((latitude >> 16) & 0xFF);
			buffer.at(3) = static_cast<std::uint8_t>((latitude >> 24) & 0xFF);
			buffer.at(4) = static_cast<std::uint8_t>(longitude & 0xFF);
			buffer.at(5) = static_cast<std::uint8_t>((longitude >> 8) & 0xFF);
			buffer.at(6) = static_cast<std::uint8_t>((longitude >> 16) & 0xFF);
			buffer.at(7) = static_cast<std::uint8_t>((longitude >> 24) & 0xFF);
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

		std::uint16_t CourseOverGroundSpeedOverGroundRapidUpdate::get_raw_course_over_ground() const
		{
			return courseOverGround;
		}

		float CourseOverGroundSpeedOverGroundRapidUpdate::get_course_over_ground() const
		{
			return (0.0001f * courseOverGround);
		}

		bool CourseOverGroundSpeedOverGroundRapidUpdate::set_course_over_ground(std::uint16_t course)
		{
			bool retVal = (courseOverGround != course);
			courseOverGround = course;
			return retVal;
		}

		std::uint16_t CourseOverGroundSpeedOverGroundRapidUpdate::get_raw_speed_over_ground() const
		{
			return speedOverGround;
		}

		float CourseOverGroundSpeedOverGroundRapidUpdate::get_speed_over_ground() const
		{
			return (0.01f * speedOverGround);
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

		void CourseOverGroundSpeedOverGroundRapidUpdate::serialize(std::vector<std::uint8_t> &buffer)
		{
			buffer.resize(CAN_DATA_LENGTH);

			buffer.at(0) = sequenceID;
			buffer.at(1) = (0xFC | static_cast<std::uint8_t>(cogReference));
			buffer.at(2) = static_cast<std::uint8_t>(courseOverGround & 0xFF);
			buffer.at(3) = static_cast<std::uint8_t>((courseOverGround >> 8) & 0xFF);
			buffer.at(4) = static_cast<std::uint8_t>(speedOverGround & 0xFF);
			buffer.at(5) = static_cast<std::uint8_t>((speedOverGround >> 8) & 0xFF);
			buffer.at(6) = 0xFF; // Reserved
			buffer.at(7) = 0xFF; // Reserved
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

		void PositionDeltaHighPrecisionRapidUpdate::serialize(std::vector<std::uint8_t> &buffer)
		{
			buffer.resize(CAN_DATA_LENGTH);

			buffer.at(0) = sequenceID;
			//! @todo Finish serializer
		}

		GNSSPositionData::GNSSPositionData(std::shared_ptr<ControlFunction> source) :
		  senderControlFunction(source)
		{
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
			return longitude;
		}

		double GNSSPositionData::get_longitude() const
		{
			return (longitude * 10E-16);
		}

		std::int32_t GNSSPositionData::get_geoidal_separation() const
		{
			return geoidalSeparation;
		}

		bool GNSSPositionData::set_geoidal_separation(std::int32_t separation)
		{
			bool retVal = (geoidalSeparation != separation);
			geoidalSeparation = separation;
			return retVal;
		}

		std::uint32_t GNSSPositionData::get_timestamp() const
		{
			return messageTimestamp_ms;
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

		GNSSPositionData::GNSSMethod GNSSPositionData::get_gnss_method() const
		{
			return method;
		}

		bool GNSSPositionData::set_gnss_method(GNSSMethod gnssFixMethod)
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

		void GNSSPositionData::serialize(std::vector<std::uint8_t> &buffer)
		{
			buffer.resize(MINIMUM_LENGTH_BYTES);

			buffer.at(0) = sequenceID;
			buffer.at(1) = static_cast<std::uint8_t>(positionDate & 0xFF);
			buffer.at(2) = static_cast<std::uint8_t>((positionDate >> 8) & 0xFF);
			buffer.at(3) = static_cast<std::uint8_t>(positionTime & 0xFF);
			buffer.at(4) = static_cast<std::uint8_t>((positionTime >> 8) & 0xFF);
			buffer.at(5) = static_cast<std::uint8_t>((positionTime >> 16) & 0xFF);
			buffer.at(6) = static_cast<std::uint8_t>((positionTime >> 24) & 0xFF);
			buffer.at(7) = static_cast<std::uint8_t>(latitude & 0xFF);
			buffer.at(8) = static_cast<std::uint8_t>((latitude >> 8) & 0xFF);
			buffer.at(9) = static_cast<std::uint8_t>((latitude >> 16) & 0xFF);
			buffer.at(10) = static_cast<std::uint8_t>((latitude >> 24) & 0xFF);
			buffer.at(11) = static_cast<std::uint8_t>((latitude >> 32) & 0xFF);
			buffer.at(12) = static_cast<std::uint8_t>((latitude >> 40) & 0xFF);
			buffer.at(13) = static_cast<std::uint8_t>((latitude >> 48) & 0xFF);
			buffer.at(14) = static_cast<std::uint8_t>((latitude >> 56) & 0xFF);
			buffer.at(15) = static_cast<std::uint8_t>(longitude & 0xFF);
			buffer.at(16) = static_cast<std::uint8_t>((longitude >> 8) & 0xFF);
			buffer.at(17) = static_cast<std::uint8_t>((longitude >> 16) & 0xFF);
			buffer.at(18) = static_cast<std::uint8_t>((longitude >> 24) & 0xFF);
			buffer.at(19) = static_cast<std::uint8_t>((longitude >> 32) & 0xFF);
			buffer.at(20) = static_cast<std::uint8_t>((longitude >> 40) & 0xFF);
			buffer.at(21) = static_cast<std::uint8_t>((longitude >> 48) & 0xFF);
			buffer.at(22) = static_cast<std::uint8_t>((longitude >> 56) & 0xFF);
			buffer.at(23) = static_cast<std::uint8_t>(altitude & 0xFF);
			buffer.at(24) = static_cast<std::uint8_t>((altitude >> 8) & 0xFF);
			buffer.at(25) = static_cast<std::uint8_t>((altitude >> 16) & 0xFF);
			buffer.at(26) = static_cast<std::uint8_t>((altitude >> 24) & 0xFF);
			buffer.at(27) = static_cast<std::uint8_t>((altitude >> 32) & 0xFF);
			buffer.at(28) = static_cast<std::uint8_t>((altitude >> 40) & 0xFF);
			buffer.at(29) = static_cast<std::uint8_t>((altitude >> 48) & 0xFF);
			buffer.at(30) = static_cast<std::uint8_t>((altitude >> 56) & 0xFF);
			buffer.at(31) = (static_cast<std::uint8_t>(systemType) & 0x0F);
			buffer.at(31) |= ((static_cast<std::uint8_t>(method) & 0x0F) << 4);
			buffer.at(32) = (static_cast<std::uint8_t>(integrityChecking) | 0xFC);
			buffer.at(33) = numberOfSpaceVehicles;
			buffer.at(34) = static_cast<std::uint8_t>(horizontalDilutionOfPrecision & 0xFF);
			buffer.at(35) = static_cast<std::uint8_t>((horizontalDilutionOfPrecision >> 8) & 0xFF);
			buffer.at(36) = static_cast<std::uint8_t>(positionalDilutionOfPrecision & 0xFF);
			buffer.at(37) = static_cast<std::uint8_t>((positionalDilutionOfPrecision >> 8) & 0xFF);
			buffer.at(38) = static_cast<std::uint8_t>(geoidalSeparation & 0xFF);
			buffer.at(39) = static_cast<std::uint8_t>((geoidalSeparation >> 8) & 0xFF);
			buffer.at(40) = static_cast<std::uint8_t>((geoidalSeparation >> 16) & 0xFF);
			buffer.at(41) = static_cast<std::uint8_t>((geoidalSeparation >> 24) & 0xFF);
			buffer.at(42) = get_number_of_reference_stations();

			for (std::uint8_t i = 0; i < get_number_of_reference_stations(); i++)
			{
				buffer.push_back((static_cast<std::uint8_t>(referenceStations.at(i).stationType) & 0x0F) |
				                 ((referenceStations.at(i).stationID & 0x0F) << 4));
				buffer.push_back(static_cast<std::uint8_t>(referenceStations.at(i).stationID >> 4));
				buffer.push_back(static_cast<std::uint8_t>(referenceStations.at(i).ageOfDGNSSCorrections & 0xFF));
				buffer.push_back(static_cast<std::uint8_t>((referenceStations.at(i).ageOfDGNSSCorrections >> 8) & 0xFF));
			}
		}

		Datum::Datum(std::shared_ptr<ControlFunction> source) :
		  senderControlFunction(source)
		{
			referenceDatum.resize(DATUM_STRING_LENGTHS);
			localDatum.resize(DATUM_STRING_LENGTHS);
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

			if (localDatum.length() != DATUM_STRING_LENGTHS)
			{
				localDatum.resize(DATUM_STRING_LENGTHS);
			}
			return retVal;
		}

		std::string Datum::get_reference_datum() const
		{
			return referenceDatum;
		}

		bool Datum::set_reference_datum(const std::string &datum)
		{
			bool retVal = (datum != referenceDatum);
			referenceDatum = datum;

			if (referenceDatum.length() != DATUM_STRING_LENGTHS)
			{
				referenceDatum.resize(DATUM_STRING_LENGTHS);
			}
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

		float Datum::get_delta_altitude() const
		{
			return (0.02f * deltaAltitude);
		}

		bool Datum::set_delta_altitude(std::int32_t delta)
		{
			bool retVal = (deltaAltitude != delta);
			deltaAltitude = delta;
			return retVal;
		}

		void Datum::serialize(std::vector<std::uint8_t> &buffer)
		{
			buffer.resize(LENGTH_BYTES);

			buffer.at(0) = localDatum.at(0);
			buffer.at(1) = localDatum.at(1);
			buffer.at(2) = localDatum.at(2);
			buffer.at(3) = localDatum.at(3);
			buffer.at(4) = static_cast<std::uint8_t>(deltaLatitude & 0xFF);
			buffer.at(5) = static_cast<std::uint8_t>((deltaLatitude >> 8) & 0xFF);
			buffer.at(6) = static_cast<std::uint8_t>((deltaLatitude >> 16) & 0xFF);
			buffer.at(7) = static_cast<std::uint8_t>((deltaLatitude >> 24) & 0xFF);
			buffer.at(8) = static_cast<std::uint8_t>(deltaLongitude & 0xFF);
			buffer.at(9) = static_cast<std::uint8_t>((deltaLongitude >> 8) & 0xFF);
			buffer.at(10) = static_cast<std::uint8_t>((deltaLongitude >> 16) & 0xFF);
			buffer.at(11) = static_cast<std::uint8_t>((deltaLongitude >> 24) & 0xFF);
			buffer.at(12) = static_cast<std::uint8_t>(deltaAltitude & 0xFF);
			buffer.at(13) = static_cast<std::uint8_t>((deltaAltitude >> 8) & 0xFF);
			buffer.at(14) = static_cast<std::uint8_t>((deltaAltitude >> 16) & 0xFF);
			buffer.at(15) = static_cast<std::uint8_t>((deltaAltitude >> 24) & 0xFF);
			buffer.at(16) = referenceDatum.at(0);
			buffer.at(17) = referenceDatum.at(1);
			buffer.at(18) = referenceDatum.at(2);
			buffer.at(19) = referenceDatum.at(3);
		}
	}
}
