#pragma once

#include "can_message.hpp"

namespace isobus
{
    typedef void (*CANLibCallback)(CANMessage *message, void *parentPointer);

    class ParameterGroupNumberCallbackData
    {
    public:
        ParameterGroupNumberCallbackData(std::uint32_t parameterGroupNumber, CANLibCallback callback);
        ParameterGroupNumberCallbackData(const ParameterGroupNumberCallbackData &oldObj);

        bool operator==(const ParameterGroupNumberCallbackData& obj);
        ParameterGroupNumberCallbackData& operator= (const ParameterGroupNumberCallbackData &obj);

        std::uint32_t get_parameter_group_number() const;
        CANLibCallback get_callback() const;
    private:
        CANLibCallback mCallback;
        std::uint32_t mParameterGroupNumber;
    };
} // namespace isobus
