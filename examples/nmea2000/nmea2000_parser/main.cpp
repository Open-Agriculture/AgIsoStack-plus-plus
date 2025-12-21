#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/nmea2000_message_definitions.hpp"
#include "isobus/isobus/nmea2000_message_interface.hpp"

#include <math.h>
#include <chrono>
#include <csignal>
#include <future>
#include <iostream>
#include <iterator>
#include <memory>
#include <thread>

#define PI 3.141592653589793238463

static std::atomic_bool running = { true };

void signal_handler(int)
{
	running = false;
}

void on_cog_sog_update(const std::shared_ptr<isobus::NMEA2000Messages::CourseOverGroundSpeedOverGroundRapidUpdate> message, bool changed)
{
	std::cout << "COG/SOG update: (updated=" << changed << ")" << std::endl;
	std::cout << "  SID: " << static_cast<int>(message->get_sequence_id()) << std::endl;
	std::cout << "  COG reference: " << static_cast<int>(message->get_course_over_ground_reference()) << std::endl;
	std::cout << "  COG: " << message->get_course_over_ground() / (PI / 180) << " degrees" << std::endl;
	std::cout << "  SOG: " << message->get_speed_over_ground() * 3.6 << " km/h" << std::endl;
}

void on_datum_update(const std::shared_ptr<isobus::NMEA2000Messages::Datum> message, bool changed)
{
	std::cout << "Datum update: (updated=" << changed << ")" << std::endl;
	std::cout << "  Local datum: " << message->get_local_datum() << std::endl;
	std::cout << "  Delta latitude: " << message->get_delta_latitude() << " degrees" << std::endl;
	std::cout << "  Delta longitude: " << message->get_delta_longitude() << " degrees" << std::endl;
	std::cout << "  Delta altitude: " << message->get_delta_altitude() << " m" << std::endl;
	std::cout << "  Reference datum: " << message->get_reference_datum() << std::endl;
}

void on_position_update(const std::shared_ptr<isobus::NMEA2000Messages::GNSSPositionData> message, bool changed)
{
	const auto daysSinceEpoch = std::chrono::duration_cast<std::chrono::hours>(std::chrono::system_clock::now().time_since_epoch()).count() / 24;
	const auto secondsSinceMidnight = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() % (24 * 60 * 60);

	std::cout << "Position update: (updated=" << changed << ")" << std::endl;

	std::cout << "  Date: " << static_cast<int>(message->get_position_date()) << " days since epoch"
	          << " (today is " << static_cast<int>(daysSinceEpoch) << ")" << std::endl;
	std::cout << "  Time: " << static_cast<int>(message->get_position_time()) << " seconds since midnight"
	          << " (now is " << static_cast<int>(secondsSinceMidnight) << ")" << std::endl;
	std::cout << "  Latitude: " << message->get_latitude() << " degrees" << std::endl;
	std::cout << "  Longitude: " << message->get_longitude() << " degrees" << std::endl;
	std::cout << "  Altitude: " << message->get_altitude() << " m" << std::endl;
	std::cout << "  GNSS type: " << static_cast<int>(message->get_gnss_method()) << std::endl;
	std::cout << "  Method: " << static_cast<int>(message->get_gnss_method()) << std::endl;
	std::cout << "  Number of satellites: " << static_cast<int>(message->get_number_of_space_vehicles()) << std::endl;
	std::cout << "  HDOP: " << message->get_horizontal_dilution_of_precision() << std::endl;
	std::cout << "  PDOP: " << message->get_positional_dilution_of_precision() << std::endl;
	std::cout << "  Geoidal separation: " << message->get_geoidal_separation() << " m" << std::endl;
	std::cout << "  Number of reference stations: " << static_cast<int>(message->get_number_of_reference_stations()) << std::endl;
	for (std::uint8_t i = 0; i < message->get_number_of_reference_stations(); i++)
	{
		std::cout << "    Reference station " << static_cast<int>(i) << ":" << std::endl;
		std::cout << "      Station ID: " << static_cast<int>(message->get_reference_station_id(i)) << std::endl;
		std::cout << "      Type of system: " << static_cast<int>(message->get_reference_station_system_type(i)) << std::endl;
		std::cout << "      Age of correction: " << message->get_reference_station_corrections_age(i) << " sec" << std::endl;
	}
}

void on_position_rapid_update(const std::shared_ptr<isobus::NMEA2000Messages::PositionRapidUpdate> message, bool changed)
{
	std::cout << "Position rapid update: (updated=" << changed << ")" << std::endl;
	std::cout << "  Latitude: " << message->get_latitude() << " degrees" << std::endl;
	std::cout << "  Longitude: " << message->get_longitude() << " degrees" << std::endl;
}

void on_turn_rate_update(const std::shared_ptr<isobus::NMEA2000Messages::RateOfTurn> message, bool changed)
{
	std::cout << "Rate of turn update: (updated=" << changed << ")" << std::endl;
	std::cout << "  SID: " << static_cast<int>(message->get_sequence_id()) << std::endl;
	std::cout << "  Rate of turn: " << message->get_rate_of_turn() / (PI / 180) << " degrees/s" << std::endl;
}

void on_vessel_heading_update(const std::shared_ptr<isobus::NMEA2000Messages::VesselHeading> message, bool changed)
{
	std::cout << "Vessel heading update: (updated=" << changed << ")" << std::endl;
	std::cout << "  SID: " << static_cast<int>(message->get_sequence_id()) << std::endl;
	std::cout << "  Heading: " << message->get_heading() / (PI / 180) << " degrees" << std::endl;
	std::cout << "  Magnetic deviation: " << message->get_magnetic_deviation() / (PI / 180) << " degrees" << std::endl;
	std::cout << "  Magnetic variation: " << message->get_magnetic_variation() / (PI / 180) << " degrees" << std::endl;
	std::cout << "  Sensor reference: " << static_cast<int>(message->get_sensor_reference()) << std::endl;
}

int main()
{
	std::signal(SIGINT, signal_handler);

	std::shared_ptr<isobus::CANHardwarePlugin> canDriver = nullptr;
#if defined(ISOBUS_SOCKETCAN_AVAILABLE)
	canDriver = std::make_shared<isobus::SocketCANInterface>("vcan0");
#elif defined(ISOBUS_WINDOWSPCANBASIC_AVAILABLE)
	canDriver = std::make_shared<isobus::PCANBasicWindowsPlugin>(PCAN_USBBUS1);
#elif defined(ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE)
	canDriver = std::make_shared<isobus::InnoMakerUSB2CANWindowsPlugin>(0); // CAN0
#elif defined(ISOBUS_MACCANPCAN_AVAILABLE)
	canDriver = std::make_shared<isobus::MacCANPCANPlugin>(PCAN_USBBUS1);
#elif defined(ISOBUS_SYS_TEC_AVAILABLE)
	canDriver = std::make_shared<isobus::SysTecWindowsPlugin>();
#endif
	if (nullptr == canDriver)
	{
		std::cout << "Unable to find a CAN driver. Please make sure you have one of the above drivers installed with the library." << std::endl;
		std::cout << "If you want to use a different driver, please add it to the list above." << std::endl;
		return -1;
	}

	isobus::CANHardwareInterface::set_number_of_can_channels(1);
	isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);

	if ((!isobus::CANHardwareInterface::start()) || (!canDriver->get_is_valid()))
	{
		std::cout << "Failed to start hardware interface. A CAN driver might be invalid." << std::endl;
		return -2;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	isobus::NAME TestDeviceNAME(0);

	//! Make sure you change these for your device!!!!
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(0);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SystemMonitor));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(1407);

	auto TestInternalECU = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0);

	// Make sure address claiming is done before we continue
	auto addressClaimedFuture = std::async(std::launch::async, [&TestInternalECU]() {
		while (!TestInternalECU->get_address_valid())
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
	});
	if (addressClaimedFuture.wait_for(std::chrono::seconds(5)) == std::future_status::timeout)
	{
		std::cout << "Address claiming failed. Please make sure that your internal control function can claim a valid address." << std::endl;
		return -3;
	}

	// Construct NMEA2K interface, defaulting to all messages disabled
	isobus::NMEA2000MessageInterface n2kInterface(TestInternalECU, false, false, false, false, false, false, false);
	n2kInterface.initialize();

	// Listen to incoming NMEA2K messages
	n2kInterface.get_course_speed_over_ground_rapid_update_event_publisher().add_listener(on_cog_sog_update);
	n2kInterface.get_datum_event_publisher().add_listener(on_datum_update);
	n2kInterface.get_gnss_position_data_event_publisher().add_listener(on_position_update);
	n2kInterface.get_position_rapid_update_event_publisher().add_listener(on_position_rapid_update);
	n2kInterface.get_rate_of_turn_event_publisher().add_listener(on_turn_rate_update);
	n2kInterface.get_vessel_heading_event_publisher().add_listener(on_vessel_heading_update);

	std::cout << "Starting to parse NMEA2K messages. Press Ctrl+C to stop." << std::endl;
	while (running)
	{
		// Do nothing forever, just wait for Ctrl+C, new NMEA2000 messages will be notified to us with events
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	isobus::CANHardwareInterface::stop();
	return 0;
}
