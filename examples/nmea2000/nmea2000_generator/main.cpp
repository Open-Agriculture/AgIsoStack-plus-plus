#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/nmea2000_message_definitions.hpp"
#include "isobus/isobus/nmea2000_message_interface.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <future>
#include <iostream>
#include <iterator>
#include <memory>

static std::atomic_bool running = { true };

void signal_handler(int)
{
	running = false;
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
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
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

	// The sequence identifier is set to an arbitrary value, but is in practice used to tie related messages together.
	// Example: A GNSS position message and a COG/SOG message that are not sent at the same time, but their sequence identifiers are the same,
	// then the data can be seen as taken at the same time.
	const auto sequenceIdentifier = 13;

	// Enable and configure the messages we want to send
	n2kInterface.set_enable_sending_cog_sog_cyclically(true);
	auto &cog_sog_message = n2kInterface.get_cog_sog_transmit_message();
	cog_sog_message.set_sequence_id(sequenceIdentifier);
	cog_sog_message.set_course_over_ground_reference(isobus::NMEA2000Messages::CourseOverGroundSpeedOverGroundRapidUpdate::CourseOverGroundReference::Error);
	cog_sog_message.set_course_over_ground(43633); // 4.3633 radians = 250 degrees
	cog_sog_message.set_speed_over_ground(200); // 2 m/s = 7.2 km/h

	n2kInterface.set_enable_sending_datum_cyclically(true);
	auto &datum_message = n2kInterface.get_datum_transmit_message();
	datum_message.set_local_datum("W84");
	datum_message.set_delta_latitude(1234000); // 0.1234 degrees
	datum_message.set_delta_longitude(5678000); // 0.5678 degrees
	datum_message.set_delta_altitude(98); // 0.98 meters
	datum_message.set_reference_datum("WGS84");

	n2kInterface.set_enable_sending_gnss_position_data_cyclically(true);
	auto &position_data_message = n2kInterface.get_gnss_position_data_transmit_message();
	position_data_message.set_sequence_id(sequenceIdentifier);
	auto daysSinceEpoch = std::chrono::duration_cast<std::chrono::hours>(std::chrono::system_clock::now().time_since_epoch()).count() / 24;
	position_data_message.set_position_date(static_cast<std::uint16_t>(daysSinceEpoch));
	auto secondsSinceMidnight = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() % (24 * 60 * 60);
	position_data_message.set_position_time(static_cast<std::uint32_t>(secondsSinceMidnight / 0.0001));
	position_data_message.set_latitude(static_cast<int64_t>(51.69917 / 1E-16)); // 51.69917 degrees
	position_data_message.set_longitude(static_cast<int64_t>(5.30417 / 1E-16)); // 5.30417 degrees
	position_data_message.set_altitude(static_cast<int64_t>(1.23 / 1E-06)); // 1.23 meters
	position_data_message.set_type_of_system(isobus::NMEA2000Messages::GNSSPositionData::TypeOfSystem::GPSPlusSBASPlusGLONASS);
	position_data_message.set_gnss_method(isobus::NMEA2000Messages::GNSSPositionData::GNSSMethod::RTKFixedInteger);
	position_data_message.set_integrity(isobus::NMEA2000Messages::GNSSPositionData::Integrity::Caution);
	position_data_message.set_number_of_space_vehicles(12); // 12 satellites
	position_data_message.set_horizontal_dilution_of_precision(-123); // -1.23
	position_data_message.set_positional_dilution_of_precision(-456); // -4.56
	position_data_message.set_geoidal_separation(-789); // -7.89 meters
	position_data_message.set_number_of_reference_stations(3);
	for (uint8_t i = 0; i < 3; i++)
	{
		position_data_message.set_reference_station(i, // Index
		                                            i + 1, // Station ID
		                                            isobus::NMEA2000Messages::GNSSPositionData::TypeOfSystem::GPSPlusGLONASS, // Type of system
		                                            i * 150 // Arbitrary age of correction (1.5s * i)
		);
	}

	n2kInterface.set_enable_sending_position_rapid_update_cyclically(true);
	n2kInterface.get_position_rapid_update_transmit_message().set_latitude(static_cast<std::int32_t>(51.69917 / 1E-07)); // 51.69917 degrees
	n2kInterface.get_position_rapid_update_transmit_message().set_longitude(static_cast<std::int32_t>(5.30417 / 1E-07)); // 5.30417 degrees

	n2kInterface.set_enable_sending_rate_of_turn_cyclically(true);
	n2kInterface.get_rate_of_turn_transmit_message().set_sequence_id(sequenceIdentifier);
	n2kInterface.get_rate_of_turn_transmit_message().set_rate_of_turn(static_cast<std::int32_t>(-1.234 / 3.125E-08)); // -1.234 radians/s = -70.7 degrees/s

	n2kInterface.set_enable_sending_vessel_heading_cyclically(true);
	auto &vessel_heading_message = n2kInterface.get_vessel_heading_transmit_message();
	vessel_heading_message.set_sequence_id(sequenceIdentifier);
	vessel_heading_message.set_heading(43633); // 4.3633 radians = 250 degrees
	vessel_heading_message.set_magnetic_deviation(-4363); // -0.4363 radians = -25 degrees
	vessel_heading_message.set_magnetic_variation(-5236); // -0.5236 radians = -30 degrees
	vessel_heading_message.set_sensor_reference(isobus::NMEA2000Messages::VesselHeading::HeadingSensorReference::Error);

	std::cout << "Starting to send NMEA2K messages. Press Ctrl+C to stop." << std::endl;
	while (running)
	{
		// Update the NMEA2K interface periodically so that it can send messages
		n2kInterface.update();

		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	isobus::CANHardwareInterface::stop();
	return 0;
}
