//================================================================================================
/// @file isobus_functionalities.hpp
///
/// @brief Defines a class that manages the control function functionalities message data.
/// (PGN 64654, 0xFC8E) as defined in ISO11783-12
///
/// @details The parameters defined here can be found at https://www.isobus.net/isobus/option
///
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#ifndef ISOBUS_FUNCTIONALITIES_HPP
#define ISOBUS_FUNCTIONALITIES_HPP

#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_parameter_group_number_request_protocol.hpp"
#include "isobus/utility/processing_flags.hpp"
#include "isobus/utility/thread_synchronization.hpp"

#include <list>
#include <vector>

namespace isobus
{
	class DiagnosticProtocol; // Forward declaration

	/// @brief Manages the control function functionalities message
	class ControlFunctionFunctionalities
	{
	public:
		/// @brief Enumerates the different functionalities that an ISOBUS ECU can report
		/// in the control function functionalities message.
		enum class Functionalities : std::uint8_t
		{
			MinimumControlFunction = 0,
			UniversalTerminalServer = 1,
			UniversalTerminalWorkingSet = 2,
			AuxOInputs = 3,
			AuxOFunctions = 4,
			AuxNInputs = 5,
			AuxNFunctions = 6,
			TaskControllerBasicServer = 7,
			TaskControllerBasicClient = 8,
			TaskControllerGeoServer = 9,
			TaskControllerGeoClient = 10,
			TaskControllerSectionControlServer = 11,
			TaskControllerSectionControlClient = 12,
			BasicTractorECUServer = 13,
			BasicTractorECUImplementClient = 14,
			TractorImplementManagementServer = 15,
			TractorImplementManagementClient = 16,
			FileServer = 17,
			FileServerClient = 18,

			ReservedRangeBegin = 19,
			MaxFunctionalityReserved = 255
		};

		/// @brief This parameter reports which minimum control function functionality options are supported.
		enum class MinimumControlFunctionOptions : std::uint8_t
		{
			NoOptions = 0x00,
			Type1ECUInternalWeakTermination = 0x01,
			Type2ECUInternalEndPointTermination = 0x02,
			SupportOfHeartbeatProducer = 0x04,
			SupportOfHeartbeatConsumer = 0x08,

			Reserved = 0xF0
		};

		/// @brief This parameter reports which auxiliary control type 1 functionality type functions are supported by
		/// an implement working set auxiliary function or an auxiliary function input unit.
		enum class AuxOOptions : std::uint8_t
		{
			NoOptions = 0x00,
			SupportsType0Function = 0x01,
			SupportsType1Function = 0x02,
			SupportsType2Function = 0x04,

			Reserved = 0xF8
		};

		/// @brief This parameter reports which auxiliary control type 2 functionality type functions are supported by
		/// an implement working set auxiliary function or an auxiliary function input unit.
		enum class AuxNOptions : std::uint16_t
		{
			NoOptions = 0x00,
			SupportsType0Function = 0x01,
			SupportsType1Function = 0x02,
			SupportsType2Function = 0x04,
			SupportsType3Function = 0x08,
			SupportsType4Function = 0x10,
			SupportsType5Function = 0x20,
			SupportsType6Function = 0x40,
			SupportsType7Function = 0x80,
			SupportsType8Function = 0x100,
			SupportsType9Function = 0x200,
			SupportsType10Function = 0x400,
			SupportsType11Function = 0x800,
			SupportsType12Function = 0x1000,
			SupportsType13Function = 0x2000,
			SupportsType14Function = 0x4000,

			Reserved = 0x8000
		};

		/// @brief This option byte reports which task controller geo functionality options are supported by an
		/// implement working set master or a task controller.
		enum class TaskControllerGeoServerOptions : std::uint8_t
		{
			NoOptions = 0x00,
			PolygonBasedPrescriptionMapsAreSupported = 0x01,

			Reserved = 0xFE
		};

		/// @brief This parameter reports which tractor ECU class and functionality options are supported by an implement
		/// working set master or a tractor ECU.
		enum class BasicTractorECUOptions : std::uint8_t
		{
			TECUNotMeetingCompleteClass1Requirements = 0x00,
			Class1NoOptions = 0x01,
			Class2NoOptions = 0x02,
			ClassRequiredLighting = 0x04,
			NavigationOption = 0x08,
			FrontHitchOption = 0x10,
			GuidanceOption = 0x20,
			Reserved = 0xC0
		};

		/// @brief This parameter reports which TIM options are supported by a TIM server or an implement
		/// working set master
		enum class TractorImplementManagementOptions
		{
			NoOptions = 0,
			FrontPTODisengagementIsSupported,
			FrontPTOEngagementCCWIsSupported,
			FrontPTOengagementCWIsSupported,
			FrontPTOspeedCCWIsSupported,
			FrontPTOspeedCWIsSupported,
			RearPTODisengagementIsSupported,
			RearPTOEngagementCCWIsSupported,
			RearPTOEngagementCWIsSupported,
			RearPTOSpeedCCWIsSupported,
			RearPTOSpeedCWIsSupported,
			FrontHitchMotionIsSupported,
			FrontHitchPositionIsSupported,
			RearHitchMotionIsSupported,
			RearHitchPositionIsSupported,
			VehicleSpeedInForwardDirectionIsSupported,
			VehicleSpeedInReverseDirectionIsSupported,
			VehicleSpeedStartMotionIsSupported,
			VehicleSpeedStopMotionIsSupported,
			VehicleSpeedForwardSetByServerIsSupported,
			VehicleSpeedReverseSetByServerIsSupported,
			VehicleSpeedChangeDirectionIsSupported,
			GuidanceCurvatureIsSupported
		};

		/// @brief Constructor for a ControlFunctionFunctionalities object
		/// @param[in] sourceControlFunction The control function to use when sending messages
		explicit ControlFunctionFunctionalities(std::shared_ptr<InternalControlFunction> sourceControlFunction);

		/// @brief Destructor for a ControlFunctionFunctionalities object
		~ControlFunctionFunctionalities();

		/// @brief Adds or removes a supported functionality
		/// @param[in] functionality The functionality to change
		/// @param[in] functionalityGeneration The generation of the functionality to report
		/// @param[in] isSupported If true, this class will add reporting as such on the ISOBUS. If false, it will not be reported on the bus.
		/// @note Minimum Control Function is enabled by default, and generally should not be disabled.
		void set_functionality_is_supported(Functionalities functionality, std::uint8_t functionalityGeneration, bool isSupported);

		/// @brief Returns if a functionality was previously configured with set_functionality_is_supported
		/// @param[in] functionality The functionality to check against
		/// @returns true if the specified functionality will be reported as being supported, otherwise false
		bool get_functionality_is_supported(Functionalities functionality);

		/// @brief Returns the generation that was set for the specified functionality when
		/// set_functionality_is_supported was called for that functionality.
		/// @param[in] functionality The functionality to check against
		/// @returns The generation associated with the specified functionality, or 0 if not configured
		std::uint8_t get_functionality_generation(Functionalities functionality);

		/// @brief Sets a minimum control function functionality option to a new state.
		/// @details Options set to true will be reported as "supported" to a requester of your
		/// control function functionalities within the "minimum control function" functionality section of the message.
		/// @param[in] option The option to set
		/// @param[in] optionState The state to set for the associated option
		void set_minimum_control_function_option_state(MinimumControlFunctionOptions option, bool optionState);

		/// @brief Returns the current state of the specified minimum control function functionality option
		/// @param[in] option The option to check the state of
		/// @returns The state of the option. If true, the option is being reported as "supported".
		bool get_minimum_control_function_option_state(MinimumControlFunctionOptions option);

		/// @brief Sets an AUX-O inputs functionality option to a new state.
		/// @details Options set to true will be reported as "supported" to a requester of your
		/// control function functionalities within the "AUX-O inputs" functionality section of the message.
		/// @param[in] option The option to set
		/// @param[in] optionState The state to set for the associated option
		void set_aux_O_inputs_option_state(AuxOOptions option, bool optionState);

		/// @brief Gets the state of an AUX-O inputs functionality option.
		/// @param[in] option The option to get
		/// @returns The state of the option. If true, the option is being reported as "supported".
		bool get_aux_O_inputs_option_state(AuxOOptions option);

		/// @brief Sets an AUX-O functions functionality option to a new state.
		/// @details Options set to true will be reported as "supported" to a requester of your
		/// control function functionalities within the "AUX-O functions" functionality section of the message.
		/// @param[in] option The option to set
		/// @param[in] optionState The state to set for the associated option
		void set_aux_O_functions_option_state(AuxOOptions option, bool optionState);

		/// @brief Gets the state of an AUX-O functions functionality option.
		/// @param[in] option The option to get
		/// @returns The state of the option. If true, the option is being reported as "supported".
		bool get_aux_O_functions_option_state(AuxOOptions option);

		/// @brief Sets an AUX-N inputs functionality option to a new state.
		/// @details Options set to true will be reported as "supported" to a requester of your
		/// control function functionalities within the "AUX-N inputs" functionality section of the message.
		/// @param[in] option The option to set
		/// @param[in] optionState The state to set for the associated option
		void set_aux_N_inputs_option_state(AuxNOptions option, bool optionState);

		/// @brief Gets the state of an AUX-N inputs functionality option.
		/// @param[in] option The option to get
		/// @returns The state of the option. If true, the option is being reported as "supported".
		bool get_aux_N_inputs_option_state(AuxNOptions option);

		/// @brief Sets an AUX-N functions functionality option to a new state.
		/// @details Options set to true will be reported as "supported" to a requester of your
		/// control function functionalities within the "AUX-N functions" functionality section of the message.
		/// @param[in] option The option to set
		/// @param[in] optionState The state to set for the associated option
		void set_aux_N_functions_option_state(AuxNOptions option, bool optionState);

		/// @brief Gets the state of an AUX-N functions functionality option.
		/// @param[in] option The option to get
		/// @returns The state of the option. If true, the option is being reported as "supported".
		bool get_aux_N_functions_option_state(AuxNOptions option);

		/// @brief Sets a task controller geo server functionality option to a new state.
		/// @details Options set to true will be reported as "supported" to a requester of your
		/// control function functionalities within the "task controller geo server" functionality section of the message.
		/// @param[in] option The option to set
		/// @param[in] optionState The state to set for the associated option
		void set_task_controller_geo_server_option_state(TaskControllerGeoServerOptions option, bool optionState);

		/// @brief Gets the state of a TC GEO server functionality option.
		/// @param[in] option The option to get
		/// @returns The state of the option. If true, the option is being reported as "supported".
		bool get_task_controller_geo_server_option_state(TaskControllerGeoServerOptions option);

		/// @brief Sets a task controller geo client's only functionality option, which is the number of control channels.
		/// @details The value you set will be reported to the requestor of your CF's functionalities
		/// within the "task controller geo client" functionality section of the message.
		/// @param[in] numberOfControlChannels The number of control channels your ECU supports for TC GEO (client side)
		void set_task_controller_geo_client_option(std::uint8_t numberOfControlChannels);

		/// @brief Gets the state of the only TC GEO client functionality option, which is the number of control channels.
		/// @returns The number of supported TC GEO client control channels that are supported
		std::uint8_t get_task_controller_geo_client_option();

		/// @brief Sets a task controller section control server's options
		/// @details The values set here will be reported as "supported" to a requester of your CF's functionalities
		/// @param[in] numberOfSupportedBooms The number of booms your application TC server supports
		/// @param[in] numberOfSupportedSections The number of sections your application TC server supports
		void set_task_controller_section_control_server_option_state(std::uint8_t numberOfSupportedBooms, std::uint8_t numberOfSupportedSections);

		/// @brief Gets the number of supported booms for the TC section control server functionality
		/// @returns The number of supported booms being reported in the TC section control server functionality
		std::uint8_t get_task_controller_section_control_server_number_supported_booms();

		/// @brief Gets the number of supported sections for the TC section control server functionality
		/// @returns The number of supported sections being reported in the TC section control server functionality
		std::uint8_t get_task_controller_section_control_server_number_supported_sections();

		/// @brief Sets a task controller section control client's options
		/// @details The values set here will be reported as "supported" to a requester of your CF's functionalities
		/// @param[in] numberOfSupportedBooms The number of booms your application TC client supports
		/// @param[in] numberOfSupportedSections The number of sections your application TC client supports
		void set_task_controller_section_control_client_option_state(std::uint8_t numberOfSupportedBooms, std::uint8_t numberOfSupportedSections);

		/// @brief Gets the number of supported booms for the TC section control client functionality
		/// @returns The number of supported booms being reported in the TC section control client functionality
		std::uint8_t get_task_controller_section_control_client_number_supported_booms();

		/// @brief Gets the number of supported sections for the TC section control client functionality
		/// @returns The number of supported sections being reported in the TC section control client functionality
		std::uint8_t get_task_controller_section_control_client_number_supported_sections();

		/// @brief Sets a tractor ECU server functionality option to a new state.
		/// @details The values set here will be reported as "supported" to a requester of your CF's functionalities
		/// @param[in] option The option to set
		/// @param[in] optionState The state to set for the associated option
		void set_basic_tractor_ECU_server_option_state(BasicTractorECUOptions option, bool optionState);

		/// @brief Gets the state of a basic tractor ECU server functionality option.
		/// @param[in] option The option to get
		/// @returns The state of the option. If true, the option is being reported as "supported".
		bool get_basic_tractor_ECU_server_option_state(BasicTractorECUOptions option);

		/// @brief Sets a tractor ECU client functionality option to a new state.
		/// @details The values set here will be reported as "supported" to a requester of your CF's functionalities
		/// @param[in] option The option to set
		/// @param[in] optionState The state to set for the associated option
		void set_basic_tractor_ECU_implement_client_option_state(BasicTractorECUOptions option, bool optionState);

		/// @brief Gets the state of a basic tractor ECU implement client functionality option.
		/// @param[in] option The option to get
		/// @returns The state of the option. If true, the option is being reported as "supported".
		bool get_basic_tractor_ECU_implement_client_option_state(BasicTractorECUOptions option);

		/// @brief Sets a tractor implement management (TIM) server functionality option to a new state.
		/// @details The values set here will be reported as "supported" to a requester of your CF's functionalities
		/// @param[in] option The option to set
		/// @param[in] optionState The state to set for the associated option
		void set_tractor_implement_management_server_option_state(TractorImplementManagementOptions option, bool optionState);

		/// @brief Gets the state of a basic tractor implement management client functionality option.
		/// @param[in] option The option to get
		/// @returns The state of the option. If true, the option is being reported as "supported".
		bool get_tractor_implement_management_server_option_state(TractorImplementManagementOptions option);

		/// @brief Sets a tractor implement management (TIM) server aux valve's functionality options to a new state
		/// @details The values set here will be reported as "supported" to a requester of your CF's functionalities
		/// @param[in] auxValveIndex The index of the aux valve to set, between 0 and 31
		/// @param[in] stateSupported Set to true to indicate you support the state of this valve, otherwise false
		/// @param[in] flowSupported Set to true to indicate your support aux valve flow with this valve
		void set_tractor_implement_management_server_aux_valve_option(std::uint8_t auxValveIndex, bool stateSupported, bool flowSupported);

		/// @brief Returns if a particular aux valve's state control is supported in the TIM server functionality
		/// @param[in] auxValveIndex The index of the aux valve to check
		/// @returns true if the aux valve's state you specified is being reported as "supported".
		bool get_tractor_implement_management_server_aux_valve_state_supported(std::uint8_t auxValveIndex);

		/// @brief Returns if a particular aux valve's flow control is supported in the TIM server functionality
		/// @param[in] auxValveIndex The index of the aux valve to check
		/// @returns true if the aux valve you specified is being reported as "supported".
		bool get_tractor_implement_management_server_aux_valve_flow_supported(std::uint8_t auxValveIndex);

		/// @brief Sets a tractor implement management (TIM) client functionality option to a new state.
		/// @details The values set here will be reported as "supported" to a requester of your CF's functionalities
		/// @param[in] option The option to set
		/// @param[in] optionState The state to set for the associated option
		void set_tractor_implement_management_client_option_state(TractorImplementManagementOptions option, bool optionState);

		/// @brief Gets the state of a TIM client functionality option.
		/// @param[in] option The option to get
		/// @returns The state of the option. If true, the option is being reported as "supported".
		bool get_tractor_implement_management_client_option_state(TractorImplementManagementOptions option);

		/// @brief Sets a tractor implement management (TIM) client aux valve's functionality options to a new state
		/// @details The values set here will be reported as "supported" to a requester of your CF's functionalities
		/// @param[in] auxValveIndex The index of the aux valve to set, between 0 and 31
		/// @param[in] stateSupported Set to true to indicate you support the state of this valve, otherwise false
		/// @param[in] flowSupported Set to true to indicate your support aux valve flow with this valve
		void set_tractor_implement_management_client_aux_valve_option(std::uint8_t auxValveIndex, bool stateSupported, bool flowSupported);

		/// @brief Returns if a particular aux valve's state control is supported in the TIM client functionality
		/// @param[in] auxValveIndex The index of the aux valve to check
		/// @returns true if the aux valve's state you specified is being reported as "supported".
		bool get_tractor_implement_management_client_aux_valve_state_supported(std::uint8_t auxValveIndex);

		/// @brief Returns if a particular aux valve's flow control is supported in the TIM client functionality
		/// @param[in] auxValveIndex The index of the aux valve to check
		/// @returns true if the aux valve you specified is being reported as "supported".
		bool get_tractor_implement_management_client_aux_valve_flow_supported(std::uint8_t auxValveIndex);

		/// @brief The diagnostic protocol will call this update function, make sure to call DiagnosticProtocol::update() in your update loop
		void update();

	protected:
		/// @brief Populates a vector with the message data needed to send PGN 0xFC8E
		/// @param[in,out] messageData The buffer to populate with data (will be cleared before use)
		void get_message_content(std::vector<std::uint8_t> &messageData);

	private:
		/// @brief Stores the raw byte data associated with a functionality based on
		/// what the user has enabled and what options the user has set for that functionality.
		class FunctionalityData
		{
		public:
			/// @brief Constructor for a FunctionalityData object
			/// @param[in] functionalityToStore The ISO functionality that the object will represent
			explicit FunctionalityData(Functionalities functionalityToStore);

			/// @brief Sets up default data associated to the functionality the object is representing
			/// @details This sets up the serializedValue to be a valid "no options" default set of bytes.
			void configure_default_data();

			/// @brief A helper function to set a particular option bit to some value within an option byte
			/// @note If the parameters are out of range, the data will not be modified
			/// @param[in] byteIndex The options byte index inside which the bit should be set
			/// @param[in] bit The bit offset index of the bit you want to set, between 0 and 7
			/// @param[in] value The new value for the desired bit
			void set_bit_in_option(std::uint8_t byteIndex, std::uint8_t bit, bool value);

			/// @brief A helper function to get a particular bit's value in the specified option byte
			/// @note If the parameters are out of range, this will return zero.
			/// @param[in] byteIndex The options byte index inside which the bit should be retrieved
			/// @param[in] bit The bit offset index of the bit you want to get, between 0 and 7
			/// @returns The state of the requested bit
			bool get_bit_in_option(std::uint8_t byteIndex, std::uint8_t bit);

			Functionalities functionality = Functionalities::MinimumControlFunction; ///< The functionality associated with this data
			std::vector<std::uint8_t> serializedValue; ///< The raw message data value for this functionality
			std::uint8_t generation = 1; ///< The generation of the functionality supported
		};

		/// @brief Enumerates a set of flags representing messages to be transmitted by this interfaces
		enum class TransmitFlags : std::uint32_t
		{
			ControlFunctionFunctionalitiesMessage = 0, ///< A flag to send the CF Functionalities message

			NumberOfFlags ///< The number of flags enumerated in this enum
		};

		/// @brief Checks for the existence of a functionality in the list of previously configured functionalities
		/// and returns an iterator to that functionality in the list
		/// @param[in] functionalityToRetrieve The functionality to return
		/// @returns Iterator to the desired functionality, or supportedFunctionalities.end() if not found
		std::list<FunctionalityData>::iterator get_functionality(Functionalities functionalityToRetrieve);

		/// @brief A wrapper to to get an option from the first byte of a functionalities' data
		/// @param[in] functionality The functionality associated to the option being retrieved
		/// @param[in] byteIndex The index of the option byte to query within
		/// @param[in] option The option bit to get the state for
		/// @returns The state of the bit that was requested
		bool get_functionality_byte_option(Functionalities functionality, std::uint8_t byteIndex, std::uint8_t option);

		/// @brief Returns the byte index of the specified TIM option in the CF Functionalities message data
		/// associated with wither TIM server or TIM client functionalities.
		/// @param[in] option The option for which you want to know the byte index
		/// @returns The byte index of the specified option in the TIM functionalities' message data
		std::uint8_t get_tim_option_byte_index(TractorImplementManagementOptions option) const;

		/// @brief Returns the bit offset of a specified TIM functionality option into the
		/// TIM client and server functionality message data
		/// @param[in] option The option for which you want to know the bit index into the TIM functionality message data
		/// @returns The bit offset/index of the specified option int the TIM functionality message data
		std::uint8_t get_tim_option_bit_index(TractorImplementManagementOptions option) const;

		/// @brief Handles PGN requests for the control function functionalities message
		/// @param[in] parameterGroupNumber The PGN that was requested
		/// @param[in] requestingControlFunction The control function that is requesting the PGN
		/// @param[out] acknowledge Tells the PGN request protocol to ACK ack the request
		/// @param[out] acknowledgeType Tells the PGN request protocol what kind of ACK to use
		/// @param[in] parentPointer A generic context variable, usually the "this" pointer of the registrant for callbacks
		/// @returns true if the PGN was handled, otherwise false
		static bool pgn_request_handler(std::uint32_t parameterGroupNumber,
		                                std::shared_ptr<ControlFunction> requestingControlFunction,
		                                bool &acknowledge,
		                                AcknowledgementType &acknowledgeType,
		                                void *parentPointer);

		/// @brief Processes set transmit flags to send messages
		/// @param[in] flag The flag to process
		/// @param[in] parentPointer A pointer back to an instance of this interface
		static void process_flags(std::uint32_t flag, void *parentPointer);

		static constexpr std::uint8_t NUMBER_TIM_AUX_VALVES_PER_BYTE = 4; ///< The number of aux valves per byte of TIM functionality message data
		static constexpr std::uint8_t NUMBER_TIM_AUX_VALVES = 32; ///< The max number of TIM aux valves

		std::shared_ptr<InternalControlFunction> myControlFunction; ///< The control function to send messages as
		std::list<FunctionalityData> supportedFunctionalities; ///< A list of all configured functionalities and their data
		ProcessingFlags txFlags; ///< Handles retries for sending the CF functionalities message
		Mutex functionalitiesMutex; ///< Since messages come in on a different thread than the main app (probably), this mutex protects the functionality data
	};
} // namespace isobus
#endif // ISOBUS_FUNCTIONALITIES_HPP
