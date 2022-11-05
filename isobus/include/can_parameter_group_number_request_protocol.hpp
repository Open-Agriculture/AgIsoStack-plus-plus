//================================================================================================
/// @file can_parameter_group_number_request_protocol.hpp
///
/// @brief A protocol that handles PGN requests
/// @details The purpose of this protocol is to simplify and standardize how PGN requests
/// are made and responded to.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#ifndef CAN_PARAMETER_GROUP_NUMBER_REQUEST_PROTOCOL_HPP
#define CAN_PARAMETER_GROUP_NUMBER_REQUEST_PROTOCOL_HPP

#include "can_badge.hpp"
#include "can_control_function.hpp"
#include "can_managed_message.hpp"
#include "can_network_manager.hpp"
#include "can_protocol.hpp"

#include <memory>

namespace isobus
{
	//================================================================================================
	/// @class ParameterGroupNumberRequestProtocol
	///
	/// @brief A protocol that handles PGN requests
	/// @details The purpose of this protocol is to simplify and standardize how PGN requests
	/// are made and responded to. It provides a way to easily send a PGN request or a request for
	/// repitition rate, as well as methods to receive PGN requests.
	//================================================================================================
	class ParameterGroupNumberRequestProtocol : public CANLibProtocol
	{
	public:
		/// @brief The protocol's initializer function
		void initialize(CANLibBadge<CANNetworkManager>) override;

		/// @brief Used to tell the CAN stack that PGN requests should be handled for the specified internal control function
		/// @details This will allocate an instance of this protocol
		/// @returns `true` If the protocol instance was created OK with the passed in ICF
		static bool assign_pgn_request_protocol_to_internal_control_function(std::shared_ptr<InternalControlFunction> internalControlFunction);

		/// @brief Used to tell the CAN stack that PGN requests should no longer be handled for the specified internal control function
		/// @details This will delete an instance of this protocol
		/// @returns `true` If the protocol instance was deleted OK according to the passed in ICF
		static bool deassign_pgn_request_protocol_to_internal_control_function(std::shared_ptr<InternalControlFunction> internalControlFunction);

		/// @brief Sends a PGN request to the specified control function
		/// @param[in] pgn The PGN to request
		/// @param[in] source The internal control function to send from
		/// @param[in] destination The control function to request `pgn` from
		/// @returns `true` if the request was successfully sent
		static bool request_parameter_group_number(std::uint32_t pgn, InternalControlFunction *source, ControlFunction *destination);

		/// @brief Sends a PGN request for repitition rate
		/// @details Use this if you want the requestee to send you the specified PGN at some fixed interval
		/// @param[in] pgn The PGN to request
		/// @param[in] repetitionRate_ms The repetition rate to request in milliseconds
		/// @param[in] source The internal control function to send from
		/// @param[in] destination The control function to send the request to
		/// @returns `true` if the request was sent
		static bool request_repetition_rate(std::uint32_t pgn, std::uint16_t repetitionRate_ms, InternalControlFunction *source, ControlFunction *destination);

		/// @brief Registers for a callback on receipt of a PGN request
		/// @param[in] pgn The PGN you want to handle in the callback
		/// @param[in] callback The callback function to register
		/// @returns true if the callback was registered, false if the callback is nullptr or is already registered for the same PGN
		bool register_pgn_request_callback(std::uint32_t pgn, PGNRequestCallback callback);

		/// @brief Registers for a callback on receipt of a request for repetition rate
		/// @param[in] pgn The PGN you want to handle in the callback
		/// @param[in] callback The callback function to register
		/// @returns true if the callback was registered, false if the callback is nullptr or is already registered for the same PGN
		bool register_request_for_repetition_rate_callback(std::uint32_t pgn, PGNRequestCallback callback);

		/// @brief Updates the protocol cyclically
		void update(CANLibBadge<CANNetworkManager>) override;

		static constexpr std::uint8_t PGN_REQUEST_LENGTH = 3; ///< The CAN data length of a PGN request

	private:
		/// @brief A storage class for holding PGN callbacks and their associated PGN
		class PGNRequestCallbackInfo
		{
		public:
			/// @brief Constructor for PGNRequestCallbackInfo
			/// @param[in] callback A PGNRequestCallback
			/// @param[in] parameterGroupNumber The PGN associcated with the callback
			PGNRequestCallbackInfo(PGNRequestCallback callback, std::uint32_t parameterGroupNumber);

			/// @brief A utility function for determining if the data in the object is equal to another object
			/// @details The objects are the same if the pgn and callbackFunction both match
			/// @param[in] obj The object to compare against
			/// @returns true if the objects have identical data
			bool operator==(const PGNRequestCallbackInfo &obj);

			PGNRequestCallback callbackFunction; ///< The actual callback
			std::uint32_t pgn; ///< The PGN associated with the callback
		};

		/// @brief The types of acknowldegement that can be sent in the Ack PGN
		enum class AcknowledgementType : std::uint8_t
		{
			Positive = 0, ///< "ACK" Indicates that the request was completed
			Negative = 1, ///< "NACK" Indicates the request was not completed or we do not support the PGN
			AccessDenied = 2, ///< Signals to the requestor that their CF is not allowed to request this PGN
			CannotRespond = 3 ///< Signals to the requestor that we are unable to accept the request for some reason
		};

		/// @brief Constructor for the PGN request protocol
		/// @param[in] internalControlFunction The internal control function assigned to the protocol instance
		ParameterGroupNumberRequestProtocol(std::shared_ptr<InternalControlFunction> internalControlFunction);

		/// @brief Destructor for the PGN request protocol
		~ParameterGroupNumberRequestProtocol();

		/// @brief A generic way for a protocol to process a received message
		/// @param[in] message A received CAN message
		void process_message(CANMessage *const message) override;

		/// @brief A generic way for a protocol to process a received message
		/// @param[in] message A received CAN message
		/// @param[in] parent Provides the context to the actual TP manager object
		static void process_message(CANMessage *const message, void *parent);

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

		/// @brief Sends a message using the acknowledgement PGN
		/// @param[in] type The type of acknowledgement to send (Ack, vs Nack, etc)
		/// @param[in] parameterGroupNumber The PGN to acknowledge
		/// @param[in] source The source control function to send from
		/// @param[in] destination The destination control function to send the acknowledgement to
		/// @returns true if the message was sent, false otherwise
		bool send_acknowledgement(AcknowledgementType type, std::uint32_t parameterGroupNumber, InternalControlFunction *source, ControlFunction *destination);

		static std::list<ParameterGroupNumberRequestProtocol *> pgnRequestProtocolList; ///< List of all PGN request protocol instances (one per ICF)

		std::shared_ptr<InternalControlFunction> myControlFunction; ///< The internal control function that this protocol will send from
		std::vector<PGNRequestCallbackInfo> pgnRequestCallbacks; ///< A list of all registered PGN callbacks and the PGN associated with each callback
		std::vector<PGNRequestCallbackInfo> repetitionRateCallbacks; ///< A list of all registered request for repetition rate callbacks and the PGN associated with the callback
		std::mutex pgnRequestMutex; ///< A mutex to protect the callback lists
	};
}

#endif // CAN_PARAMETER_GROUP_NUMBER_REQUEST_PROTOCOL_HPP
