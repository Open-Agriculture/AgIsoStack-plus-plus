#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/isobus_file_server_client.hpp"

#include "console_logger.cpp"

#include <atomic>
#include <csignal>
#include <iostream>
#include <iterator>
#include <memory>

static std::atomic_bool running = { true };

void signal_handler(int)
{
	running = false;
}

enum class ExampleStateMachineState
{
	OpenFile,
	WaitForFileToBeOpen,
	WriteFile,
	WaitForFileWrite,
	CloseFile,
	GetVolumeInfo,
	WaitForVolumeInfo,
	ChangeToRoot,
	OpenListOfVolumes,
	WaitForOpenListOfVolumes,
	ReadListOfVolumes,
	ExampleComplete
};

int main()
{
	std::signal(SIGINT, signal_handler);
	isobus::CANStackLogger::set_can_stack_logger_sink(&logger);
	isobus::CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Debug);
	std::shared_ptr<isobus::CANHardwarePlugin> canDriver = nullptr;
#if defined(ISOBUS_SOCKETCAN_AVAILABLE)
	canDriver = std::make_shared<isobus::SocketCANInterface>("can0");
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
		std::cout << "Failed to start hardware interface. The CAN driver might be invalid." << std::endl;
		return -2;
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	isobus::NAME TestDeviceNAME(0);

	// Consider customizing these values to match your device
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(1);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(1407);

	std::vector<isobus::NAMEFilter> fsNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::FileServerOrPrinter));
	fsNameFilters.push_back(testFilter);

	auto TestInternalECU = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0);
	auto TestPartnerFS = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, fsNameFilters);
	auto TestFileServerClient = std::make_shared<isobus::FileServerClient>(TestPartnerFS, TestInternalECU);

	TestFileServerClient->initialize(true);

	ExampleStateMachineState state = ExampleStateMachineState::OpenFile;
	std::string fileNameToUse = "FSExampleFile.txt";
	std::uint8_t fileHandle = isobus::FileServerClient::INVALID_FILE_HANDLE;
	const std::string fileExampleContents = "This is an example file! Visit us on Github https://github.com/Open-Agriculture/AgIsoStack-plus-plus";
	bool volumeStatusReceived = false;

	while (running)
	{
		// A little state machine to run our example.
		// Most functions on FS client interface are async, and can take a variable amount of time to complete, so
		// you will need to have some kind of stateful wrapper to manage your file operations.
		// This is essentially unavoidable, as interacting with files over the bus is a fairly involved, slow process.
		//
		// This state machine demonstrates a bunch of different kinds of operations, and you may not need them all for your application.
		switch (state)
		{
			case ExampleStateMachineState::OpenFile:
			{
				if (TestFileServerClient->open_file(fileNameToUse, true, true, isobus::FileServerClient::FileOpenMode::OpenFileForReadingAndWriting, isobus::FileServerClient::FilePointerMode::AppendMode))
				{
					state = ExampleStateMachineState::WaitForFileToBeOpen;
					isobus::CANStackLogger::info("[Example]: Opening File");
				}
			}
			break;

			// While the interface tries to open the file, we wait and poll to see if it is open.
			case ExampleStateMachineState::WaitForFileToBeOpen:
			{
				// When we get a valid file handle, that means the file has been opened and is ready to be interacted with
				fileHandle = TestFileServerClient->get_file_handle(fileNameToUse);
				if (isobus::FileServerClient::INVALID_FILE_HANDLE != fileHandle)
				{
					state = ExampleStateMachineState::WriteFile;
					isobus::CANStackLogger::info("[Example]: File is open");
				}
			}
			break;

			case ExampleStateMachineState::WriteFile:
			{
				if (TestFileServerClient->write_file(fileHandle, reinterpret_cast<const std::uint8_t *>(fileExampleContents.data()), fileExampleContents.size(), nullptr))
				{
					state = ExampleStateMachineState::WaitForFileWrite;
					isobus::CANStackLogger::info("[Example]: Writing file");
				}
			}
			break;

			case ExampleStateMachineState::WaitForFileWrite:
			{
				if (isobus::FileServerClient::FileState::FileOpen == TestFileServerClient->get_file_state(fileHandle))
				{
					// If the file is back in the open state, then writing is done. This can be checked instead of waiting for a callback if you want.
					state = ExampleStateMachineState::CloseFile;
					isobus::CANStackLogger::info("[Example]: Write complete. Closing file.");
				}
			}
			break;

			// Let's clean up, and close the file.
			case ExampleStateMachineState::CloseFile:
			{
				if (TestFileServerClient->close_file(TestFileServerClient->get_file_handle(fileNameToUse)))
				{
					state = ExampleStateMachineState::GetVolumeInfo;
					isobus::CANStackLogger::info("[Example]: File Closed.");
				}
			}
			break;

			// You don't really need to query the volume info if you don't want to. We do it
			// here just to show how to do it. It's helpful if you want to see if you're dealing with a USB drive vs on-board disk or something.
			// Just be aware that you'll get different results depending on your current directory if you don't request a specific volume name.
			case ExampleStateMachineState::GetVolumeInfo:
			{
				TestFileServerClient->get_volume_status_event_dispatcher().add_listener([&volumeStatusReceived](isobus::FileServerClient::VolumeStatusInfo status) { volumeStatusReceived = true; });
				if (TestFileServerClient->request_current_volume_status("")) // A blank volume name requests the volume of our "current directory"
				{
					isobus::CANStackLogger::info("[Example]: Requesting current volume information");
					state = ExampleStateMachineState::WaitForVolumeInfo;
				}
			}
			break;

			case ExampleStateMachineState::WaitForVolumeInfo:
			{
				if (volumeStatusReceived)
				{
					isobus::CANStackLogger::info("[Example]: Done.");
					state = ExampleStateMachineState::ChangeToRoot;
				}
			}
			break;

			case ExampleStateMachineState::ChangeToRoot:
			{
				if (TestFileServerClient->change_directory(std::string("\\\\")))
				{
					isobus::CANStackLogger::info("[Example]: Changing to the root directory.");
					state = ExampleStateMachineState::OpenListOfVolumes;
				}
			}
			break;

			case ExampleStateMachineState::OpenListOfVolumes:
			{
				if (TestFileServerClient->open_file(std::string("."), false, false, isobus::FileServerClient::FileOpenMode::OpenDirectory, isobus::FileServerClient::FilePointerMode::RandomAccess))
				{
					isobus::CANStackLogger::info("[Example]: Requesting volume list.");
					state = ExampleStateMachineState::WaitForOpenListOfVolumes;
				}
			}
			break;

			case ExampleStateMachineState::WaitForOpenListOfVolumes:
			{
				// When we get a valid file handle, that means the directory has been opened and is ready to be interacted with
				fileHandle = TestFileServerClient->get_file_handle(".");
				if (isobus::FileServerClient::INVALID_FILE_HANDLE != fileHandle)
				{
					TestFileServerClient->read_file(fileHandle, 2048, nullptr);
					state = ExampleStateMachineState::ExampleComplete; // todo wait
				}
			}
			break;

			// The example is complete! Do nothing until the user exits with ctrl+c
			default:
			case ExampleStateMachineState::ExampleComplete:
			{
			}
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	isobus::CANHardwareInterface::stop();
	return 0;
}
