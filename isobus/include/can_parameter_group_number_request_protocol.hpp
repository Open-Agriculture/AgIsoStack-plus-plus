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
		ParameterGroupNumberRequestProtocol();

		~ParameterGroupNumberRequestProtocol();

		/// @brief The protocol's initializer function
		void initialize(CANLibBadge<CANNetworkManager>) override;

		/// @brief Sends a PGN request to the specified control function
		/// @param pgn The PGN to request
		/// @param destination The control function to request `pgn` from
		/// @returns `true` if the request was successfully sent
		bool request_parameter_group_number(std::uint32_t pgn, InternalControlFunction *source, ControlFunction *destination);

		/// @brief Sends a PGN request for repitition rate
		/// @details Use this if you want the requestee to send you the specified PGN at some fixed interval
		/// @param pgn The PGN to request
		/// @param destination The control function to send the request to
		/// @returns `true` if the request was sent
		bool request_repetition_rate(std::uint32_t pgn, std::uint16_t repetitionRate_ms, InternalControlFunction *source, ControlFunction *destination);

		/// @brief Registers for a callback on receipt of a PGN request
		bool register_pgn_request_callback(std::uint32_t pgn, PGNRequestCallback callback);

		bool register_request_for_repetition_rate_callback(std::uint32_t pgn, PGNRequestCallback callback);

		static constexpr std::uint8_t PGN_REQUEST_LENGTH = 3;

	private:
		class PGNRequestCallbackInfo
		{
		public:
			PGNRequestCallbackInfo(PGNRequestCallback callback, std::uint32_t parameterGroupNumber);

			bool operator==(const PGNRequestCallbackInfo &obj);

			PGNRequestCallback callbackFunction;
			std::uint32_t pgn;
		};

		enum class AcknowledgementType : std::uint8_t
		{
			Positive = 0,
			Negative = 1,
			AccessDenied = 2,
			CannotRespond = 3
		};

		/// @brief A generic way for a protocol to process a received message
		/// @param[in] message A received CAN message
		void process_message(CANMessage *const message) override;

		/// @brief A generic way for a protocol to process a received message
		/// @param[in] message A received CAN message
		/// @param[in] parent Provides the context to the actual TP manager object
		static void process_message(CANMessage *const message, void *parent);

		bool send_acknowledgement(AcknowledgementType type, std::uint32_t parameterGroupNumber, InternalControlFunction *source, ControlFunction *destination);

		std::vector<PGNRequestCallbackInfo> pgnRequestCallbacks;
		std::vector<PGNRequestCallbackInfo> repetitionRateCallbacks;
		std::mutex pgnRequestMutex;
	};
}

#endif // CAN_PARAMETER_GROUP_NUMBER_REQUEST_PROTOCOL_HPP
