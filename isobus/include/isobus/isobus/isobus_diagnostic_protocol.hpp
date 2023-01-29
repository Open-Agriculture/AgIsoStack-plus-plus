//================================================================================================
/// @file isobus_diagnostic_protocol.hpp
///
/// @brief A protocol that handles the ISO 11783-12 Diagnostic Protocol and some J1939 DMs.
/// @details This protocol manages many of the messages defined in ISO 11783-12
/// and a subset of the messages defined in SAE J1939-73.
/// The ISO-11783 definition of some of these is based on the J1939 definition with some tweaks.
/// You can select if you want the protocol to behave like J1939 by calling set_j1939_mode.
/// One of the messages this protocol supports is the DM1 message.
/// The DM1 is sent via BAM, which has some implications to your application,
/// as only 1 BAM can be active at a time. This message
/// is sent at 1 Hz. In ISOBUS mode, unlike in J1939, the message is discontinued when no DTCs are active to
/// minimize bus load. Also, ISO-11783 does not utilize or support lamp status.
/// Other messages this protocol supports include: DM2, DM3, DM11, DM13, DM22, software ID, and Product ID.
///
/// @note DM13 has two primary functions. It may be used as a command, from either a tool or an
/// ECU, directed to a single controller or to all controllers to request the receiving
/// controller(s) to stop or start broadcast messages. Additionally, it may be used by an ECU
/// to inform other nodes that the sender is about to suspend its normal broadcast due to
/// commands other than a SAE J1939 DM13 command received on that same network segment.
/// The broadcast messages stopped, started, or suspended may be on networks other than SAE J1939.
/// This is not a message to ignore all communications. It is a message to minimize network traffic.
///
/// @attention It is recognized that some network messages may be required to continue even during
/// the "stop broadcast" condition. You MUST handle this in your application, as the stack cannot
/// decide what messages are required without context. In other words, you must opt-in to make
/// your application layer messages adhere to DM13 requests by explicitly calling the functions
/// on this protocol (using get_diagnostic_protocol_by_internal_control_function)
/// to check if you should send it.
///
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef ISOBUS_DIAGNOSTIC_PROTOCOL_HPP
#define ISOBUS_DIAGNOSTIC_PROTOCOL_HPP

#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_protocol.hpp"
#include "isobus/utility/processing_flags.hpp"

#include <list>
#include <memory>
#include <string>

namespace isobus
{
	//================================================================================================
	/// @class DiagnosticProtocol
	/// @brief Manages the DM1, DM2, and DM3 messages for ISO11783 or J1939
	//================================================================================================
	class DiagnosticProtocol : public CANLibProtocol
	{
	public:
		/// @brief Enumerates the different fields in the ECU identification message
		enum class ECUIdentificationFields : std::uint8_t
		{
			PartNumber = 0, ///< The part number of the physical ECU
			SerialNumber, ///< The serial number of the physical ECU
			Location, ///< The location of the ECU within a network
			Type, ///< The type of ECU. One example of a use of the ECU type could be for classifying ECU capabilities, such as I/O.
			ManufacturerName, ///< Manufacturer name string
			HardwareID, ///< ISO 11783 only, This parameter is used to associate the hardware version of an ECU connected to the ISO 11783 network to a conformance test report of that hardware
			NumberOfFields ///< The number of fields currently defined in the ISO standard
		};

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
			ReceivedNetworkDataInError = 19, ///< Any failure that is detected when the data received through the network are found replaced by the �error indicator� value 0xFE
			ConditionExists = 31 ///< The condition that is identified by the SPN exists when no applicable FMI exists (any other error)
		};

		/// @brief A set of transmit flags to manage sending DM1, DM2, and protocol ID
		enum class TransmitFlags
		{
			DM1 = 0, ///< A flag to manage sending the DM1 message
			DM2, ///< A flag to manage sending the DM2 message
			DiagnosticProtocolID, ///< A flag to manage sending the Diagnostic protocol ID message
			ProductIdentification, ///< A flag to manage sending the product identification message
			DM22, ///< Process queued up DM22 responses

			NumberOfFlags ///< The number of flags in the enum
		};

		/// @brief Enumerates the different networks in the DM13
		enum class Network : std::uint8_t
		{
			SAEJ1939Network1PrimaryVehicleNetwork = 0,
			SAEJ1922Network = 1,
			SAEJ1587Network = 2,
			CurrentDataLink = 3,
			OtherManufacturerSpecifiedPort = 4,
			SAEJ1850Network = 5,
			ISO9141Network = 6,
			SAEJ1939Network2 = 7,
			SAEJ1939Network4 = 8,
			ProprietaryNetwork2 = 9,
			ProprietaryNetwork1 = 10,
			SAEJ1939Network3 = 11,
			SAEJ1939Network5 = 25,
			SAEJ1939Network6 = 26,
			SAEJ1939Network7 = 27,
			SAEJ1939Network8 = 28,
			SAEJ1939Network11 = 29,
			SAEJ1939Network10 = 30,
			SAEJ1939Network9 = 31,
			Reserved = 32
		};

		/// @brief Enumerates the commands in the DM13
		enum class StopStartCommand : std::uint8_t
		{
			StopBroadcast = 0, ///< Stop broadcast
			StartBroadcast = 1, ///< Start broadcast
			Reserved = 2, ///< SAE Reserved
			DontCareNoAction = 3 ///< Don’t Care/take no action (leave as is)
		};

		/// @brief Enumerates the different suspend signals for DM13
		enum class SuspendSignalState : std::uint8_t
		{
			IndefiniteSuspension = 0, ///< Indefinite suspension of all broadcasts
			PartialIndefiniteSuspension = 1, ///< Indefinite suspension of some messages
			TemporarySuspension = 2, ///< Temporary suspension of all broadcasts
			PartialTemporarySuspension = 3, ///< Temporary suspension of some messages
			Resuming = 4, ///< Resuming normal broadcast pattern
			NotAvailable = 15 /// < N/A
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
			friend class DiagnosticProtocol; ///< Allow the protocol to have write access the occurance but require other to use getter only
			std::uint8_t occuranceCount; ///< Number of times the DTC has been active (0 to 126 with 127 being not available)
		};

		/// @brief Used to tell the CAN stack that diagnostic messages should be sent from the specified internal control function
		/// @details This will allocate an instance of this protocol
		/// @note Assigning the diagnostic protocol to an ICF will automatically create an instance of the PGN request protocol if needed
		/// as this protocol uses that protocol to abstract away PGN request implementation details. That protocol instance will
		/// only be deleted if you call deassign_diagnostic_protocol_to_internal_control_function and the DP PGNs were the only registered PGNs in
		/// the protocol OR if you manually deassign the PGN request protocol.
		/// Most people will not need to worry about this detail.
		/// @returns `true` If the protocol instance was created OK with the passed in ICF
		static bool assign_diagnostic_protocol_to_internal_control_function(std::shared_ptr<InternalControlFunction> internalControlFunction);

		/// @brief Used to tell the CAN stack that diagnostic messages should no longer be sent from the specified internal control function
		/// @details This will delete an instance of this protocol and may delete an associated but unused instance of the PGN request protocol.
		/// @returns `true` If the protocol instance was deleted OK according to the passed in ICF
		static bool deassign_diagnostic_protocol_to_internal_control_function(std::shared_ptr<InternalControlFunction> internalControlFunction);

		/// @brief Used to tell the CAN stack that diagnostic messages should no longer be sent from any internal control function
		/// @details This will delete all instances of this protocol and may delete associated but unused instances of the PGN request protocol.
		static void deassign_all_diagnostic_protocol_to_internal_control_functions();

		/// @brief Retuns the diagnostic protocol assigned to an internal control function, if any
		/// @param internalControlFunction The internal control function to search against
		/// @returns The protocol object associated to the passed in ICF, or `nullptr` if none found that match the passed in ICF
		static DiagnosticProtocol *get_diagnostic_protocol_by_internal_control_function(std::shared_ptr<InternalControlFunction> internalControlFunction);

		/// @brief Parses out the DM13 J1939 network states from a CAN message
		/// @param[in] message The message to parse from
		/// @param[in] networkStates The returned network state bitfield based on the message contents
		/// @returns `true` if the message was parsed, `false` if the message was invalid
		static bool parse_j1939_network_states(CANMessage *const message, std::uint32_t &networkStates);

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

		/// @brief Clears all previously configured software ID fields set with set_software_id_field
		void clear_software_id_fields();

		/// @brief Returns if broadcasts are suspended for the specified CAN channel (requested by DM13)
		/// @param[in] canChannelIndex The CAN channel to check for suspended broadcasts
		/// @returns `true` if broadcasts should are suspended for the specified channel
		bool get_are_broadcasts_stopped_for_channel(std::uint8_t canChannelIndex) const;

		/// @brief Sets one of the ECU identification strings for the ECU ID message
		/// @details See ECUIdentificationFields for a brief description of the fields
		/// @note The fields in this message are optional and separated by an ASCII �*�. It is not necessary to include parametric
		/// data for all fields. Any additional ECU identification fields defined in the future will be appended at the end.
		/// @attention Do not include the "*" character in your field values
		/// @param[in] field The field to set
		/// @param[in] value The string value associated with the ECU ID field
		void set_ecu_id_field(ECUIdentificationFields field, std::string value);

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
		bool get_diagnostic_trouble_code_active(const DiagnosticTroubleCode &dtc);

		/// @brief Sets the product ID code used in the diagnostic protocol "Product Identification" message (PGN 0xFC8D)
		/// @details The product identification code, as assigned by the manufacturer, corresponds with the number on the
		/// type plate of a product. For vehicles, this number can be the same as the VIN. For stand-alone systems, such as VTs,
		/// this number can be the same as the ECU ID number. The combination of the product identification code and brand shall
		/// make the product globally unique.
		/// @param value The ascii product identification code, up to 50 characters long
		/// @returns true if the value was set, false if the string is too long
		bool set_product_identification_code(std::string value);

		/// @brief Sets the product identification brand used in the diagnostic protocol "Product Identification" message (PGN 0xFC8D)
		/// @details The product identification brand specifies the brand of a product. The combination of the product ID code and brand
		/// shall make the product unique in the world.
		/// @param value The ascii product brand, up to 50 characters long
		/// @returns true if the value was set, false if the string is too long
		bool set_product_identification_brand(std::string value);

		/// @brief Sets the product identification model used in the diagnostic protocol "Product Identification" message (PGN 0xFC8D)
		/// @details The product identification model specifies a unique product within a brand.
		/// @param value The ascii model string, up to 50 characters
		/// @returns true if the value was set, false if the string is too long
		bool set_product_identification_model(std::string value);

		/// @brief Adds an ascii string to this internal control function's software ID
		/// @details Use this to identify the software version of your application.
		/// Seperate fields will be transmitted with a `*` delimeter.
		/// For example, if your main application's version is 1.00, and you have a bootloader
		/// that is version 2.00, you could set field `0` to be "App v1.00" and
		/// field `1` to be "Bootloader v2.00", and it will be transmitted on request as:
		/// "App v1.00*Bootloader v2.00*" in accordance with ISO 11783-12
		/// You can remove a field by setting it to ""
		/// @param[in] index The field index to set
		/// @param[in] value The software ID string to add
		void set_software_id_field(std::uint32_t index, std::string value);

		/// @brief Informs the network that you are going to suspend broadcasts
		/// @param[in] canChannelIndex The CAN channel you will suspend broadcasts on. Will be converted to the proper message `Network` by the stack
		/// @param[in] sourceControlFunction The internal control function to send the DM13 from
		/// @param[in] suspendTime_seconds If you know the time for which broadcasts will be suspended, put it here, otherwise 0xFFFF
		/// @returns `true` if the message was sent, otherwise `false`
		bool suspend_broadcasts(std::uint8_t canChannelIndex, InternalControlFunction *sourceControlFunction, std::uint16_t suspendTime_seconds = 0xFFFF);

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

		/// @brief The DM22 multiplexor bytes. All bytes not given a value here are reserved by SAE
		enum class DM22ControlByte : std::uint8_t
		{
			RequestToClearPreviouslyActiveDTC = 0x01, ///< Clear a previously active DTC
			PositiveAcknowledgeOfPreviouslyActiveDTCClear = 0x02, ///< ACK for clearing a previously active DTC
			NegativeAcknowledgeOfPreviouslyActiveDTCClear = 0x03, ///< NACK for clearing a previously active DTC
			RequestToClearActiveDTC = 0x11, ///< Clear an active DTC
			PositiveAcknowledgeOfActiveDTCClear = 0x12, ///< ACK clearing an active DTC
			NegativeAcknowledgeOfActiveDTCClear = 0x13 ///< NACK clearing an active DTC
		};

		/// @brief The negative acknowledge (NACK) reasons for a DM22 message
		enum class DM22NegativeAcknowledgeIndicator : std::uint8_t
		{
			General = 0x00, ///< General negative acknowledge
			AccessDenied = 0x01, ///< Security denied access
			UnknownOrDoesNotExist = 0x02, ///< The DTC is unknown or does not exist
			DTCUNoLongerPreviouslyActive = 0x03, ///< The DTC in in the active list but it was requested to clear from inactive list
			DTCNoLongerActive = 0x04 ///< DTC is inactive, not active, but active was requested to be cleared
		};

		/// @brief A structure to hold data about DM22 responses we need to send
		struct DM22Data
		{
			ControlFunction *destination; ///< Destination for the DM22 message
			std::uint32_t suspectParameterNumber; ///< SPN of the DTC for the DM22
			std::uint8_t failureModeIdentifier; ///< FMI of the DTC for the DM22
			std::uint8_t nackIndicator; ///< The NACK reason, if applicable
			bool clearActive; ///< true if the DM22 was for an active DTC, false for previously active
			bool nack; ///< true if we are sending a NACK instead of PACK. Determines if we use nackIndicator
		};

		static constexpr std::uint32_t DM_MAX_FREQUENCY_MS = 1000; ///< You are techically allowed to send more than this under limited circumstances, but a hard limit saves 4 RAM bytes per DTC and has BAM benefits
		static constexpr std::uint32_t DM13_HOLD_SIGNAL_TRANSMIT_INTERVAL_MS = 5000; ///< Defined in 5.7.13.13 SPN 1236
		static constexpr std::uint32_t DM13_TIMEOUT_MS = 6000; ///< The timout in 5.7.13 after which nodes shall revert back to the normal broadcast state
		static constexpr std::uint16_t MAX_PAYLOAD_SIZE_BYTES = 1785; ///< DM 1 and 2 are limited to the BAM message max, becuase ETP does not allow global destinations
		static constexpr std::uint8_t DM_PAYLOAD_BYTES_PER_DTC = 4; ///< The number of payload bytes per DTC that gets encoded into the messages
		static constexpr std::uint8_t PRODUCT_IDENTIFICATION_MAX_STRING_LENGTH = 50; ///< The max string length allowed in the fields of product ID, as defined in ISO 11783-12
		static constexpr std::uint8_t DM13_NUMBER_OF_J1939_NETWORKS = 11; ///< The number of networks in DM13 that are set aside for J1939
		static constexpr std::uint8_t DM13_NETWORK_BITMASK = 0x03; ///< Used to mask the network SPN values
		static constexpr std::uint8_t DM13_BITS_PER_NETWORK = 2; ///< Number of bits for the network SPNs

		/// @brief Lists the J1939 networks by index rather than by definition in J1939-73 5.7.13
		static constexpr Network J1939NetworkIndicies[DM13_NUMBER_OF_J1939_NETWORKS] = { Network::SAEJ1939Network1PrimaryVehicleNetwork,
			                                                                               Network::SAEJ1939Network2,
			                                                                               Network::SAEJ1939Network3,
			                                                                               Network::SAEJ1939Network4,
			                                                                               Network::SAEJ1939Network5,
			                                                                               Network::SAEJ1939Network6,
			                                                                               Network::SAEJ1939Network7,
			                                                                               Network::SAEJ1939Network8,
			                                                                               Network::SAEJ1939Network9,
			                                                                               Network::SAEJ1939Network10,
			                                                                               Network::SAEJ1939Network11 };

		/// @brief The constructor for this protocol
		explicit DiagnosticProtocol(std::shared_ptr<InternalControlFunction> internalControlFunction);

		/// @brief The destructor for this protocol
		~DiagnosticProtocol();

		/// @brief A utility function to get the CAN representation of a FlashState
		/// @param flash The flash state to convert
		/// @returns The two bit lamp state for CAN
		std::uint8_t convert_flash_state_to_byte(FlashState flash);

		/// @brief A utility function that will clean up PGN registrations
		void deregister_all_pgns();

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

		/// @brief Sends a DM22 response message
		/// @param data The components of the DM22 response
		/// @returns true if the message was sent
		bool send_diagnostic_message_22_response(DM22Data data);

		/// @brief Sends a message that identifies which diagnostic protocols are supported
		/// @returns true if the message was sent, otherwise false
		bool send_diagnostic_protocol_identification();

		/// @brief Sends the DM13 to alert network devices of impending suspended broadcasts
		/// @returns `true` if the message was sent, otherwise `false`
		bool send_dm13_announce_suspension(InternalControlFunction *sourceControlFunction, std::uint16_t suspendTime_seconds);

		/// @brief Sends the ECU ID message
		/// @returns true if the message was sent
		bool send_ecu_identification();

		/// @brief Sends the product identification message (PGN 0xFC8D)
		/// @returns true if the message was sent, otherwise false
		bool send_product_identification();

		/// @brief Sends the software ID message
		/// @returns true if the message was sent, otherwise false
		bool send_software_identification();

		/// @brief Processes any DM22 responses from the queue
		/// @details We queue responses so that we can do Tx retries if needed
		/// @returns true if queue was completely processed, false if messages remain that could not be sent
		bool process_all_dm22_responses();

		/// @brief A generic way for a protocol to process a received message
		/// @param[in] message A received CAN message
		void process_message(CANMessage *const message) override;

		/// @brief A generic way for a protocol to process a received message
		/// @param[in] message A received CAN message
		/// @param[in] parent Provides the context to the actual TP manager object
		static void process_message(CANMessage *const message, void *parent);

		/// @brief Handles PGN requests for the diagnostic protocol
		/// @param[in] parameterGroupNumber The PGN being requested
		/// @param[in] requestingControlFunction The control function that is requesting the PGN
		/// @param[out] acknowledge Tells the PGN request protocol if it should respond to the request
		/// @param[out] acknowledgementType The type of acknowledgement to send to the requestor
		/// @returns true if any callback was able to handle the PGN request
		bool process_parameter_group_number_request(std::uint32_t parameterGroupNumber,
		                                            ControlFunction *requestingControlFunction,
		                                            bool &acknowledge,
		                                            AcknowledgementType &acknowledgementType);

		// @brief Handles PGN requests for the diagnostic protocol
		/// @param[in] parameterGroupNumber The PGN being requested
		/// @param[in] requestingControlFunction The control function that is requesting the PGN
		/// @param[out] acknowledge Tells the PGN request protocol if it should respond to the request
		/// @param[out] acknowledgementType The type of acknowledgement to send to the requestor
		/// @param[in] parentPointer Generic context variable, usually a pointer to the class that the callback was registed for
		/// @returns true if any callback was able to handle the PGN request
		static bool process_parameter_group_number_request(std::uint32_t parameterGroupNumber,
		                                                   ControlFunction *requestingControlFunction,
		                                                   bool &acknowledge,
		                                                   AcknowledgementType &acknowledgementType,
		                                                   void *parentPointer);

		/// @brief A generic callback for a the class to process flags from the `ProcessingFlags`
		/// @param[in] flag The flag to process
		/// @param[in] parentPointer A generic context pointer to reference a specific instance of this protocol in the callback
		static void process_flags(std::uint32_t flag, void *parentPointer);

		static std::list<DiagnosticProtocol *> diagnosticProtocolList; ///< List of all diagnostic protocol instances (one per ICF)

		std::shared_ptr<InternalControlFunction> myControlFunction; ///< The internal control function that this protocol will send from
		std::vector<DiagnosticTroubleCode> activeDTCList; ///< Keeps track of all the active DTCs
		std::vector<DiagnosticTroubleCode> inactiveDTCList; ///< Keeps track of all the previously active DTCs
		std::vector<DM22Data> dm22ResponseQueue; ///< Maintaining a list of DM22 responses we need to send to allow for retrying in case of Tx failures
		std::vector<std::string> ecuIdentificationFields; ///< Stores the ECU ID fields so we can transmit them when ECUID's PGN is requested
		std::vector<std::string> softwareIdentificationFields; ///< Stores the Software ID fields so we can transmit them when the PGN is requested
		ProcessingFlags txFlags; ///< An instance of the processing flags to handle retries of some messages
		std::string productIdentificationCode; ///< The product identification code for sending the product identification message
		std::string productIdentificationBrand; ///< The product identification brand for sending the product identification message
		std::string productIdentificationModel; ///< The product identification model name for sending the product identification message
		std::uint32_t lastDM1SentTimestamp; ///< A timestamp in milliseconds of the last time a DM1 was sent
		std::uint32_t stopBroadcastNetworkBitfield; ///< Bitfield for tracking the network broadcast states for DM13
		std::uint32_t lastDM13ReceivedTimestamp; ///< A timestamp in milliseconds when we last got a DM13 message
		bool j1939Mode; ///< Tells the protocol to operate according to J1939 instead of ISO11783
	};
}

#endif // ISOBUS_DIAGNOSTIC_PROTOCOL_HPP
