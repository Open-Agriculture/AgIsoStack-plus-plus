//================================================================================================
/// @file can_callbacks.hpp
///
/// @brief An object to represent common callbacks used within this CAN stack.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_CALLBACKS_HPP
#define CAN_CALLBACKS_HPP

#include "can_message.hpp"

namespace isobus
{
	class InternalControlFunction;
	class ControlFunction;
	/// @brief A callback for control functions to get CAN messages
	typedef void (*CANLibCallback)(CANMessage *message, void *parentPointer);
	/// @brief A callback to get chunks of data for transfer by a protocol
	typedef bool (*DataChunkCallback)(std::uint32_t callbackIndex,
	                                  std::uint32_t bytesOffset,
	                                  std::uint32_t numberOfBytesNeeded,
	                                  std::uint8_t *chunkBuffer,
	                                  void *parentPointer);
	/// @brief A callback for when a transmit is completed by the stack
	typedef void (*TransmitCompleteCallback)(std::uint32_t parameterGroupNumber,
	                                         std::uint32_t dataLength,
	                                         InternalControlFunction *sourceControlFunction,
	                                         ControlFunction *destinationControlFunction,
	                                         bool successful,
	                                         void *parentPointer);
	typedef bool (*PGNRequestCallback)(std::uint32_t parameterGroupNumber,
	                                   const ControlFunction *requestingControlFunction,
	                                   bool &acknowledge);

	typedef bool (*PGNRequestForRepetitionRateCallback)(std::uint32_t parameterGroupNumber,
	                                                    const ControlFunction *requestingControlFunction,
	                                                    std::uint32_t repetitionRate);

	//================================================================================================
	/// @class ParameterGroupNumberCallbackData
	///
	/// @brief A storage class to hold data about PGN callbacks.
	//================================================================================================
	class ParameterGroupNumberCallbackData
	{
	public:
		/// @brief A constructor for holding callback data
		/// @param[in] parameterGroupNumber The PGN you want to register a callback for
		/// @param[in] callback The function you want the stack to call when it gets receives a message with a matching PGN
		/// @param[in] parentPointer A generic variable that can provide context to which object the callback was meant for
		ParameterGroupNumberCallbackData(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parentPointer);

		/// @brief A copy constructor for holding callback data
		/// @param[in] oldObj The object to copy from
		ParameterGroupNumberCallbackData(const ParameterGroupNumberCallbackData &oldObj);

		/// @brief Equality operator for this class
		/// @param[in] obj The object to check equality against
		/// @returns true if the objects have equivalent data
		bool operator==(const ParameterGroupNumberCallbackData &obj);

		/// @brief Assignment operator for this class
		/// @param[in] obj The object to assign data from
		/// @returns The lhs of the operator
		ParameterGroupNumberCallbackData &operator=(const ParameterGroupNumberCallbackData &obj);

		/// @brief Returns the PGN associated with this callback data
		/// @returns The PGN associated with this callback data
		std::uint32_t get_parameter_group_number() const;

		/// @brief Returns the callback pointer for this data object
		/// @returns The callback pointer for this data object
		CANLibCallback get_callback() const;

		/// @brief Returns the parent pointer for this data object
		void *get_parent() const;

	private:
		CANLibCallback mCallback; ///< The callback that will get called when a matching PGN is received
		std::uint32_t mParameterGroupNumber; ///< The PGN assocuiated with this callback
		void *mParent; ///< A generic variable that can provide context to which object the callback was meant for
	};
} // namespace isobus

#endif // CAN_CALLBACKS_HPP
