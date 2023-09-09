#include <isobus.hpp>

#include "ObjectPool.cpp"

using namespace isobus;

auto can0 = std::make_shared<FlexCANT4Plugin>(0);
std::shared_ptr<InternalControlFunction>  ISOBUSControlFunction = nullptr;
std::shared_ptr<DiagnosticProtocol> ISOBUSDiagnostics = nullptr;
std::shared_ptr<VirtualTerminalClient> ExampleVirtualTerminalClient = nullptr;
std::shared_ptr<void> softKeyListener = nullptr;
std::shared_ptr<void> buttonListener = nullptr;

// A log sink for the CAN stack
class CustomLogger : public isobus::CANStackLogger
{
public:
	void sink_CAN_stack_log(CANStackLogger::LoggingLevel level, const std::string &text) override
	{
		switch (level)
		{
			case LoggingLevel::Debug:
			{
				Serial.print("[Debug]: ");
			}
			break;

			case LoggingLevel::Info:
			{
				Serial.print("[Info]: ");
			}
			break;

			case LoggingLevel::Warning:
			{
				Serial.print("[Warning]: ");
			}
			break;

			case LoggingLevel::Error:
			{
				Serial.print("[Error]: ");
			}
			break;

			case LoggingLevel::Critical:
			{
        Serial.print("[Critical]: ");
			}
			break;
		}
		 Serial.println(text.c_str());
	}
};

static CustomLogger logger;

// This callback will provide us with event driven notifications of button presses from the stack
void handleVTKeyEvents(const VirtualTerminalClient::VTKeyEvent &event)
{
	static std::uint32_t exampleNumberOutput = 214748364; // In the object pool the output number has an offset of -214748364 so we use this to represent 0.

	switch (event.keyEvent)
	{
		case VirtualTerminalClient::KeyActivationCode::ButtonUnlatchedOrReleased:
		{
			switch (event.objectID)
			{
				case Plus_Button:
				{
					ExampleVirtualTerminalClient->send_change_numeric_value(ButtonExampleNumber_VarNum, ++exampleNumberOutput);
				}
				break;

				case Minus_Button:
				{
					ExampleVirtualTerminalClient->send_change_numeric_value(ButtonExampleNumber_VarNum, --exampleNumberOutput);
				}
				break;

				case alarm_SoftKey:
				{
					ExampleVirtualTerminalClient->send_change_active_mask(example_WorkingSet, example_AlarmMask);
				}
				break;

				case acknowledgeAlarm_SoftKey:
				{
					ExampleVirtualTerminalClient->send_change_active_mask(example_WorkingSet, mainRunscreen_DataMask);
				}
				break;

				default:
					break;
			}
		}
		break;

		default:
			break;
	}
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  CANStackLogger::set_can_stack_logger_sink(&logger);
  CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Debug);
  // Optional, add delay() here to give you time to connect to the serial logger
  CANHardwareInterface::set_number_of_can_channels(1);
  CANHardwareInterface::assign_can_channel_frame_handler(0, can0);
  CANHardwareInterface::start();
  CANHardwareInterface::update();

  NAME deviceNAME(0);
  // Make sure you change these for your device
  // This is an example device that is using a manufacturer code that is currently unused at time of writing
  deviceNAME.set_arbitrary_address_capable(true);
  deviceNAME.set_industry_group(0);
  deviceNAME.set_device_class(0);
  deviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
  deviceNAME.set_identity_number(2);
  deviceNAME.set_ecu_instance(0);
  deviceNAME.set_function_instance(0);
  deviceNAME.set_device_class_instance(0);
  deviceNAME.set_manufacturer_code(64);
  // Change 0x81 to be your preferred address, but 0x81 is the base arbitrary address
  ISOBUSControlFunction = InternalControlFunction::create(deviceNAME, 0x81, 0);
  ISOBUSDiagnostics = std::make_shared<DiagnosticProtocol>(ISOBUSControlFunction);
  ISOBUSDiagnostics->initialize();

  // Change these to be specific to your device
  ISOBUSDiagnostics->set_product_identification_brand("Arduino");
  ISOBUSDiagnostics->set_product_identification_code("123456789");
  ISOBUSDiagnostics->set_product_identification_model("Example");
  ISOBUSDiagnostics->set_software_id_field(0, "0.0.1");
  ISOBUSDiagnostics->set_ecu_id_field(DiagnosticProtocol::ECUIdentificationFields::HardwareID, "Hardware ID");
  ISOBUSDiagnostics->set_ecu_id_field(DiagnosticProtocol::ECUIdentificationFields::Location, "The Aether");
  ISOBUSDiagnostics->set_ecu_id_field(DiagnosticProtocol::ECUIdentificationFields::ManufacturerName, "None");
  ISOBUSDiagnostics->set_ecu_id_field(DiagnosticProtocol::ECUIdentificationFields::PartNumber, "1234");
  ISOBUSDiagnostics->set_ecu_id_field(DiagnosticProtocol::ECUIdentificationFields::SerialNumber, "1");
  ISOBUSDiagnostics->set_ecu_id_field(DiagnosticProtocol::ECUIdentificationFields::Type, "AgISOStack");

  // Set up virtual terminal client
  const NAMEFilter filterVirtualTerminal(NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(NAME::Function::VirtualTerminal));
  const std::vector<NAMEFilter> vtNameFilters = { filterVirtualTerminal };
  auto TestPartnerVT = PartneredControlFunction::create(0, vtNameFilters);
  ExampleVirtualTerminalClient = std::make_shared<VirtualTerminalClient>(TestPartnerVT, ISOBUSControlFunction);
  ExampleVirtualTerminalClient->set_object_pool(0, VirtualTerminalClient::VTVersion::Version3, VT3TestPool, sizeof(VT3TestPool), "AIS1");
  softKeyListener = ExampleVirtualTerminalClient->add_vt_soft_key_event_listener(handleVTKeyEvents);
  buttonListener = ExampleVirtualTerminalClient->add_vt_button_event_listener(handleVTKeyEvents);
  ExampleVirtualTerminalClient->initialize(false);
}

void loop() {
  // put your main code here, to run repeatedly:
  ISOBUSDiagnostics->update(); // Update diagnostics interface
  ExampleVirtualTerminalClient->update(); // Update VT client
  CANHardwareInterface::update(); // Update CAN stack
}
