//================================================================================================
/// @file can_callbacks.cpp
///
/// @brief An object to represent common callbacks used within this CAN stack.
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/can_callbacks.hpp"

namespace isobus
{
	ParameterGroupNumberCallbackData::ParameterGroupNumberCallbackData(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parentPointer, std::shared_ptr<InternalControlFunction> internalControlFunction) :
	  callback(callback),
	  parameterGroupNumber(parameterGroupNumber),
	  parent(parentPointer),
	  internalControlFunctionFilter(internalControlFunction)
	{
	}

	bool ParameterGroupNumberCallbackData::operator==(const ParameterGroupNumberCallbackData &obj) const
	{
		return ((obj.callback == this->callback) &&
		        (obj.parameterGroupNumber == this->parameterGroupNumber) &&
		        (obj.parent == this->parent) &&
		        (obj.internalControlFunctionFilter == this->internalControlFunctionFilter));
	}

	std::uint32_t ParameterGroupNumberCallbackData::get_parameter_group_number() const
	{
		return parameterGroupNumber;
	}

	CANLibCallback ParameterGroupNumberCallbackData::get_callback() const
	{
		return callback;
	}

	void *ParameterGroupNumberCallbackData::get_parent() const
	{
		return parent;
	}

	std::shared_ptr<InternalControlFunction> ParameterGroupNumberCallbackData::get_internal_control_function() const
	{
		return internalControlFunctionFilter;
	}
} // namespace isobus
