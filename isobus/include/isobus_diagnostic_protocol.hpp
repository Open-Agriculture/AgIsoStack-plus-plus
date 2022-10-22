//================================================================================================
/// @file isobus_diagnostic_protocol.hpp
///
/// @brief A protocol that handles the ISO-11783 Active DTC Protocol.
/// @details The ISO-11783 definition of DM1 is based on the J1939 definition with some tweaks.
/// This protocol reports active diagnostic trouble codes as defined by
/// SAE J1939-73. The message this protocol sends is sent via BAM, which has some
/// implications to your application, as only 1 BAM can be active at a time. This message
/// is sent at 1 Hz. Unlike in J1939, the message is discontinued when no DTCs are active to
/// minimize bus load. Also, ISO-11783 does not utilize or support lamp status.
/// You can revert to the standard J1939 behavior though if you want.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef ISOBUS_DIAGNOSTIC_PROTOCOL_HPP
#define ISOBUS_DIAGNOSTIC_PROTOCOL_HPP

#include "can_internal_control_function.hpp"
#include "can_protocol.hpp"
#include "processing_flags.hpp"

#include <list>
#include <memory>

namespace isobus
{
	//================================================================================================
	/// @class DiagnosticProtocol
	/// @brief Manages the DM1, DM2, and DM3 messages for ISO11783 or J1939
	//================================================================================================
	class DiagnosticProtocol : public CANLibProtocol
	{
	public:
		/// @brief The DTC lamp status as defined in J1939-73. Not used when in ISO11783 mode
		enum class LampStatus
		{
			None,
			MalfunctionIndicatorLampSolid, ///< A lamp used to relay only emissions-related trouble code information
			MalfuctionIndicatorLampSlowFlash, ///< A lamp used to relay only emissions-related trouble code information
			MalfunctionIndicatorLampFastFlash, ///< A lamp used to relay only emissions-related trouble code information
			RedStopLampSolid, ///< This lamp is used to relay trouble code information that is of a severe-enough condition that it warrants stopping the vehicle
			RedStopLampSlowFlash, ///< This lamp is used to relay trouble code information that is of a severe-enough condition that it warrants stopping the vehicle
			RedStopLampFastFlash, ///< This lamp is used to relay trouble code information that is of a severe-enough condition that it warrants stopping the vehicle
			AmberWarningLampSolid, ///< This lamp is used to relay trouble code information that is reporting a problem with the vehicle system but the vehicle need not be immediately stopped.
			AmberWarningLampSlowFlash, ///< This lamp is used to relay trouble code information that is reporting a problem with the vehicle system but the vehicle need not be immediately stopped.
			AmberWarningLampFastFlash, ///< This lamp is used to relay trouble code information that is reporting a problem with the vehicle system but the vehicle need not be immediately stopped.
			EngineProtectLampSolid, ///< This lamp is used to relay trouble code information that is reporting a problem with a vehicle system that is most probably not electronic sub-system related
			EngineProtectLampSlowFlash, ///< This lamp is used to relay trouble code information that is reporting a problem with a vehicle system that is most probably not electronic sub-system related
			EngineProtectLampFastFlash ///< This lamp is used to relay trouble code information that is reporting a problem with a vehicle system that is most probably not electronic sub-system related
		};

		/// @brief FMI as defined in ISO11783-12 Annex E
		enum class FailureModeIdentifier
		{
			DataValidAboveNormalMostSevere = 0, ///< Condition is above normal as determined by the predefined most severe level limits for that particular measure of the condition
			DataValidBelowNormalMostSevere = 1, ///< Condition is below normal as determined by the predefined most severe level limits for that particular measure of the condition
			DataErratic = 2, ///< Erratic or intermittent data include all measurements that change at a rate not considered possible in real - world conditions
			VoltageAboveNormal = 3, ///< A voltage signal, data or otherwise, is above the predefined limits that bound the range
			VoltageBelowNormal = 4, ///< A voltage signal, data or otherwise, is below the predefined limits that bound the range
			CurrentBelowNormal = 5, ///< A current signal, data or otherwise, is below the predefined limits that bound the range
			CurrentAboveNormal = 6, ///< A current signal, data or otherwise, is above the predefined limits that bound the range
			MechanicalSystemNotResponding = 7, ///< Any fault that is detected as the result of an improper mechanical adjustment, an improper response or action of a mechanical system
			AbnormalFrequency = 8, ///< Any frequency or PWM signal that is outside the predefined limits which bound the signal range for frequency or duty cycle
			AbnotmalUpdateRate = 9, ///< Any failure that is detected when receipt of data through the data network is not at the update rate expected or required
			AbnormalRateOfChange = 10, ///< Any data, exclusive of FMI 2, that are considered valid but which are changing at a rate that is outside the predefined limits that bound the rate of change for the system
			RootCauseNotKnown = 11, ///< It has been detected that a failure has occurred in a particular subsystem but the exact nature of the fault is not known
			BadIntellegentDevice = 12, ///< Internal diagnostic procedures have determined that the failure is one which requires the replacement of the ECU
			OutOfCalibration = 13, ///< A failure that can be identified as the result of improper calibration
			SpecialInstructions = 14, ///< Used when the on-board system can isolate the failure to a small number of choices but not to a single point of failure. See 11783-12 Annex E
			DataValidAboveNormalLeastSevere = 15, ///< Condition is above what would be considered normal as determined by the predefined least severe level limits for that particular measure of the condition
			DataValidAboveNormalModeratelySevere = 16, ///< Condition is above what would be considered normal as determined by the predefined moderately severe level limits for that particular measure of the condition
			DataValidBelowNormalLeastSevere = 17, ///< Condition is below what would be considered normal as determined by the predefined least severe level limits for that particular measure of the condition
			DataValidBelowNormalModeratelySevere = 18, ///< Condition is below what would be considered normal as determined by the predefined moderately severe level limits for that particular measure of the condition
			ReceivedNetworkDataInError = 19, ///< Any failure that is detected when the data received through the network are found replaced by the “error indicator” value 0xFE
			ConditionExists = 31 ///< The condition that is identified by the SPN exists when no applicable FMI exists (any other error)
		};

		/// @brief A set of transmit flags to manage sending DM1, DM2, and protocol ID
		enum class TransmitFlags
		{
			DM1 = 0, ///< A flag to manage sending the DM1 message
			DM2, ///< A flag to manage sending the DM2 message
			DiagnosticProtocolID, ///< A flag to manage sending the Diagnostic protocol ID message

			NumberOfFlags ///< The number of flags in the enum
		};

		//================================================================================================
		/// @class DiagnosticTroubleCode
		/// @brief A storage class for describing a complete DTC
		//================================================================================================
		class DiagnosticTroubleCode
		{
		public:
			/// @brief Constructor for a DTC, sets default values at construction time
			/// @param[in] internalControlFunction The internal control function to use for sending messages
			DiagnosticTroubleCode();

			/// @brief Constructor for a DTC, sets all values explicitly
			/// @param[in] spn The suspect parameter number
			/// @param[in] fmi The failure mode indicator
			/// @param[in] lamp The J1939 lamp status. Set to `None` if you don't care about J1939
			DiagnosticTroubleCode(std::uint32_t spn, FailureModeIdentifier fmi, LampStatus lamp);

			/// @brief A useful way to compare DTC objects to each other for equality
			/// @param[in] obj The "rhs" of the comparison
			/// @returns `true` if the objects were equal
			bool operator==(const DiagnosticTroubleCode &obj);

			///  @brief Returns the occurance count, which will be kept track of by the protocol
			std::uint8_t get_occurrance_count() const;

			std::uint32_t suspectParameterNumber; ///< This 19-bit number is used to identify the item for which diagnostics are being reported
			std::uint8_t failureModeIdentifier; ///< The FMI defines the type of failure detected in the sub-system identified by an SPN
			LampStatus lampState; ///< The J1939 lamp state for this DTC
		private:
			friend class DiagnosticProtocol;
			std::uint8_t occuranceCount; ///< Number of times the DTC has been active (0 to 126 with 127 being not available)
		};

		/// @brief Used to tell the CAN stack that diagnostic messages should be sent from the specified internal control function
		/// @details This will allocate an instance of this protocol
		/// @returns `true` If the protocol instance was created OK with the passed in ICF
		static bool assign_diagnostic_protocol_to_internal_control_function(std::shared_ptr<InternalControlFunction> internalControlFunction);

		/// @brief Used to tell the CAN stack that diagnostic messages should no longer be sent from the specified internal control function
		/// @details This will delete an instance of this protocol
		/// @returns `true` If the protocol instance was deleted OK according to the passed in ICF
		static bool deassign_diagnostic_protocol_to_internal_control_function(std::shared_ptr<InternalControlFunction> internalControlFunction);

		/// @brief Retuns the diagnostic protocol assigned to an internal control function, if any
		/// @param internalControlFunction The internal control function to search against
		/// @returns The protocol object associated to the passed in ICF, or `nullptr` if none found that match the passed in ICF
		static DiagnosticProtocol *get_diagnostic_protocol_by_internal_control_function(std::shared_ptr<InternalControlFunction> internalControlFunction);

		/// @brief The protocol's initializer function
		void initialize(CANLibBadge<CANNetworkManager>) override;

		/// @brief Enables the protocol to run in J1939 mode instead of ISO11783 mode
		/// @details See ISO11783-12 and J1939-73 for a complete explanation of the differences
		/// @param[in] value The desired mode. `true` for J1939 mode, `false` for ISO11783 mode
		void set_j1939_mode(bool value);

		/// @brief Returns `true` if the protocol is in J1939 mode instead of ISO11783 mode, `false` if using ISO11783 mode
		/// @returns `true` if the protocol is in J1939 mode instead of ISO11783 mode, `false` if using ISO11783 mode
		bool get_j1939_mode() const;

		/// @brief Clears the list of active DTCs and makes them all inactive
		void clear_active_diagnostic_trouble_codes();

		/// @brief Clears the list of inactive DTCs and clears occurance counts
		void clear_inactive_diagnostic_trouble_codes();

		/// @brief Adds a DTC to the active list, or removes one from the active list
		/// @details When you call this function with a DTC and `true`, it will be added to the DM1 message.
		/// When you call it with a DTC and `false` it will be moved to the inactive list.
		/// If you get `false` as a return value, either the DTC was already in the target state or the data was not valid
		/// @param[in] dtc A diagnostic trouble code whose state should be altered
		/// @param[in] active Sets if the DTC is currently active or not
		/// @returns True if the DTC was added/removed from the list, false if DTC was not valid or target state is invalid
		bool set_diagnostic_trouble_code_active(const DiagnosticTroubleCode &dtc, bool active);

		/// @brief Returns if a DTC is active
		/// @param[in] dtc A diagnostic trouble code whose state should be altered
		/// @returns `true` if the DTC was in the active list
		bool get_diagnostic_trouble_code_active(const DiagnosticTroubleCode &dtc) const;

		/// @brief Updates the protocol cyclically
		void update(CANLibBadge<CANNetworkManager>) override;

	private:
		/// @brief Lists the different lamps in J1939-73
		enum class Lamps
		{
			MalfunctionIndicatorLamp, ///< The "MIL"
			RedStopLamp, ///< The "RSL"
			AmberWarningLamp, ///< The "AWL"
			ProtectLamp ///< The engine protect lamp
		};

		/// @brief Enumerates lamp flash states in J1939
		enum class FlashState
		{
			Solid, ///< Solid / no flash
			Slow, ///< Slow flash
			Fast ///< Fast flash
		};

		static constexpr std::uint32_t DM_MAX_FREQUENCY_MS = 1000; ///< You are techically allowed to send more than this under limited circumstances, but a hard limit saves 4 RAM bytes per DTC and has BAM benefits
		static constexpr std::uint8_t DM_PAYLOAD_BYTES_PER_DTC = 4; ///< The number of payload bytes per DTC that gets encoded into the messages

		/// @brief The constructor for this protocol
		explicit DiagnosticProtocol(std::shared_ptr<InternalControlFunction> internalControlFunction);

		/// @brief The destructor for this protocol
		~DiagnosticProtocol();

		/// @brief A utility function to get the CAN representation of a FlashState
		/// @param flash The flash state to convert
		/// @returns The two bit lamp state for CAN
		std::uint8_t convert_flash_state_to_byte(FlashState flash);

		/// @brief This is a way to find the overall lamp states to report
		/// @details This searches the active DTC list to find if a lamp is on or off, and to find the overall flash state for that lamp.
		/// Basically, since the lamp states are global to the CAN message, we need a way to resolve the "total" lamp state from the list.
		/// @param[in] targetLamp The lamp to find the status of
		/// @param[out] flash How the lamp should be flashing
		/// @param[out] lampOn If the lamp state is on for any DTC
		void get_active_list_lamp_state_and_flash_state(Lamps targetLamp, FlashState &flash, bool &lampOn);

		/// @brief This is a way to find the overall lamp states to report
		/// @details This searches the inactive DTC list to find if a lamp is on or off, and to find the overall flash state for that lamp.
		/// Basically, since the lamp states are global to the CAN message, we need a way to resolve the "total" lamp state from the list.
		/// @param[in] targetLamp The lamp to find the status of
		/// @param[out] flash How the lamp should be flashing
		/// @param[out] lampOn If the lamp state is on for any DTC
		void get_inactive_list_lamp_state_and_flash_state(Lamps targetLamp, FlashState &flash, bool &lampOn);

		/// @brief The network manager calls this to see if the protocol can accept a non-raw CAN message for processing
		/// @note In this protocol, we do not accept messages from the network manager for transmission
		/// @param[in] parameterGroupNumber The PGN of the message
		/// @param[in] data The data to be sent
		/// @param[in] messageLength The length of the data to be sent
		/// @param[in] source The source control function
		/// @param[in] destination The destination control function
		/// @param[in] transmitCompleteCallback A callback for when the protocol completes its work
		/// @param[in] parentPointer A generic context object for the tx complete and chunk callbacks
		/// @param[in] frameChunkCallback A callback to get some data to send
		/// @returns true if the message was accepted by the protocol for processing
		bool protocol_transmit_message(std::uint32_t parameterGroupNumber,
		                               const std::uint8_t *data,
		                               std::uint32_t messageLength,
		                               ControlFunction *source,
		                               ControlFunction *destination,
		                               TransmitCompleteCallback transmitCompleteCallback,
		                               void *parentPointer,
		                               DataChunkCallback frameChunkCallback) override;

		/// @brief Sends a DM1 encoded CAN message
		/// @returns true if the message was sent, otherwise false
		bool send_diagnostic_message_1();

		/// @brief Sends a DM2 encoded CAN message
		/// @returns true if the message was sent, otherwise false
		bool send_diagnostic_message_2();

		/// @brief Sends an ACK (pgn E800) for clearing inactive DTCs via DM3
		/// @todo Replace manual ACK with a PGN request protocol to simplify ACK/NACK
		/// @param destination The destination control function for the ACK
		/// @returns true if te message was sent, otherwise false
		bool send_diagnostic_message_3_ack(ControlFunction *destination);

		/// @brief Sends a message that identifies which diagnostic protocols are supported
		/// @returns true if the message was sent, otherwise false
		bool send_diagnostic_protocol_identification();

		/// @brief A generic way for a protocol to process a received message
		/// @param[in] message A received CAN message
		void process_message(CANMessage *const message) override;

		/// @brief A generic way for a protocol to process a received message
		/// @param[in] message A received CAN message
		/// @param[in] parent Provides the context to the actual TP manager object
		static void process_message(CANMessage *const message, void *parent);

		/// @brief A generic callback for a the class to process flags from the `ProcessingFlags`
		/// @param[in] flag The flag to process
		/// @param[in] parentPointer A generic context pointer to reference a specific instance of this protocol in the callback
		static void process_flags(std::uint32_t flag, void *parentPointer);

		static std::list<DiagnosticProtocol *> diagnosticProtocolList; ///< List of all diagnostic protocol instances (one per ICF)

		std::shared_ptr<InternalControlFunction> myControlFunction; ///< The internal control function that this protocol will send from
		std::vector<DiagnosticTroubleCode> activeDTCList; ///< Keeps track of all the active DTCs
		std::vector<DiagnosticTroubleCode> inactiveDTCList; ///< Keeps track of all the previously active DTCs
		ProcessingFlags txFlags; ///< An instance of the processing flags to handle retries of some messages
		std::uint32_t lastDM1SentTimestamp; ///< A timestamp in milliseconds of the last time a DM1 was sent
		std::uint32_t lastDM2SentTimestamp; ///< A timestamp in milliseconds of the last time a DM2 was sent
		bool j1939Mode; ///< Tells the protocol to operate according to J1939 instead of ISO11783
	};
}

#endif // ISOBUS_DIAGNOSTIC_PROTOCOL_HPP
