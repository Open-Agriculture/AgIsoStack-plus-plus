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
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef ISOBUS_FUNCTIONALITIES_HPP
#define ISOBUS_FUNCTIONALITIES_HPP

#include "isobus/isobus/can_internal_control_function.hpp"

#include <list>
#include <mutex>
#include <vector>

namespace isobus
{
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
		// an implement working set auxiliary function or an auxiliary function input unit.
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
			NoOptions,
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

		/// @brief Adds or removes a supported functionality
		/// @param[in] functionality The functionality to change
		/// @param[in] functionalityGeneration The generation of the functionality to report
		/// @param[in] isSupported If true, this class will add reporting as such on the ISOBUS. If false, it will not be reported on the bus.
		/// @note Minimum Control Function is enabled by defualt, and generally should not be disabled.
		void set_functionality_is_supported(Functionalities functionality, std::uint8_t functionalityGeneration, bool isSupported);

		bool get_functionality_is_supported(Functionalities functionality);

		std::uint8_t get_functionality_generation(Functionalities functionality);

		void set_minimum_control_function_option_state(MinimumControlFunctionOptions option, bool optionState);

		void set_aux_O_inputs_option_state(AuxOOptions option, bool optionState);
		void set_aux_O_functions_option_state(AuxOOptions option, bool optionState);
		void set_aux_N_inputs_option_state(AuxNOptions option, bool optionState);
		void set_aux_N_functions_option_state(AuxNOptions option, bool optionState);
		void set_task_controller_geo_server_option_state(TaskControllerGeoServerOptions option, bool optionState);
		void set_task_controller_geo_client_option(std::uint8_t numberOfControlChannels);
		void set_task_controller_section_control_server_option_state(std::uint8_t numberOfSupportedBooms, std::uint8_t numberOfSupportedSections, bool optionState);
		void set_task_controller_section_control_client_option_state(std::uint8_t numberOfSupportedBooms, std::uint8_t numberOfSupportedSections, bool optionState);
		void set_basic_tractor_ECU_server_option_state(BasicTractorECUOptions option, bool optionState);
		void set_basic_tractor_ECU_implement_client_option_state(BasicTractorECUOptions option, bool optionState);
		void set_tractor_implement_management_server_option_state(TractorImplementManagementOptions option, bool optionState);
		void set_tractor_implement_management_server_aux_valve_option(std::uint8_t auxValveIndex, bool stateSupported, bool flowSupported);
		void set_tractor_implement_management_client_option_state(TractorImplementManagementOptions option, bool optionState);
		void set_tractor_implement_management_client_aux_valve_option(std::uint8_t auxValveIndex, bool stateSupported, bool flowSupported);

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

			Functionalities functionality = Functionalities::MinimumControlFunction; ///< The funcionalty associated with this data
			std::vector<std::uint8_t> serializedValue; ///< The raw message data value for this functionality
			std::uint8_t generation = 1; ///< The generation of the functionality supported
		};

		/// @brief Checks for the existance of a functionality in the list of previously configured functionalities
		/// and returns an iterator to that functionality in the list
		/// @param[in] functionalityToRetreive The functionality to return
		/// @returns Iterator to the desired functionality, or supportedFunctionalities.end() if not found
		std::list<FunctionalityData>::iterator get_functionality(Functionalities functionalityToRetreive);

		/// @brief Populates a vector with the message data needed to send PGN 0xFC8E
		/// @param[in,out] messageData The buffer to populate with data (will be cleared before use)
		void get_message_content(std::vector<std::uint8_t> &messageData);

		std::uint8_t get_tim_option_byte_index(TractorImplementManagementOptions option) const;

		std::uint8_t get_tim_option_bit_index(TractorImplementManagementOptions option) const;

		static constexpr std::uint8_t NUMBER_TIM_AUX_VALVES_PER_BYTE = 4;

		std::shared_ptr<InternalControlFunction> myControlFunction; ///< The control function to send messages as
		std::list<FunctionalityData> supportedFunctionalities; ///< A list of all configured functionalities and their data
		std::mutex functionalitiesMutex; ///< Since messages come in on a different thread than the main app (probably), this mutex protects the functionality data
	};
} // namespace isobus
#endif // ISOBUS_FUNCTIONALITIES_HPP
