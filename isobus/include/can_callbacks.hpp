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
    typedef void (*CANLibCallback)(CANMessage *message, void *parentPointer);

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
