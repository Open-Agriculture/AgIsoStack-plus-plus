//================================================================================================
/// @file can_parameter_group_number_request_protocol.hpp
///
/// @brief A protocol that handles PGN requests
/// @details The purpose of this protocol is to simplify and standardize how PGN requests
/// are made and responded to.
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================
#ifndef CAN_PARAMETER_GROUP_NUMBER_REQUEST_PROTOCOL_HPP
#define CAN_PARAMETER_GROUP_NUMBER_REQUEST_PROTOCOL_HPP

#include "isobus/isobus/can_badge.hpp"
#include "isobus/isobus/can_control_function.hpp"
#include "isobus/isobus/can_network_manager.hpp"

#include <memory>

namespace isobus
{
	//================================================================================================
	/// @class ParameterGroupNumberRequestProtocol
	///
	/// @brief A protocol that handles PGN requests
	/// @details The purpose of this protocol is to simplify and standardize how PGN requests
	/// are made and responded to. It provides a way to easily send a PGN request or a request for
	/// repetition rate, as well as methods to receive PGN requests.
	//================================================================================================
	class ParameterGroupNumberRequestProtocol
	{
	public:
		/// @brief The constructor for this protocol
		/// @param[in] internalControlFunction The internal control function that owns this protocol and will be used to send messages
		explicit ParameterGroupNumberRequestProtocol(std::shared_ptr<InternalControlFunction> internalControlFunction);

		/// @brief The destructor for this protocol
		~ParameterGroupNumberRequestProtocol();

		/// @brief Remove the copy constructor
		ParameterGroupNumberRequestProtocol(const ParameterGroupNumberRequestProtocol &) = delete;

		/// @brief Remove the copy assignment operator
		ParameterGroupNumberRequestProtocol &operator=(const ParameterGroupNumberRequestProtocol &) = delete;

		/// @brief Sends a PGN request to the specified control function
		/// @param[in] pgn The PGN to request
		/// @param[in] source The internal control function to send from
		/// @param[in] destination The control function to request `pgn` from
		/// @returns `true` if the request was successfully sent
		static bool request_parameter_group_number(std::uint32_t pgn, std::shared_ptr<InternalControlFunction> source, std::shared_ptr<ControlFunction> destination);

		/// @brief Sends a PGN request for repetition rate
		/// @details Use this if you want the destination CF to send you the specified PGN at some fixed interval
		/// @param[in] pgn The PGN to request
		/// @param[in] repetitionRate_ms The repetition rate to request in milliseconds
		/// @param[in] source The internal control function to send from
		/// @param[in] destination The control function to send the request to
		/// @returns `true` if the request was sent
		static bool request_repetition_rate(std::uint32_t pgn, std::uint16_t repetitionRate_ms, std::shared_ptr<InternalControlFunction> source, std::shared_ptr<ControlFunction> destination);

		/// @brief Registers for a callback on receipt of a PGN request
		/// @param[in] pgn The PGN you want to handle in the callback
		/// @param[in] callback The callback function to register
		/// @param[in] parentPointer Generic context variable, usually the `this` pointer of the class registering the callback
		/// @returns true if the callback was registered, false if the callback is nullptr or is already registered for the same PGN
		bool register_pgn_request_callback(std::uint32_t pgn, PGNRequestCallback callback, void *parentPointer);

		/// @brief Registers for a callback on receipt of a request for repetition rate
		/// @param[in] pgn The PGN you want to handle in the callback
		/// @param[in] callback The callback function to register
		/// @param[in] parentPointer Generic context variable, usually the `this` pointer of the class registering the callback
		/// @returns true if the callback was registered, false if the callback is nullptr or is already registered for the same PGN
		bool register_request_for_repetition_rate_callback(std::uint32_t pgn, PGNRequestForRepetitionRateCallback callback, void *parentPointer);

		/// @brief Removes a previously registered PGN request callback
		/// @param[in] pgn The PGN associated with the callback
		/// @param[in] callback The callback function to remove
		/// @param[in] parentPointer Generic context variable, usually the `this` pointer of the class that registered the callback
		/// @returns true if the callback was removed, false if no callback matched the parameters
		bool remove_pgn_request_callback(std::uint32_t pgn, PGNRequestCallback callback, void *parentPointer);

		/// @brief Removes a callback for repetition rate requests
		/// @param[in] pgn The PGN associated with the callback
		/// @param[in] callback The callback function to remove
		/// @param[in] parentPointer Generic context variable, usually the `this` pointer of the class that registered the callback
		/// @returns true if the callback was registered, false if the callback is nullptr or is already registered for the same PGN
		bool remove_request_for_repetition_rate_callback(std::uint32_t pgn, PGNRequestForRepetitionRateCallback callback, void *parentPointer);

		/// @brief Returns the number of PGN request callbacks that have been registered with this protocol instance
		/// @returns The number of PGN request callbacks that have been registered with this protocol instance
		std::size_t get_number_registered_pgn_request_callbacks() const;

		/// @brief Returns the number of PGN request for repetition rate callbacks that have been registered with this protocol instance
		/// @returns The number of PGN request for repetition rate callbacks that have been registered with this protocol instance
		std::size_t get_number_registered_request_for_repetition_rate_callbacks() const;

	private:
		/// @brief A storage class for holding PGN callbacks and their associated PGN
		class PGNRequestCallbackInfo
		{
		public:
			/// @brief Constructor for PGNRequestCallbackInfo
			/// @param[in] callback A PGNRequestCallback
			/// @param[in] parameterGroupNumber The PGN associated with the callback
			/// @param[in] parentPointer Pointer to the class that registered the callback, or `nullptr`
			PGNRequestCallbackInfo(PGNRequestCallback callback, std::uint32_t parameterGroupNumber, void *parentPointer);

			/// @brief A utility function for determining if the data in the object is equal to another object
			/// @details The objects are the same if the pgn and callbackFunction both match
			/// @param[in] obj The object to compare against
			/// @returns true if the objects have identical data
			bool operator==(const PGNRequestCallbackInfo &obj) const;

			PGNRequestCallback callbackFunction; ///< The actual callback
			std::uint32_t pgn; ///< The PGN associated with the callback
			void *parent; ///< Pointer to the class that registered the callback, or `nullptr`
		};

		/// @brief A storage class for holding PGN callbacks and their associated PGN
		class PGNRequestForRepetitionRateCallbackInfo
		{
		public:
			/// @brief Constructor for PGNRequestCallbackInfo
			/// @param[in] callback A PGNRequestCallback
			/// @param[in] parameterGroupNumber The PGN associated with the callback
			/// @param[in] parentPointer Pointer to the class that registered the callback, or `nullptr`
			PGNRequestForRepetitionRateCallbackInfo(PGNRequestForRepetitionRateCallback callback, std::uint32_t parameterGroupNumber, void *parentPointer);

			/// @brief A utility function for determining if the data in the object is equal to another object
			/// @details The objects are the same if the pgn and callbackFunction both match
			/// @param[in] obj The object to compare against
			/// @returns true if the objects have identical data
			bool operator==(const PGNRequestForRepetitionRateCallbackInfo &obj) const;

			PGNRequestForRepetitionRateCallback callbackFunction; ///< The actual callback
			std::uint32_t pgn; ///< The PGN associated with the callback
			void *parent; ///< Pointer to the class that registered the callback, or `nullptr`
		};

		static constexpr std::uint8_t PGN_REQUEST_LENGTH = 3; ///< The CAN data length of a PGN request

		/// @brief A generic way for a protocol to process a received message
		/// @param[in] message A received CAN message
		void process_message(const CANMessage &message);

		/// @brief A generic way for a protocol to process a received message
		/// @param[in] message A received CAN message
		/// @param[in] parent Provides the context to the actual TP manager object
		static void process_message(const CANMessage &message, void *parent);

		/// @brief Sends a message using the acknowledgement PGN
		/// @param[in] type The type of acknowledgement to send (Ack, vs Nack, etc)
		/// @param[in] parameterGroupNumber The PGN to acknowledge
		/// @param[in] destination The destination control function to send the acknowledgement to
		/// @returns true if the message was sent, false otherwise
		bool send_acknowledgement(AcknowledgementType type, std::uint32_t parameterGroupNumber, std::shared_ptr<ControlFunction> destination) const;

		std::shared_ptr<InternalControlFunction> myControlFunction; ///< The internal control function that this protocol will send from
		std::vector<PGNRequestCallbackInfo> pgnRequestCallbacks; ///< A list of all registered PGN callbacks and the PGN associated with each callback
		std::vector<PGNRequestForRepetitionRateCallbackInfo> repetitionRateCallbacks; ///< A list of all registered request for repetition rate callbacks and the PGN associated with the callback
		Mutex pgnRequestMutex; ///< A mutex to protect the callback lists
	};
}

#endif // CAN_PARAMETER_GROUP_NUMBER_REQUEST_PROTOCOL_HPP
