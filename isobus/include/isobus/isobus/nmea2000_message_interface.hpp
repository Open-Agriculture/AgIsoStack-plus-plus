//================================================================================================
/// @file nmea2000_message_interface.hpp
///
/// @brief A message interface for processing or sending NMEA2K messages commonly used on
/// an ISO 11783 network.
///
/// @details This interface provides a common interface for sending and receiving common
/// NMEA2000 messages that might be found on an ISO 11783 network. ISO11783-7 defines
/// that GNSS information be sent using NMEA2000 parameter groups like the ones included
/// in this interface.
///
/// @note This library and its authors are not affiliated with the National Marine
/// Electronics Association in any way.
///
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#ifndef NMEA2000_MESSAGE_INTERFACE_HPP
#define NMEA2000_MESSAGE_INTERFACE_HPP

#include "isobus/isobus/nmea2000_message_definitions.hpp"
#include "isobus/utility/event_dispatcher.hpp"
#include "isobus/utility/processing_flags.hpp"

namespace isobus
{
	/// @brief An interface for sending and receiving common NMEA2000 messages on an ISO11783 network
	class NMEA2000MessageInterface
	{
	public:
		/// @brief Constructor for a NMEA2000MessageInterface
		/// @param[in] sendingControlFunction The control function the interface should use to send messages, or nullptr optionally
		/// @param[in] enableSendingCogSogCyclically Set to true for the interface to attempt to send the COG & SOG message cyclically
		/// @param[in] enableSendingDatumCyclically Set to true for the interface to attempt to send the Datum message cyclically
		/// @param[in] enableSendingGNSSPositionDataCyclically Set to true for the interface to attempt to send the GNSS position data message message cyclically
		/// @param[in] enableSendingPositionDeltaHighPrecisionRapidUpdateCyclically Set to true for the interface to attempt to send the position delta message cyclically
		/// @param[in] enableSendingPositionRapidUpdateCyclically Set to true for the interface to attempt to send the position rapid update message cyclically
		/// @param[in] enableSendingRateOfTurnCyclically Set to true for the interface to attempt to send the rate of turn message cyclically
		/// @param[in] enableSendingVesselHeadingCyclically Set to true for the interface to attempt to send the vessel heading message cyclically
		NMEA2000MessageInterface(std::shared_ptr<InternalControlFunction> sendingControlFunction,
		                         bool enableSendingCogSogCyclically,
		                         bool enableSendingDatumCyclically,
		                         bool enableSendingGNSSPositionDataCyclically,
		                         bool enableSendingPositionDeltaHighPrecisionRapidUpdateCyclically,
		                         bool enableSendingPositionRapidUpdateCyclically,
		                         bool enableSendingRateOfTurnCyclically,
		                         bool enableSendingVesselHeadingCyclically);

		/// @brief Destructor for a NMEA2000MessageInterface
		~NMEA2000MessageInterface();

		/// @brief Returns a CourseOverGroundSpeedOverGroundRapidUpdate object that you can use to
		/// set the message's individual signal values, which will then be transmitted if the interface is configured to do so.
		/// @returns CourseOverGroundSpeedOverGroundRapidUpdate used to set the individual signal values sent in the COG & SOG message
		NMEA2000Messages::CourseOverGroundSpeedOverGroundRapidUpdate &get_cog_sog_transmit_message();

		/// @brief Returns a Datum object that you can use to set the message's individual signal values,
		/// which will then be transmitted if the interface is configured to do so.
		/// @returns Datum used to set the individual signal values sent in the Datum message
		NMEA2000Messages::Datum &get_datum_transmit_message();

		/// @brief Returns a GNSSPositionData object that you can use to
		/// set the message's individual signal values, which will then be transmitted if the interface is configured to do so.
		/// @returns GNSSPositionData used to set the individual signal values sent in the GNSS position data message
		NMEA2000Messages::GNSSPositionData &get_gnss_position_data_transmit_message();

		/// @brief Returns a PositionDeltaHighPrecisionRapidUpdate object that you can use to
		/// set the message's individual signal values, which will then be transmitted if the interface is configured to do so.
		/// @returns PositionDeltaHighPrecisionRapidUpdate used to set the individual signal values sent in the position delta message
		NMEA2000Messages::PositionDeltaHighPrecisionRapidUpdate &get_position_delta_high_precision_rapid_update_transmit_message();

		/// @brief Returns a PositionRapidUpdate object that you can use to
		/// set the message's individual signal values, which will then be transmitted if the interface is configured to do so.
		/// @returns PositionRapidUpdate used to set the individual signal values sent in the position rapid update message
		NMEA2000Messages::PositionRapidUpdate &get_position_rapid_update_transmit_message();

		/// @brief Returns a RateOfTurn object that you can use to
		/// set the message's individual signal values, which will then be transmitted if the interface is configured to do so.
		/// @returns RateOfTurn used to set the individual signal values sent in the rate of turn message
		NMEA2000Messages::RateOfTurn &get_rate_of_turn_transmit_message();

		/// @brief Returns a VesselHeading object that you can use to
		/// set the message's individual signal values, which will then be transmitted if the interface is configured to do so.
		/// @returns VesselHeading used to set the individual signal values sent in the vessel heading message
		NMEA2000Messages::VesselHeading &get_vessel_heading_transmit_message();

		/// @brief Returns the number of unique senders of the COG & SOG message
		/// @returns The number of unique COG & SOG message senders
		std::size_t get_number_received_course_speed_over_ground_message_sources() const;

		/// @brief Returns the number of unique datum message senders
		/// @returns The number of unique datum message senders
		std::size_t get_number_received_datum_message_sources() const;

		/// @brief Returns the number of unique GNSS position data message senders
		/// @returns The number of GNSS position data message senders
		std::size_t get_number_received_gnss_position_data_message_sources() const;

		/// @brief Returns the number of unique delta position message senders
		/// @returns The number of unique position delta high precision rapid update message senders
		std::size_t get_number_received_position_delta_high_precision_rapid_update_message_sources() const;

		/// @brief Returns the number of unique position rapid update message senders
		/// @returns The number of unique position rapid update message senders
		std::size_t get_number_received_position_rapid_update_message_sources() const;

		/// @brief Returns the number of unique rate of turn message senders
		/// @returns The number of unique rate of turn message senders
		std::size_t get_number_received_rate_of_turn_message_sources() const;

		/// @brief Returns the number of unique vessel heading message senders
		/// @returns The number of unique vessel heading message senders
		std::size_t get_number_received_vessel_heading_message_sources() const;

		/// @brief Returns the content of the COG & SOG message
		/// based on the index of the sender. Use this to read the received messages' content.
		/// @param[in] index An index of senders of the the message
		/// @note Only one device on the bus will send this normally, but we provide a generic way to get
		/// an arbitrary number of these. So generally using only index 0 will be acceptable.
		/// @note It is also possible that this message may not be present, depending on your machine.
		/// @returns The content of the COG & SOG message
		std::shared_ptr<NMEA2000Messages::CourseOverGroundSpeedOverGroundRapidUpdate> get_received_course_speed_over_ground_message(std::size_t index) const;

		/// @brief Returns the content of the Datum message
		/// based on the index of the sender. Use this to read the received messages' content.
		/// @param[in] index An index of senders of the the message
		/// @note Only one device on the bus will send this normally, but we provide a generic way to get
		/// an arbitrary number of these. So generally using only index 0 will be acceptable.
		/// @note It is also possible that this message may not be present, depending on your machine.
		/// @returns The content of the Datum message
		std::shared_ptr<NMEA2000Messages::Datum> get_received_datum_message(std::size_t index) const;

		/// @brief Returns the content of the GNSS position data message
		/// based on the index of the sender. Use this to read the received messages' content.
		/// @param[in] index An index of senders of the the message
		/// @note Only one device on the bus will send this normally, but we provide a generic way to get
		/// an arbitrary number of these. So generally using only index 0 will be acceptable.
		/// @note It is also possible that this message may not be present, depending on your machine.
		/// @returns The content of the GNSS position data message
		std::shared_ptr<NMEA2000Messages::GNSSPositionData> get_received_gnss_position_data_message(std::size_t index) const;

		/// @brief Returns the content of the position delta high precision rapid update message
		/// based on the index of the sender. Use this to read the received messages' content.
		/// @param[in] index An index of senders of the the message
		/// @note Only one device on the bus will send this normally, but we provide a generic way to get
		/// an arbitrary number of these. So generally using only index 0 will be acceptable.
		/// @note It is also possible that this message may not be present, depending on your machine.
		/// @returns The content of the position delta high precision rapid update message
		std::shared_ptr<NMEA2000Messages::PositionDeltaHighPrecisionRapidUpdate> get_received_position_delta_high_precision_rapid_update_message(std::size_t index) const;

		/// @brief Returns the content of the position rapid update message
		/// based on the index of the sender. Use this to read the received messages' content.
		/// @param[in] index An index of senders of the the message
		/// @note Only one device on the bus will send this normally, but we provide a generic way to get
		/// an arbitrary number of these. So generally using only index 0 will be acceptable.
		/// @note It is also possible that this message may not be present, depending on your machine.
		/// @returns The content of the position rapid update message
		std::shared_ptr<NMEA2000Messages::PositionRapidUpdate> get_received_position_rapid_update_message(std::size_t index) const;

		/// @brief Returns the content of the rate of turn message
		/// based on the index of the sender. Use this to read the received messages' content.
		/// @param[in] index An index of senders of the the message
		/// @note Only one device on the bus will send this normally, but we provide a generic way to get
		/// an arbitrary number of these. So generally using only index 0 will be acceptable.
		/// @note It is also possible that this message may not be present, depending on your machine.
		/// @returns The content of the rate of turn message
		std::shared_ptr<NMEA2000Messages::RateOfTurn> get_received_rate_of_turn_message(std::size_t index) const;

		/// @brief Returns the content of the vessel heading message
		/// based on the index of the sender. Use this to read the received messages' content.
		/// @param[in] index An index of senders of the the message
		/// @note Only one device on the bus will send this normally, but we provide a generic way to get
		/// an arbitrary number of these. So generally using only index 0 will be acceptable.
		/// @note It is also possible that this message may not be present, depending on your machine.
		/// @returns The content of the vessel heading message
		std::shared_ptr<NMEA2000Messages::VesselHeading> get_received_vessel_heading_message(std::size_t index) const;

		/// @brief Returns an event dispatcher which you can use to get callbacks when new/updated COG & SOG messages are received.
		/// @returns The event publisher for COG & SOG messages
		EventDispatcher<const std::shared_ptr<NMEA2000Messages::CourseOverGroundSpeedOverGroundRapidUpdate>, bool> &get_course_speed_over_ground_rapid_update_event_publisher();

		/// @brief Returns an event dispatcher which you can use to get callbacks when new/updated datum messages are received.
		/// @returns The event publisher for datum messages
		EventDispatcher<const std::shared_ptr<NMEA2000Messages::Datum>, bool> &get_datum_event_publisher();

		/// @brief Returns an event dispatcher which you can use to get callbacks when new/updated GNSS position data messages are received.
		/// @returns The event publisher for GNSS position data messages
		EventDispatcher<const std::shared_ptr<NMEA2000Messages::GNSSPositionData>, bool> &get_gnss_position_data_event_publisher();

		/// @brief Returns an event dispatcher which you can use to get callbacks when new/updated position delta high precision rapid update messages are received.
		/// @returns The event publisher for position delta high precision rapid update messages
		EventDispatcher<const std::shared_ptr<NMEA2000Messages::PositionDeltaHighPrecisionRapidUpdate>, bool> &get_position_delta_high_precision_rapid_update_event_publisher();

		/// @brief Returns an event dispatcher which you can use to get callbacks when new/updated position rapid update messages are received.
		/// @returns The event publisher for position rapid update messages
		EventDispatcher<const std::shared_ptr<NMEA2000Messages::PositionRapidUpdate>, bool> &get_position_rapid_update_event_publisher();

		/// @brief Returns an event dispatcher which you can use to get callbacks when new/updated rate of turn messages are received.
		/// @returns The event publisher for rate of turn messages
		EventDispatcher<const std::shared_ptr<NMEA2000Messages::RateOfTurn>, bool> &get_rate_of_turn_event_publisher();

		/// @brief Returns an event dispatcher which you can use to get callbacks when new/updated vessel heading messages are received.
		/// @returns The event publisher for vessel heading messages
		EventDispatcher<const std::shared_ptr<NMEA2000Messages::VesselHeading>, bool> &get_vessel_heading_event_publisher();

		/// @brief Returns if the interface has cyclic sending of the course/speed over ground message enabled
		/// @returns True if the interface has cyclic sending of the course/speed over ground message enabled, otherwise false
		bool get_enable_sending_cog_sog_cyclically() const;

		/// @brief Instructs the interface to enable or disable sending the course/speed over ground message cyclically
		/// @param[in] enable Set to true to have the interface cyclically send the course/speed over ground message cyclically
		void set_enable_sending_cog_sog_cyclically(bool enable);

		/// @brief Returns if the interface has cyclic sending of the datum message enabled
		/// @returns True if the interface has cyclic sending of the datum message enabled, otherwise false
		bool get_enable_sending_datum_cyclically() const;

		/// @brief Instructs the interface to enable or disable sending the datum data message cyclically
		/// @param[in] enable Set to true to have the interface cyclically send the datum message cyclically
		void set_enable_sending_datum_cyclically(bool enable);

		/// @brief Returns if the interface has cyclic sending of the GNSS position data message enabled
		/// @returns True if the interface has cyclic sending of the GNSS position data message enabled, otherwise false
		bool get_enable_sending_gnss_position_data_cyclically() const;

		/// @brief Instructs the interface to enable or disable sending the GNSS position data message cyclically
		/// @param[in] enable Set to true to have the interface cyclically send the GNSS position data message cyclically
		void set_enable_sending_gnss_position_data_cyclically(bool enable);

		/// @brief Returns if the interface has cyclic sending of the position delta high precision rapid update message enabled
		/// @returns True if the interface has cyclic sending of the position delta high precision rapid update message enabled, otherwise false
		bool get_enable_sending_position_delta_high_precision_rapid_update_cyclically() const;

		/// @brief Instructs the interface to enable or disable sending the position delta high precision rapid update message cyclically
		/// @param[in] enable Set to true to have the interface cyclically send the position delta high precision rapid update message cyclically
		void set_enable_sending_position_delta_high_precision_rapid_update_cyclically(bool enable);

		/// @brief Returns if the interface has cyclic sending of the position rapid update message enabled
		/// @returns True if the interface has cyclic sending of the position rapid update message enabled, otherwise false
		bool get_enable_sending_position_rapid_update_cyclically() const;

		/// @brief Instructs the interface to enable or disable sending the position rapid update message cyclically
		/// @param[in] enable Set to true to have the interface cyclically send the position rapid update message cyclically
		void set_enable_sending_position_rapid_update_cyclically(bool enable);

		/// @brief Returns if the interface has cyclic sending of the rate of turn message enabled
		/// @returns True if the interface has cyclic sending of the rate of turn message enabled, otherwise false
		bool get_enable_sending_rate_of_turn_cyclically() const;

		/// @brief Instructs the interface to enable or disable sending the rate of turn message cyclically
		/// @param[in] enable Set to true to have the interface cyclically send the rate of turn message cyclically
		void set_enable_sending_rate_of_turn_cyclically(bool enable);

		/// @brief Returns if the interface has cyclic sending of the vessel heading message enabled
		/// @returns True if the interface has cyclic sending of the vessel heading message enabled, otherwise false
		bool get_enable_sending_vessel_heading_cyclically() const;

		/// @brief Instructs the interface to enable or disable sending the vessel heading message cyclically
		/// @param[in] enable Set to true to have the interface cyclically send the vessel heading message cyclically
		void set_enable_sending_vessel_heading_cyclically(bool enable);

		/// @brief Initializes the interface. Registers it with the network manager. Must be called before the interface can work properly.
		void initialize();

		/// @brief Returns if initialize has been called
		/// @returns True if initialize has been called, otherwise false. Terminate sets this back to false.
		bool get_initialized() const;

		/// @brief Unregisters the interface from the network manager
		void terminate();

		/// @brief Updates the diagnostic protocol. Must be called periodically. 50ms Is a good minimum interval for this object.
		void update();

	private:
		/// @brief Enumerates a set of flags to manage sending various NMEA2000 messages from this interface
		enum class TransmitFlags : std::uint32_t
		{
			CourseOverGroundSpeedOverGroundRapidUpdate = 0,
			Datum,
			GNSSPositionData,
			PositionDeltaHighPrecisionRapidUpdate,
			PositionRapidUpdate,
			RateOfTurn,
			VesselHeading,

			NumberOfFlags
		};

		/// @brief A generic callback for a the class to process flags from the `ProcessingFlags`
		/// @param[in] flag The flag to process
		/// @param[in] parentPointer A generic context pointer to reference a specific instance of this protocol in the callback
		static void process_flags(std::uint32_t flag, void *parentPointer);

		/// @brief Processes a CAN message destined for an instance of this interface
		/// @param[in] message The CAN message being received
		/// @param[in] parentPointer A context variable to find the relevant class instance
		static void process_rx_message(const CANMessage &message, void *parentPointer);

		/// @brief Checks to see if any received messages are timed out and prunes them if needed
		void check_receive_timeouts();

		/// @brief Checks to see if any transmit flags need to be set based on the last time the message was sent, if enabled.
		void check_transmit_timeouts();

		ProcessingFlags txFlags; ///< A set of flags used to track what messages need to be transmitted or retried
		NMEA2000Messages::CourseOverGroundSpeedOverGroundRapidUpdate cogSogTransmitMessage; ///< Stores a set of data specifically for transmitting the PGN 129026 (0x1F802) if enabled
		NMEA2000Messages::Datum datumTransmitMessage; ///< Stores a set of data specifically for transmitting the PGN 129044 (0x1F814) if enabled
		NMEA2000Messages::GNSSPositionData gnssPositionDataTransmitMessage; ///< Stores a set of data specifically for transmitting the PGN 129029 (0x1F805) if enabled
		NMEA2000Messages::PositionDeltaHighPrecisionRapidUpdate positionDeltaHighPrecisionRapidUpdateTransmitMessage; ///< Stores a set of data specifically for transmitting the PGN 129027 (0x1F803) if enabled
		NMEA2000Messages::PositionRapidUpdate positionRapidUpdateTransmitMessage; ///< Stores a set of data specifically for transmitting the PGN 129025 (0x1F801) if enabled
		NMEA2000Messages::RateOfTurn rateOfTurnTransmitMessage; ///< Stores a set of data specifically for transmitting the PGN 127251 (0x1F113) if enabled
		NMEA2000Messages::VesselHeading vesselHeadingTransmitMessage; ///< Stores a set of data specifically for transmitting the PGN 127250 (0x1F112) if enabled
		std::vector<std::shared_ptr<NMEA2000Messages::CourseOverGroundSpeedOverGroundRapidUpdate>> receivedCogSogMessages; ///< Stores all received (and not timed out) sources of the COG & SOG message
		std::vector<std::shared_ptr<NMEA2000Messages::Datum>> receivedDatumMessages; ///< Stores all received (and not timed out) sources of the Datum message
		std::vector<std::shared_ptr<NMEA2000Messages::GNSSPositionData>> receivedGNSSPositionDataMessages; ///< Stores all received (and not timed out) sources of the GNSS position data message
		std::vector<std::shared_ptr<NMEA2000Messages::PositionDeltaHighPrecisionRapidUpdate>> receivedPositionDeltaHighPrecisionRapidUpdateMessages; ///< Stores all received (and not timed out) sources of the position delta message
		std::vector<std::shared_ptr<NMEA2000Messages::PositionRapidUpdate>> receivedPositionRapidUpdateMessages; ///< Stores all received (and not timed out) sources of the position rapid update message
		std::vector<std::shared_ptr<NMEA2000Messages::RateOfTurn>> receivedRateOfTurnMessages; ///< Stores all received (and not timed out) sources of the rate of turn message
		std::vector<std::shared_ptr<NMEA2000Messages::VesselHeading>> receivedVesselHeadingMessages; ///< Stores all received (and not timed out) sources of the vessel heading message
		EventDispatcher<const std::shared_ptr<NMEA2000Messages::CourseOverGroundSpeedOverGroundRapidUpdate>, bool> cogSogEventPublisher; ///< An event dispatcher for notifying when new guidance machine info messages are received
		EventDispatcher<const std::shared_ptr<NMEA2000Messages::Datum>, bool> datumEventPublisher; ///< An event dispatcher for notifying when new guidance machine info messages are received
		EventDispatcher<const std::shared_ptr<NMEA2000Messages::GNSSPositionData>, bool> gnssPositionDataEventPublisher; ///< An event dispatcher for notifying when new guidance machine info messages are received
		EventDispatcher<const std::shared_ptr<NMEA2000Messages::PositionDeltaHighPrecisionRapidUpdate>, bool> positionDeltaHighPrecisionRapidUpdateEventPublisher; ///< An event dispatcher for notifying when new guidance machine info messages are received
		EventDispatcher<const std::shared_ptr<NMEA2000Messages::PositionRapidUpdate>, bool> positionRapidUpdateEventPublisher; ///< An event dispatcher for notifying when new guidance machine info messages are received
		EventDispatcher<const std::shared_ptr<NMEA2000Messages::RateOfTurn>, bool> rateOfTurnEventPublisher; ///< An event dispatcher for notifying when new guidance machine info messages are received
		EventDispatcher<const std::shared_ptr<NMEA2000Messages::VesselHeading>, bool> vesselHeadingEventPublisher; ///< An event dispatcher for notifying when new guidance machine info messages are received
		bool sendCogSogCyclically; ///< Determines if the interface will try to send the COG & SOG message cyclically
		bool sendDatumCyclically; ///< Determines if the interface will try to send the Datum message cyclically
		bool sendGNSSPositionDataCyclically; ///< Determines if the interface will try to send the GNSS position data message cyclically
		bool sendPositionDeltaHighPrecisionRapidUpdateCyclically; ///< Determines if the interface will try to send the position delta high precision rapid update message message cyclically
		bool sendPositionRapidUpdateCyclically; ///< Determines if the interface will try to send the position rapid update  message cyclically
		bool sendRateOfTurnCyclically; ///< Determines if the interface will try to send the rate of turn message cyclically
		bool sendVesselHeadingCyclically; ///< Determines if the interface will try to send the vessel heading message cyclically
		bool initialized = false; ///< Tracks if initialize has been called
	};
} // namespace isobus
#endif // NMEA2000_MESSAGE_INTERFACE_HPP
