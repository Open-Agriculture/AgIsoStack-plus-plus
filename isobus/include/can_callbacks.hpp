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
	typedef void (*CANLibCallback)(CANMessage *message, void *parentPointer);
	typedef bool (*DataChunkCallback)(std::uint32_t callbackIndex,
	                                  std::uint32_t bytesOffset,
	                                  std::uint32_t numberOfBytesNeeded,
	                                  std::uint8_t *chunkBuffer,
	                                  void *parentPointer);
	typedef void (*TransmitCompleteCallback)(std::uint32_t parameterGroupNumber,
	                                         std::uint32_t dataLength,
	                                         InternalControlFunction *sourceControlFunction,
	                                         ControlFunction *destinationControlFunction,
	                                         bool successful,
	                                         void *parentPointer);

    class ParameterGroupNumberCallbackData
    {
    public:
        ParameterGroupNumberCallbackData(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parentPointer);
        ParameterGroupNumberCallbackData(const ParameterGroupNumberCallbackData &oldObj);

        bool operator==(const ParameterGroupNumberCallbackData& obj);
        ParameterGroupNumberCallbackData& operator= (const ParameterGroupNumberCallbackData &obj);

        std::uint32_t get_parameter_group_number() const;
        CANLibCallback get_callback() const;
		void *get_parent() const;
    private:
        CANLibCallback mCallback;
        std::uint32_t mParameterGroupNumber;
		void *mParent;
    };
} // namespace isobus

#endif // CAN_CALLBACKS_HPP
