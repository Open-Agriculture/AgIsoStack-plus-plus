//================================================================================================
/// @file isobus_time_date_interface.hpp
///
/// @brief Defines an interface for accessing or sending time and date information using
/// the Time/Date (TD) PGN. Can be useful for interacting with an ISOBUS file server,
/// or just for keeping track of time and date information as provided by some authoritative
/// control function on the bus. Control functions which provide the message this interface
/// manages are expected to have a real-time clock (RTC) or GPS time source.
/// @author Adrian Del Grosso
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#ifndef ISOBUS_TIME_DATE_INTERFACE_HPP
#define ISOBUS_TIME_DATE_INTERFACE_HPP

#include "isobus/isobus/can_callbacks.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_message.hpp"
#include "isobus/utility/event_dispatcher.hpp"

namespace isobus
{
	/// @brief An interface for sending and receiving time and date information using the Time/Date (TD) PGN, 0xFEE6.
	/// You may hear this time referred to as "ISOBUS Time" in some cases. It is normally provided by control functions with a
	/// real-time clock (RTC) or GPS source. This is not the same thing as the NMEA2000 time and date, which is PGN 129033 (0x1F809), and is
	/// backwards compatible with J1939 which uses this same PGN and message structure.
	class TimeDateInterface
	{
	public:
		/// @brief A struct to hold time and date information.
		/// This will generally be a UTC time and date, unless the local hour offset is 0,
		/// in which case it will be a local time and date.
		/// We store it slightly differently than the PGN to make it easier to work with.
		struct TimeAndDate
		{
			std::uint16_t milliseconds = 0; ///< Number of milliseconds. This has resolution of 0.25s, so it will be either 0, 250, 500, or 750
			std::uint8_t seconds = 0; ///< Number of seconds, range: 0 to 59s
			std::uint8_t minutes = 0; ///< Number of minutes, range: 0 to 59m
			std::uint8_t hours = 0; ///< Number of hours, range: 0 to 23h
			std::uint8_t quarterDays = 0; ///< Number of quarter days. This is a less precise version of "hours" that is used in some cases. Range: 0 to 3. 0 is midnight, 1 is 6am, 2 is noon, 3 is 6pm
			std::uint8_t day = 0; ///< Number of days, range 0 to 31
			std::uint8_t month = 0; ///< Number of months, range 1 to 12
			std::uint16_t year = 1985; ///< The year. Range: 1985 to 2235
			std::int8_t localMinuteOffset = 0; ///< Local minute offset is the number of minutes between the UTC time and date and a local time and date. This value is added to UTC time and date to determine the local time and date. The local offset is a positive value for times east of the Prime Meridian to the International Date Line.
			std::int8_t localHourOffset = 0; ///< Local hour offset is the number of hours between the UTC time and date and a local time and date. This value is added to UTC time and date to determine the local time and date. The local offset is a positive value for times east of the Prime Meridian to the International Date Line.
		};

		/// @brief A struct to hold time and date information and the control function that sent it.
		/// Used by the event dispatcher to provide event driven access to time and date information.
		struct TimeAndDateInformation
		{
			TimeAndDate timeAndDate; ///< The time and date information
			std::shared_ptr<ControlFunction> controlFunction; ///< The control function that sent the time and date information
		};

		/// @brief Constructor for the TimeDateInterface class, with no source control function.
		/// Receives time and date information from the bus, and does not transmit.
		/// This is generally the normal use case for this class.
		TimeDateInterface() = default;

		/// @brief Constructor for the TimeDateInterface class, used for when you want to also transmit the time/date.
		/// @param sourceControlFunction If you want to transmit the time and date information, you
		/// can pass a control function in this parameter to be used as the source of the information.
		/// @param timeAndDateCallback A callback that will be called when the interface needs you to tell it the current time and date.
		/// This is used to populate the time and date information that will be sent out on the bus. The function you use for this callback
		/// should be relatively quick as it will be called from the CAN stack's thread, and you don't want to delay the stack's update thread.
		/// The function should return "true" if the time and date information was successfully populated, and "false" if it was not.
		/// Note that if it returns false, the request will probably be NACKed, which is not ideal.
		TimeDateInterface(std::shared_ptr<InternalControlFunction> sourceControlFunction, const std::function<bool(TimeAndDate &timeAndDateToPopulate)> &timeAndDateCallback);

		/// @brief Destructor for the TimeDateInterface class.
		~TimeDateInterface();

		/// @brief Deleted copy constructor for TimeDateInterface
		TimeDateInterface(TimeDateInterface &) = delete;

		/// @brief Initializes the interface.
		/// @details This needs to be called before the interface is usable.
		/// It registers its PGN callback and sets up the PGN request interface
		/// if needed.
		void initialize();

		/// @brief Returns if initialize has been called yet
		/// @return `true` if initialize has been called, otherwise false
		bool is_initialized() const;

		/// @brief Returns the event dispatcher for time and date information.
		/// Use this to subscribe to event-driven time and date information events.
		/// @return The event dispatcher for time and date information
		EventDispatcher<TimeAndDateInformation> &get_event_dispatcher();

		/// @brief Sends a time and date message (a broadcast message) as long as the interface
		/// has been initialized and a control function has been set.
		/// @param timeAndDateToSend The time and date information to send
		/// @return `true` if the message was sent, otherwise `false`
		bool send_time_and_date(const TimeAndDate &timeAndDateToSend) const;

		/// @brief Requests time and date information from a specific control function, or from all control functions to see if any respond.
		/// Responses can be monitored by using the event dispatcher. See get_event_dispatcher.
		/// This is really just a very thin wrapper around the PGN request interface for convenience.
		/// @param requestingControlFunction This control function will be used to send the request.
		/// @param optionalDestination If you want to request time and date information from a specific control function, you can pass it here, otherwise pass an empty pointer.
		/// @return `true` if the request was sent, otherwise `false`
		bool request_time_and_date(std::shared_ptr<InternalControlFunction> requestingControlFunction, std::shared_ptr<ControlFunction> optionalDestination = nullptr) const;

		/// @brief Returns the control function that is being used as the source of the time and date information if one was set.
		/// @return The control function that is being used as the source of the time and date information, or an empty pointer if one was not set.
		std::shared_ptr<InternalControlFunction> get_control_function() const;

	private:
		/// @brief Parses incoming CAN messages into usable unit and language settings
		/// @param message The CAN message to parse
		/// @param parentPointer A generic context variable, usually the `this` pointer for this interface instance
		static void process_rx_message(const CANMessage &message, void *parentPointer);

		/// @brief Processes a PGN request
		/// @param[in] parameterGroupNumber The PGN being requested
		/// @param[in] requestingControlFunction The control function that is requesting the PGN
		/// @param[in] acknowledge If the request should be acknowledged (will always be false for this interface)
		/// @param[in] acknowledgeType How to acknowledge the request (will always be NACK for this interface)
		/// @param[in] parentPointer A context variable to find the relevant instance of this class
		/// @returns True if the request was serviced, otherwise false.
		static bool process_request_for_time_date(std::uint32_t parameterGroupNumber,
		                                          std::shared_ptr<ControlFunction> requestingControlFunction,
		                                          bool &acknowledge,
		                                          AcknowledgementType &acknowledgeType,
		                                          void *parentPointer);

		std::shared_ptr<InternalControlFunction> myControlFunction; ///< The control function to send messages as, or an empty pointer if not sending
		std::function<bool(TimeAndDate &timeAndDateToPopulate)> userTimeDateCallback; ///< The callback the user provided to get the time and date information at runtime to be transmitted
		EventDispatcher<TimeAndDateInformation> timeAndDateEventDispatcher; ///< The event dispatcher for time and date information
		bool initialized = false; ///< If the interface has been initialized yet
	};
} // namespace isobus
#endif // ISOBUS_TIME_DATE_INTERFACE_HPP
