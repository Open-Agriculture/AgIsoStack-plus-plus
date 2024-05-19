#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_functionalities.hpp"
#include "isobus/utility/system_timing.hpp"

#include "helpers/control_function_helpers.hpp"
#include "helpers/messaging_helpers.hpp"

using namespace isobus;

class TestControlFunctionFunctionalities : public ControlFunctionFunctionalities
{
public:
	explicit TestControlFunctionFunctionalities(std::shared_ptr<InternalControlFunction> sourceControlFunction) :
	  ControlFunctionFunctionalities(sourceControlFunction)
	{
	}

	void test_wrapper_get_message_content(std::vector<std::uint8_t> &messageData)
	{
		ControlFunctionFunctionalities::get_message_content(messageData);
	}
};

TEST(CONTROL_FUNCTION_FUNCTIONALITIES_TESTS, CFFunctionalitiesTest)
{
	VirtualCANPlugin requesterPlugin;
	requesterPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	auto internalECU = test_helpers::claim_internal_control_function(0x01, 0);
	auto otherECU = test_helpers::force_claim_partnered_control_function(0x12, 0);

	TestControlFunctionFunctionalities cfFunctionalitiesUnderTest(internalECU);

	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::MinimumControlFunction));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::UniversalTerminalServer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::UniversalTerminalWorkingSet));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::AuxOInputs));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::AuxOFunctions));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::AuxNInputs));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::AuxNFunctions));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerBasicServer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerBasicClient));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerGeoServer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerGeoClient));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerSectionControlServer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerSectionControlClient));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::BasicTractorECUServer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::BasicTractorECUImplementClient));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TractorImplementManagementServer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TractorImplementManagementClient));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::FileServer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::FileServerClient));

	// Pretend we have weak internal termination
	cfFunctionalitiesUnderTest.set_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Type1ECUInternalWeakTermination, true);

	// None of these options should do anything when the functionality hasn't been enabled
	cfFunctionalitiesUnderTest.set_aux_N_functions_option_state(ControlFunctionFunctionalities::AuxNOptions::SupportsType8Function, true);
	cfFunctionalitiesUnderTest.set_aux_N_inputs_option_state(ControlFunctionFunctionalities::AuxNOptions::SupportsType9Function, true);
	cfFunctionalitiesUnderTest.set_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType0Function, true);
	cfFunctionalitiesUnderTest.set_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType1Function, true);
	cfFunctionalitiesUnderTest.set_basic_tractor_ECU_implement_client_option_state(ControlFunctionFunctionalities::BasicTractorECUOptions::Class2NoOptions, true);
	cfFunctionalitiesUnderTest.set_basic_tractor_ECU_server_option_state(ControlFunctionFunctionalities::BasicTractorECUOptions::Class1NoOptions, true);
	cfFunctionalitiesUnderTest.set_task_controller_geo_client_option(123);
	cfFunctionalitiesUnderTest.set_task_controller_geo_server_option_state(ControlFunctionFunctionalities::TaskControllerGeoServerOptions::PolygonBasedPrescriptionMapsAreSupported, true);
	cfFunctionalitiesUnderTest.set_tractor_implement_management_client_aux_valve_option(4, true, true);
	cfFunctionalitiesUnderTest.set_tractor_implement_management_client_option_state(ControlFunctionFunctionalities::TractorImplementManagementOptions::FrontPTOengagementCWIsSupported, true);
	cfFunctionalitiesUnderTest.set_tractor_implement_management_server_aux_valve_option(6, true, true);
	cfFunctionalitiesUnderTest.set_tractor_implement_management_server_option_state(ControlFunctionFunctionalities::TractorImplementManagementOptions::FrontPTOEngagementCCWIsSupported, true);

	EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::MinimumControlFunction));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::UniversalTerminalServer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::UniversalTerminalWorkingSet));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::AuxOInputs));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::AuxOFunctions));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::AuxNInputs));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::AuxNFunctions));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerBasicServer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerBasicClient));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerGeoServer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerGeoClient));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerSectionControlServer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerSectionControlClient));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::BasicTractorECUServer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::BasicTractorECUImplementClient));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TractorImplementManagementServer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TractorImplementManagementClient));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::FileServer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::FileServerClient));

	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_N_functions_option_state(ControlFunctionFunctionalities::AuxNOptions::SupportsType8Function));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_N_inputs_option_state(ControlFunctionFunctionalities::AuxNOptions::SupportsType9Function));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType0Function));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType1Function));
	EXPECT_EQ(0, cfFunctionalitiesUnderTest.get_task_controller_geo_client_option());
	EXPECT_EQ(0, cfFunctionalitiesUnderTest.get_task_controller_section_control_server_number_supported_booms());
	EXPECT_EQ(0, cfFunctionalitiesUnderTest.get_task_controller_section_control_server_number_supported_sections());
	EXPECT_EQ(0, cfFunctionalitiesUnderTest.get_tractor_implement_management_client_aux_valve_flow_supported(4));
	EXPECT_EQ(0, cfFunctionalitiesUnderTest.get_tractor_implement_management_client_aux_valve_state_supported(4));
	EXPECT_EQ(0, cfFunctionalitiesUnderTest.get_tractor_implement_management_server_aux_valve_flow_supported(6));
	EXPECT_EQ(0, cfFunctionalitiesUnderTest.get_tractor_implement_management_server_aux_valve_state_supported(6));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_tractor_implement_management_client_option_state(ControlFunctionFunctionalities::TractorImplementManagementOptions::FrontPTOengagementCWIsSupported));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_tractor_implement_management_server_option_state(ControlFunctionFunctionalities::TractorImplementManagementOptions::FrontPTOEngagementCCWIsSupported));

	// Min CF functionality should still be enabled by default
	EXPECT_EQ(1, cfFunctionalitiesUnderTest.get_functionality_generation(ControlFunctionFunctionalities::Functionalities::MinimumControlFunction));

	// Our weak termination should be supported
	EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Type1ECUInternalWeakTermination));

	// Test Min CF functionalities
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::SupportOfHeartbeatConsumer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::SupportOfHeartbeatProducer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Type2ECUInternalEndPointTermination));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Reserved));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::NoOptions));

	cfFunctionalitiesUnderTest.set_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Type1ECUInternalWeakTermination, false);
	cfFunctionalitiesUnderTest.set_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Type2ECUInternalEndPointTermination, true);
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::SupportOfHeartbeatConsumer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::SupportOfHeartbeatProducer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Type1ECUInternalWeakTermination));
	EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Type2ECUInternalEndPointTermination));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Reserved));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::NoOptions));

	cfFunctionalitiesUnderTest.set_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Type2ECUInternalEndPointTermination, false);
	cfFunctionalitiesUnderTest.set_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::SupportOfHeartbeatConsumer, true);
	EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::SupportOfHeartbeatConsumer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::SupportOfHeartbeatProducer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Type1ECUInternalWeakTermination));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Type2ECUInternalEndPointTermination));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Reserved));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::NoOptions));

	cfFunctionalitiesUnderTest.set_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::SupportOfHeartbeatConsumer, false);
	cfFunctionalitiesUnderTest.set_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::SupportOfHeartbeatProducer, true);
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::SupportOfHeartbeatConsumer));
	EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::SupportOfHeartbeatProducer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Type1ECUInternalWeakTermination));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Type2ECUInternalEndPointTermination));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Reserved));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::NoOptions));

	cfFunctionalitiesUnderTest.set_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::SupportOfHeartbeatProducer, false);
	cfFunctionalitiesUnderTest.set_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Type2ECUInternalEndPointTermination, true);
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::SupportOfHeartbeatConsumer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::SupportOfHeartbeatProducer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Type1ECUInternalWeakTermination));
	EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Type2ECUInternalEndPointTermination));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Reserved));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::NoOptions));

	cfFunctionalitiesUnderTest.set_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Type2ECUInternalEndPointTermination, false);
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::SupportOfHeartbeatConsumer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::SupportOfHeartbeatProducer));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Type1ECUInternalWeakTermination));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Type2ECUInternalEndPointTermination));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::Reserved));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_minimum_control_function_option_state(ControlFunctionFunctionalities::MinimumControlFunctionOptions::NoOptions));

	// Test the combinations of AUX-O Inputs
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::AuxOInputs, 1, true);
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::NoOptions));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::Reserved));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType0Function));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType1Function));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType2Function));

	cfFunctionalitiesUnderTest.set_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType0Function, true);
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::NoOptions));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::Reserved));
	EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType0Function));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType1Function));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType2Function));

	cfFunctionalitiesUnderTest.set_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType0Function, false);
	cfFunctionalitiesUnderTest.set_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType1Function, true);
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::NoOptions));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::Reserved));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType0Function));
	EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType1Function));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType2Function));

	cfFunctionalitiesUnderTest.set_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType1Function, false);
	cfFunctionalitiesUnderTest.set_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType2Function, true);
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::NoOptions));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::Reserved));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType0Function));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType1Function));
	EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType2Function));

	cfFunctionalitiesUnderTest.set_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType2Function, false);
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::NoOptions));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::Reserved));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType0Function));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType1Function));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_inputs_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType2Function));
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::AuxOInputs, 1, false);

	// Test the combinations of AUX-O functions
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::AuxOFunctions, 1, true);
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::NoOptions));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::Reserved));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType0Function));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType1Function));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType2Function));

	cfFunctionalitiesUnderTest.set_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType0Function, true);
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::NoOptions));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::Reserved));
	EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType0Function));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType1Function));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType2Function));

	cfFunctionalitiesUnderTest.set_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType0Function, false);
	cfFunctionalitiesUnderTest.set_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType1Function, true);
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::NoOptions));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::Reserved));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType0Function));
	EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType1Function));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType2Function));

	cfFunctionalitiesUnderTest.set_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType1Function, false);
	cfFunctionalitiesUnderTest.set_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType2Function, true);
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::NoOptions));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::Reserved));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType0Function));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType1Function));
	EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType2Function));

	cfFunctionalitiesUnderTest.set_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType2Function, false);
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::NoOptions));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::Reserved));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType0Function));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType1Function));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_O_functions_option_state(ControlFunctionFunctionalities::AuxOOptions::SupportsType2Function));
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::AuxOFunctions, 1, false);

	// Test the combinations of AUX-N Inputs
	auto aux_n_inputs_test_wrapper = [&cfFunctionalitiesUnderTest](ControlFunctionFunctionalities::AuxNOptions functionalityOption) {
		bool retVal = true;

		for (std::uint32_t i = 1; i <= 0x80; i = i << 1)
		{
			if (static_cast<ControlFunctionFunctionalities::AuxNOptions>(i) == functionalityOption)
			{
				EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_aux_N_inputs_option_state(static_cast<ControlFunctionFunctionalities::AuxNOptions>(i)));
			}
			else
			{
				EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_N_inputs_option_state(static_cast<ControlFunctionFunctionalities::AuxNOptions>(i)));
			}
		}
		return retVal;
	};

	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::AuxNInputs, 1, true);
	for (std::uint8_t i = static_cast<std::uint8_t>(ControlFunctionFunctionalities::AuxNOptions::SupportsType0Function); i != static_cast<std::uint8_t>(ControlFunctionFunctionalities::AuxNOptions::SupportsType14Function); i = i << 1)
	{
		cfFunctionalitiesUnderTest.set_aux_N_inputs_option_state(static_cast<ControlFunctionFunctionalities::AuxNOptions>(i), true);
		aux_n_inputs_test_wrapper(static_cast<ControlFunctionFunctionalities::AuxNOptions>(i));
		cfFunctionalitiesUnderTest.set_aux_N_inputs_option_state(static_cast<ControlFunctionFunctionalities::AuxNOptions>(i), false);
	}
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::AuxNInputs, 1, false);

	// Test the combinations of AUX-N Functions
	auto aux_n_functions_test_wrapper = [&cfFunctionalitiesUnderTest](ControlFunctionFunctionalities::AuxNOptions functionalityOption) {
		bool retVal = true;

		for (std::uint32_t i = 1; i <= 0x80; i = i << 1)
		{
			if (static_cast<ControlFunctionFunctionalities::AuxNOptions>(i) == functionalityOption)
			{
				EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_aux_N_functions_option_state(static_cast<ControlFunctionFunctionalities::AuxNOptions>(i)));
			}
			else
			{
				EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_aux_N_functions_option_state(static_cast<ControlFunctionFunctionalities::AuxNOptions>(i)));
			}
		}
		return retVal;
	};

	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::AuxNFunctions, 1, true);
	for (std::uint8_t i = static_cast<std::uint8_t>(ControlFunctionFunctionalities::AuxNOptions::SupportsType0Function); i != static_cast<std::uint8_t>(ControlFunctionFunctionalities::AuxNOptions::SupportsType14Function); i = i << 1)
	{
		cfFunctionalitiesUnderTest.set_aux_N_functions_option_state(static_cast<ControlFunctionFunctionalities::AuxNOptions>(i), true);
		aux_n_functions_test_wrapper(static_cast<ControlFunctionFunctionalities::AuxNOptions>(i));
		cfFunctionalitiesUnderTest.set_aux_N_functions_option_state(static_cast<ControlFunctionFunctionalities::AuxNOptions>(i), false);
	}
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::AuxNFunctions, 1, false);

	// Test TC GEO Server
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerGeoServer, 1, true);
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_task_controller_geo_server_option_state(ControlFunctionFunctionalities::TaskControllerGeoServerOptions::NoOptions));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_task_controller_geo_server_option_state(ControlFunctionFunctionalities::TaskControllerGeoServerOptions::PolygonBasedPrescriptionMapsAreSupported));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_task_controller_geo_server_option_state(ControlFunctionFunctionalities::TaskControllerGeoServerOptions::Reserved));

	cfFunctionalitiesUnderTest.set_task_controller_geo_server_option_state(ControlFunctionFunctionalities::TaskControllerGeoServerOptions::PolygonBasedPrescriptionMapsAreSupported, true);
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_task_controller_geo_server_option_state(ControlFunctionFunctionalities::TaskControllerGeoServerOptions::NoOptions));
	EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_task_controller_geo_server_option_state(ControlFunctionFunctionalities::TaskControllerGeoServerOptions::PolygonBasedPrescriptionMapsAreSupported));
	EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_task_controller_geo_server_option_state(ControlFunctionFunctionalities::TaskControllerGeoServerOptions::Reserved));
	cfFunctionalitiesUnderTest.set_task_controller_geo_server_option_state(ControlFunctionFunctionalities::TaskControllerGeoServerOptions::PolygonBasedPrescriptionMapsAreSupported, false);
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerGeoServer, 1, false);

	// Test TC GEO Client
	EXPECT_EQ(0, cfFunctionalitiesUnderTest.get_task_controller_geo_client_option());
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerGeoClient, 1, true);
	EXPECT_EQ(0, cfFunctionalitiesUnderTest.get_task_controller_geo_client_option());
	cfFunctionalitiesUnderTest.set_task_controller_geo_client_option(125);
	EXPECT_EQ(125, cfFunctionalitiesUnderTest.get_task_controller_geo_client_option());
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerGeoClient, 1, false);

	// Test TC section control server functionalities
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerSectionControlServer, 1, true);
	EXPECT_EQ(1, cfFunctionalitiesUnderTest.get_task_controller_section_control_server_number_supported_booms());
	EXPECT_EQ(1, cfFunctionalitiesUnderTest.get_task_controller_section_control_server_number_supported_sections());
	cfFunctionalitiesUnderTest.set_task_controller_section_control_server_option_state(123, 211);
	EXPECT_EQ(123, cfFunctionalitiesUnderTest.get_task_controller_section_control_server_number_supported_booms());
	EXPECT_EQ(211, cfFunctionalitiesUnderTest.get_task_controller_section_control_server_number_supported_sections());
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerSectionControlServer, 1, false);

	// Test TC section control client functionalities
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerSectionControlClient, 1, true);
	EXPECT_EQ(1, cfFunctionalitiesUnderTest.get_task_controller_section_control_client_number_supported_booms());
	EXPECT_EQ(1, cfFunctionalitiesUnderTest.get_task_controller_section_control_client_number_supported_sections());
	cfFunctionalitiesUnderTest.set_task_controller_section_control_client_option_state(123, 211);
	EXPECT_EQ(123, cfFunctionalitiesUnderTest.get_task_controller_section_control_client_number_supported_booms());
	EXPECT_EQ(211, cfFunctionalitiesUnderTest.get_task_controller_section_control_client_number_supported_sections());
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerSectionControlClient, 1, false);

	// Test the combinations of Basic Tractor ECU Server Functions
	auto basic_tecu_server_test_wrapper = [&cfFunctionalitiesUnderTest](ControlFunctionFunctionalities::BasicTractorECUOptions functionalityOption) {
		bool retVal = true;

		for (std::uint32_t i = 1; i <= 0x20; i = i << 1)
		{
			if (static_cast<ControlFunctionFunctionalities::BasicTractorECUOptions>(i) == functionalityOption)
			{
				EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_basic_tractor_ECU_server_option_state(static_cast<ControlFunctionFunctionalities::BasicTractorECUOptions>(i)));
			}
			else
			{
				EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_basic_tractor_ECU_server_option_state(static_cast<ControlFunctionFunctionalities::BasicTractorECUOptions>(i)));
			}
		}
		return retVal;
	};

	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::BasicTractorECUServer, 1, true);
	for (std::uint8_t i = static_cast<std::uint8_t>(ControlFunctionFunctionalities::BasicTractorECUOptions::Class1NoOptions); i <= 0x20; i = i << 1)
	{
		cfFunctionalitiesUnderTest.set_basic_tractor_ECU_server_option_state(static_cast<ControlFunctionFunctionalities::BasicTractorECUOptions>(i), true);
		basic_tecu_server_test_wrapper(static_cast<ControlFunctionFunctionalities::BasicTractorECUOptions>(i));
		cfFunctionalitiesUnderTest.set_basic_tractor_ECU_server_option_state(static_cast<ControlFunctionFunctionalities::BasicTractorECUOptions>(i), false);
	}
	EXPECT_TRUE(cfFunctionalitiesUnderTest.get_basic_tractor_ECU_server_option_state(ControlFunctionFunctionalities::BasicTractorECUOptions::TECUNotMeetingCompleteClass1Requirements));
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::BasicTractorECUServer, 1, false);

	// Test the combinations of Basic Tractor ECU Client Functions
	auto basic_tecu_client_test_wrapper = [&cfFunctionalitiesUnderTest](ControlFunctionFunctionalities::BasicTractorECUOptions functionalityOption) {
		bool retVal = true;

		for (std::uint32_t i = 1; i <= 0x20; i = i << 1)
		{
			if (static_cast<ControlFunctionFunctionalities::BasicTractorECUOptions>(i) == functionalityOption)
			{
				EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_basic_tractor_ECU_implement_client_option_state(static_cast<ControlFunctionFunctionalities::BasicTractorECUOptions>(i)));
			}
			else
			{
				EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_basic_tractor_ECU_implement_client_option_state(static_cast<ControlFunctionFunctionalities::BasicTractorECUOptions>(i)));
			}
		}
		return retVal;
	};

	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::BasicTractorECUImplementClient, 1, true);
	for (std::uint8_t i = static_cast<std::uint8_t>(ControlFunctionFunctionalities::BasicTractorECUOptions::Class1NoOptions); i <= 0x20; i = i << 1)
	{
		cfFunctionalitiesUnderTest.set_basic_tractor_ECU_implement_client_option_state(static_cast<ControlFunctionFunctionalities::BasicTractorECUOptions>(i), true);
		basic_tecu_client_test_wrapper(static_cast<ControlFunctionFunctionalities::BasicTractorECUOptions>(i));
		cfFunctionalitiesUnderTest.set_basic_tractor_ECU_implement_client_option_state(static_cast<ControlFunctionFunctionalities::BasicTractorECUOptions>(i), false);
	}
	EXPECT_TRUE(cfFunctionalitiesUnderTest.get_basic_tractor_ECU_implement_client_option_state(ControlFunctionFunctionalities::BasicTractorECUOptions::TECUNotMeetingCompleteClass1Requirements));
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::BasicTractorECUImplementClient, 1, false);

	// Test TIM server options
	auto tim_server_options_test_wrapper = [&cfFunctionalitiesUnderTest](ControlFunctionFunctionalities::TractorImplementManagementOptions functionalityOption) {
		bool retVal = true;

		for (std::uint_fast8_t i = 1; i <= static_cast<std::uint_fast8_t>(ControlFunctionFunctionalities::TractorImplementManagementOptions::GuidanceCurvatureIsSupported); i++)
		{
			if (static_cast<ControlFunctionFunctionalities::TractorImplementManagementOptions>(i) == functionalityOption)
			{
				EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_tractor_implement_management_server_option_state(static_cast<ControlFunctionFunctionalities::TractorImplementManagementOptions>(i)));
			}
			else
			{
				EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_tractor_implement_management_server_option_state(static_cast<ControlFunctionFunctionalities::TractorImplementManagementOptions>(i)));
			}
		}
		return retVal;
	};

	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TractorImplementManagementServer, 1, true);
	for (std::uint8_t i = static_cast<std::uint8_t>(ControlFunctionFunctionalities::TractorImplementManagementOptions::NoOptions); i <= static_cast<std::uint8_t>(ControlFunctionFunctionalities::TractorImplementManagementOptions::GuidanceCurvatureIsSupported); i++)
	{
		cfFunctionalitiesUnderTest.set_tractor_implement_management_server_option_state(static_cast<ControlFunctionFunctionalities::TractorImplementManagementOptions>(i), true);
		tim_server_options_test_wrapper(static_cast<ControlFunctionFunctionalities::TractorImplementManagementOptions>(i));
		cfFunctionalitiesUnderTest.set_tractor_implement_management_server_option_state(static_cast<ControlFunctionFunctionalities::TractorImplementManagementOptions>(i), false);
	}
	EXPECT_TRUE(cfFunctionalitiesUnderTest.get_tractor_implement_management_server_option_state(ControlFunctionFunctionalities::TractorImplementManagementOptions::NoOptions));
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TractorImplementManagementServer, 1, false);

	// Test TIM client options
	auto tim_client_options_test_wrapper = [&cfFunctionalitiesUnderTest](ControlFunctionFunctionalities::TractorImplementManagementOptions functionalityOption) {
		bool retVal = true;

		for (std::uint_fast8_t i = 1; i <= static_cast<std::uint_fast8_t>(ControlFunctionFunctionalities::TractorImplementManagementOptions::GuidanceCurvatureIsSupported); i++)
		{
			if (static_cast<ControlFunctionFunctionalities::TractorImplementManagementOptions>(i) == functionalityOption)
			{
				EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_tractor_implement_management_client_option_state(static_cast<ControlFunctionFunctionalities::TractorImplementManagementOptions>(i)));
			}
			else
			{
				EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_tractor_implement_management_client_option_state(static_cast<ControlFunctionFunctionalities::TractorImplementManagementOptions>(i)));
			}
		}
		return retVal;
	};

	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TractorImplementManagementClient, 1, true);
	for (std::uint8_t i = static_cast<std::uint8_t>(ControlFunctionFunctionalities::TractorImplementManagementOptions::NoOptions); i <= static_cast<std::uint8_t>(ControlFunctionFunctionalities::TractorImplementManagementOptions::GuidanceCurvatureIsSupported); i++)
	{
		cfFunctionalitiesUnderTest.set_tractor_implement_management_client_option_state(static_cast<ControlFunctionFunctionalities::TractorImplementManagementOptions>(i), true);
		tim_client_options_test_wrapper(static_cast<ControlFunctionFunctionalities::TractorImplementManagementOptions>(i));
		cfFunctionalitiesUnderTest.set_tractor_implement_management_client_option_state(static_cast<ControlFunctionFunctionalities::TractorImplementManagementOptions>(i), false);
	}
	EXPECT_TRUE(cfFunctionalitiesUnderTest.get_tractor_implement_management_client_option_state(ControlFunctionFunctionalities::TractorImplementManagementOptions::NoOptions));
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TractorImplementManagementClient, 1, false);

	// Test TIM client aux valve states
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TractorImplementManagementClient, 1, true);

	for (std::uint8_t i = 0; i < 32; i++)
	{
		cfFunctionalitiesUnderTest.set_tractor_implement_management_client_aux_valve_option(i, true, true);
		for (std::uint8_t j = 0; j < 32; j++)
		{
			if (i == j)
			{
				EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_tractor_implement_management_client_aux_valve_flow_supported(j));
				EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_tractor_implement_management_client_aux_valve_state_supported(j));
			}
			else
			{
				EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_tractor_implement_management_client_aux_valve_flow_supported(j));
				EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_tractor_implement_management_client_aux_valve_state_supported(j));
			}
		}
		cfFunctionalitiesUnderTest.set_tractor_implement_management_client_aux_valve_option(i, false, false);
	}
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TractorImplementManagementClient, 1, false);

	// Test TIM Server aux valve states
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TractorImplementManagementServer, 1, true);

	for (std::uint8_t i = 0; i < 32; i++)
	{
		cfFunctionalitiesUnderTest.set_tractor_implement_management_server_aux_valve_option(i, true, true);
		for (std::uint8_t j = 0; j < 32; j++)
		{
			if (i == j)
			{
				EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_tractor_implement_management_server_aux_valve_flow_supported(j));
				EXPECT_EQ(true, cfFunctionalitiesUnderTest.get_tractor_implement_management_server_aux_valve_state_supported(j));
			}
			else
			{
				EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_tractor_implement_management_server_aux_valve_flow_supported(j));
				EXPECT_EQ(false, cfFunctionalitiesUnderTest.get_tractor_implement_management_server_aux_valve_state_supported(j));
			}
		}
		cfFunctionalitiesUnderTest.set_tractor_implement_management_server_aux_valve_option(i, false, false);
	}
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TractorImplementManagementServer, 1, false);

	// Get the virtual CAN plugin back to a known state
	CANMessageFrame testFrame = {};
	while (!requesterPlugin.get_queue_empty())
	{
		requesterPlugin.read_frame(testFrame);
	}
	ASSERT_TRUE(requesterPlugin.get_queue_empty());

	// Simulate a request for the message
	testFrame.identifier = test_helpers::create_ext_can_id(6, 0xEA00, otherECU, internalECU);
	testFrame.data[0] = 0x8E;
	testFrame.data[1] = 0xFC;
	testFrame.data[2] = 0x00;
	testFrame.dataLength = 3;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	cfFunctionalitiesUnderTest.update(); // Updating manually since we're not integrated with the diagnostic protocol inside this test

	ASSERT_TRUE(requesterPlugin.read_frame(testFrame));
	ASSERT_TRUE(testFrame.isExtendedFrame);

	// If we got a message, we will assume it's OK and validate the message buffer instead, since the payload is probably BAM
	CANHardwareInterface::stop();

	std::vector<std::uint8_t> testMessageData;

	cfFunctionalitiesUnderTest.test_wrapper_get_message_content(testMessageData);

	// We should just have the minimum CF functionalities
	ASSERT_EQ(8, testMessageData.size());
	EXPECT_EQ(0xFF, testMessageData.at(0)); // Each control function shall respond with byte 1 set to FF16
	EXPECT_EQ(1, testMessageData.at(1)); // We are reporting 1 functionality
	EXPECT_EQ(0, testMessageData.at(2)); // Functionality 0 is min CF
	EXPECT_EQ(1, testMessageData.at(3)); // We specify generation 1 by default
	EXPECT_EQ(1, testMessageData.at(4)); // 1 Option byte
	EXPECT_EQ(0, testMessageData.at(5)); // No options
	EXPECT_EQ(0xFF, testMessageData.at(6)); // 0xFF Padding to 8 bytes
	EXPECT_EQ(0xFF, testMessageData.at(7)); // 0xFF Padding to 8 bytes

	// Say we are a UT working set
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::UniversalTerminalWorkingSet, 1, true);
	cfFunctionalitiesUnderTest.test_wrapper_get_message_content(testMessageData);

	// We should just have the minimum CF functionalities PLUS the UT client
	ASSERT_EQ(10, testMessageData.size());
	EXPECT_EQ(0xFF, testMessageData.at(0)); // Each control function shall respond with byte 1 set to FF16
	EXPECT_EQ(2, testMessageData.at(1)); // We are reporting 2 functionalities
	EXPECT_EQ(0, testMessageData.at(2)); // Functionality 0 is min CF
	EXPECT_EQ(1, testMessageData.at(3)); // We specify generation 1 by default
	EXPECT_EQ(1, testMessageData.at(4)); // 1 Option byte
	EXPECT_EQ(0, testMessageData.at(5)); // No options
	EXPECT_EQ(2, testMessageData.at(6)); // UT working set
	EXPECT_EQ(1, testMessageData.at(7)); // Generation 1. These generations are seemingly user defined and arbitrary, not the ISO standard version... no SPN defined...
	EXPECT_EQ(1, testMessageData.at(8)); // 1 Option byte
	EXPECT_EQ(0, testMessageData.at(9)); // No options

	// Add Aux-N
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::AuxNFunctions, 1, true);
	cfFunctionalitiesUnderTest.test_wrapper_get_message_content(testMessageData);
	ASSERT_EQ(15, testMessageData.size());
	EXPECT_EQ(0xFF, testMessageData.at(0)); // Each control function shall respond with byte 1 set to FF16
	EXPECT_EQ(3, testMessageData.at(1)); // We are reporting 3 functionalities
	EXPECT_EQ(0, testMessageData.at(2)); // Functionality 0 is min CF
	EXPECT_EQ(1, testMessageData.at(3)); // We specify generation 1 by default
	EXPECT_EQ(1, testMessageData.at(4)); // 1 Option byte
	EXPECT_EQ(0, testMessageData.at(5)); // No options
	EXPECT_EQ(2, testMessageData.at(6)); // UT working set
	EXPECT_EQ(1, testMessageData.at(7)); // Generation 1. These generations are seemingly user defined and arbitrary, not the ISO standard version... no SPN defined...
	EXPECT_EQ(1, testMessageData.at(8)); // 1 Option byte
	EXPECT_EQ(0, testMessageData.at(9)); // No options
	EXPECT_EQ(6, testMessageData.at(10)); // Functionality 6 is AUX N functions
	EXPECT_EQ(1, testMessageData.at(11)); // We specify generation 1
	EXPECT_EQ(2, testMessageData.at(12)); // 2 Option bytes
	EXPECT_EQ(0, testMessageData.at(13)); // No options
	EXPECT_EQ(0, testMessageData.at(14)); // No options

	// Add Task controller Client
	cfFunctionalitiesUnderTest.set_functionality_is_supported(ControlFunctionFunctionalities::Functionalities::TaskControllerSectionControlClient, 1, true);
	cfFunctionalitiesUnderTest.set_task_controller_section_control_client_option_state(1, 255); // 1 Boom, 255 sections
	cfFunctionalitiesUnderTest.test_wrapper_get_message_content(testMessageData);

	ASSERT_EQ(20, testMessageData.size());
	EXPECT_EQ(0xFF, testMessageData.at(0)); // Each control function shall respond with byte 1 set to FF16
	EXPECT_EQ(4, testMessageData.at(1)); // We are reporting 4 functionalities
	EXPECT_EQ(0, testMessageData.at(2)); // Functionality 0 is min CF
	EXPECT_EQ(1, testMessageData.at(3)); // We specify generation 1 by default
	EXPECT_EQ(1, testMessageData.at(4)); // 1 Option byte
	EXPECT_EQ(0, testMessageData.at(5)); // No options
	EXPECT_EQ(2, testMessageData.at(6)); // UT working set
	EXPECT_EQ(1, testMessageData.at(7)); // Generation 1. These generations are seemingly user defined and arbitrary, not the ISO standard version... no SPN defined...
	EXPECT_EQ(1, testMessageData.at(8)); // 1 Option byte
	EXPECT_EQ(0, testMessageData.at(9)); // No options
	EXPECT_EQ(6, testMessageData.at(10)); // Functionality 6 is AUX N functions
	EXPECT_EQ(1, testMessageData.at(11)); // We specify generation 1
	EXPECT_EQ(2, testMessageData.at(12)); // 2 Option bytes
	EXPECT_EQ(0, testMessageData.at(13)); // No options
	EXPECT_EQ(0, testMessageData.at(14)); // No options
	EXPECT_EQ(12, testMessageData.at(15)); // Functionality 12 is TC section control client
	EXPECT_EQ(1, testMessageData.at(16)); // We specify generation 1
	EXPECT_EQ(2, testMessageData.at(17)); // 2 Option bytes
	EXPECT_EQ(1, testMessageData.at(18)); // 1 Boom
	EXPECT_EQ(255, testMessageData.at(19)); // 255 Sections

	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	CANNetworkManager::CANNetwork.deactivate_control_function(otherECU);
}
