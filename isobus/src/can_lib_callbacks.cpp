#include "can_lib_callbacks.hpp"

namespace isobus
{
    ParameterGroupNumberCallbackData::ParameterGroupNumberCallbackData(std::uint32_t parameterGroupNumber, CANLibCallback callback) :
	  mCallback(callback),
	  mParameterGroupNumber(parameterGroupNumber)
	{
	}

	ParameterGroupNumberCallbackData::ParameterGroupNumberCallbackData(const ParameterGroupNumberCallbackData &oldObj)
	{
		mCallback = oldObj.mCallback;
		mParameterGroupNumber = oldObj.mParameterGroupNumber;
	}

	bool ParameterGroupNumberCallbackData::operator==(const ParameterGroupNumberCallbackData &obj)
	{
		return ((obj.mCallback == this->mCallback) && (obj.mParameterGroupNumber == this->mParameterGroupNumber));
	}

	ParameterGroupNumberCallbackData& ParameterGroupNumberCallbackData::operator= (const ParameterGroupNumberCallbackData &obj)
	{
		mCallback = obj.mCallback;
		mParameterGroupNumber = obj.mParameterGroupNumber;
		return *this;
	}

	std::uint32_t ParameterGroupNumberCallbackData::get_parameter_group_number() const
	{
		return mParameterGroupNumber;
	}

    CANLibCallback ParameterGroupNumberCallbackData::get_callback() const
	{
		return mCallback;
	}
} // namespace isobus
