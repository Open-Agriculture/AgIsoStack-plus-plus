//================================================================================================
/// @file can_callbacks.cpp
///
/// @brief An object to represent common callbacks used within this CAN stack.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/can_callbacks.hpp"

namespace isobus
{
	ParameterGroupNumberCallbackData::ParameterGroupNumberCallbackData(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parentPointer, std::shared_ptr<InternalControlFunction> internalControlFunction) :
	  mCallback(callback),
	  mParameterGroupNumber(parameterGroupNumber),
	  mParent(parentPointer),
	  mInternalControlFunctionFilter(internalControlFunction)
	{
	}

	ParameterGroupNumberCallbackData::ParameterGroupNumberCallbackData(const ParameterGroupNumberCallbackData &oldObj) :
	  mCallback(oldObj.mCallback),
	  mParameterGroupNumber(oldObj.mParameterGroupNumber),
	  mParent(oldObj.mParent),
	  mInternalControlFunctionFilter(oldObj.mInternalControlFunctionFilter)
	{
	}

	bool ParameterGroupNumberCallbackData::operator==(const ParameterGroupNumberCallbackData &obj) const
	{
		return ((obj.mCallback == this->mCallback) &&
		        (obj.mParameterGroupNumber == this->mParameterGroupNumber) &&
		        (obj.mParent == this->mParent) &&
		        (obj.mInternalControlFunctionFilter == this->mInternalControlFunctionFilter));
	}

	ParameterGroupNumberCallbackData &ParameterGroupNumberCallbackData::operator=(const ParameterGroupNumberCallbackData &obj)
	{
		mCallback = obj.mCallback;
		mParameterGroupNumber = obj.mParameterGroupNumber;
		mParent = obj.mParent;
		mInternalControlFunctionFilter = obj.mInternalControlFunctionFilter;
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

	void *ParameterGroupNumberCallbackData::get_parent() const
	{
		return mParent;
	}

	std::shared_ptr<InternalControlFunction> ParameterGroupNumberCallbackData::get_internal_control_function() const
	{
		return mInternalControlFunctionFilter;
	}
} // namespace isobus
