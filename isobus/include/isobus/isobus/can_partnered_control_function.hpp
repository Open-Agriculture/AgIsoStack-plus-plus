//================================================================================================
/// @file can_partnered_control_function.hpp
///
/// @brief A class that describes a control function on the bus that the stack should communicate
/// with. Use these to describe ECUs you want to send messages to.
/// @author Adrian Del Grosso
/// @author Daan Steenbergen
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================

#ifndef CAN_PARTNERED_CONTROL_FUNCTION_HPP
#define CAN_PARTNERED_CONTROL_FUNCTION_HPP

#include "isobus/isobus/can_NAME_filter.hpp"
#include "isobus/isobus/can_badge.hpp"
#include "isobus/isobus/can_callbacks.hpp"
#include "isobus/isobus/can_control_function.hpp"

#include <vector>

namespace isobus
{
	class CANNetworkManager;
	class InternalControlFunction;

	/// @brief This represents any device on the bus you want to talk to.
	/// @details To communicate with a device on the bus, create one of these objects and tell it
	/// via the constructor what the identity of that device is using NAME fields like
	/// manufacturer code, function, and device class. The stack will take care of locating the
	/// device on the bus that matches that description, and will allow you to talk to it through
	/// passing this object to the appropriate send function in the network manager.
	class PartneredControlFunction : public ControlFunction
	{
	public:
		/// @brief the constructor for a PartneredControlFunction, which is called by the factory function
		/// @param[in] CANPort The CAN channel associated with this control function definition
		/// @param[in] NAMEFilters A list of filters that describe the identity of the CF based on NAME components
		PartneredControlFunction(std::uint8_t CANPort, const std::vector<NAMEFilter> &NAMEFilters);

		/// @brief Deleted copy constructor for PartneredControlFunction to avoid slicing
		PartneredControlFunction(PartneredControlFunction &) = delete;

		/// @brief This is how you get notified that this control function has sent you a destination specific message.
		/// @details Add a callback function here to be notified when this device has sent you a message with the specified PGN.
		/// You can also get callbacks for any/all PGNs if you pass in `CANLibParameterGroupNumber::Any` as the PGN.
		/// Optionally you can use the parent pointer to get context inside your callback as to what C++ object the callback is
		/// destined for. Whatever you pass in `parent` will be passed back to you in the callback. In theory, you could use
		/// that variable for passing any arbitrary data through the callback also.
		/// You can add as many callbacks as you want, and can use the same function for multiple PGNs if you want.
		/// @param[in] parameterGroupNumber The PGN you want to use to communicate, or `CANLibParameterGroupNumber::Any`
		/// @param[in] callback The function you want to get called when a message is received with parameterGroupNumber from this CF
		/// @param[in] parent A generic context variable that helps identify what object the callback was destined for
		/// @param[in] internalControlFunction An internal control function to filter based on. If you supply this
		/// parameter the callback will only be called when messages from the partner are received with the
		/// specified ICF as the destination.
		void add_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parent, std::shared_ptr<InternalControlFunction> internalControlFunction = nullptr);

		/// @brief Removes a callback matching *exactly* the parameters passed in
		/// @param[in] parameterGroupNumber The PGN associated with the callback being removed
		/// @param[in] callback The callback function being removed
		/// @param[in] parent A generic context variable that helps identify what object the callback was destined for
		/// @param[in] internalControlFunction The ICF being used to filter messages against
		void remove_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parent, std::shared_ptr<InternalControlFunction> internalControlFunction = nullptr);

		/// @brief Returns the number of parameter group number callbacks associated with this control function
		/// @returns The number of parameter group number callbacks associated with this control function
		std::size_t get_number_parameter_group_number_callbacks() const;

		/// @brief Returns the number of NAME filter objects that describe the identity of this control function
		/// @returns The number of NAME filter objects that describe the identity of this control function
		std::size_t get_number_name_filters() const;

		/// @brief Returns the number of NAME filters with a specific NAME parameter component, like manufacturer code
		/// @param[in] parameter The NAME parameter to check against
		/// @returns The number of NAME filters with a specific NAME parameter component
		std::size_t get_number_name_filters_with_parameter_type(NAME::NAMEParameters parameter) const;

		/// @brief Returns a NAME filter by index
		/// @param[in] index The index of the filter to get
		/// @param[out] parameter The returned parameter type
		/// @param[out] filterValue The raw value of the filter associated with the `parameter`
		/// @returns true if a filter was returned successfully, false if the index was out of range
		bool get_name_filter_parameter(std::size_t index, NAME::NAMEParameters &parameter, std::uint32_t &filterValue) const;

		/// @brief Checks to see if a NAME matches this CF's NAME filters
		/// @param[in] NAMEToCheck The NAME to check against this control function's filters
		/// @returns true if this control function matches the NAME that was passed in, false otherwise
		bool check_matches_name(NAME NAMEToCheck) const;

	private:
		friend class CANNetworkManager; ///< Allows the network manager to use get_parameter_group_number_callback

		/// @brief Returns a parameter group number associated with this control function by index
		/// @param[in] index The index from which to get the PGN callback data object
		/// @returns A reference to the PGN callback data object at the index specified
		ParameterGroupNumberCallbackData &get_parameter_group_number_callback(std::size_t index);

		const std::vector<NAMEFilter> NAMEFilterList; ///< A list of NAME parameters that describe this control function's identity
		std::vector<ParameterGroupNumberCallbackData> parameterGroupNumberCallbacks; ///< A list of all parameter group number callbacks associated with this control function
		bool initialized = false; ///< A way to track if the network manager has processed this CF against existing CFs
	};

} // namespace isobus

#endif // CAN_PARTNERED_CONTROL_FUNCTION
