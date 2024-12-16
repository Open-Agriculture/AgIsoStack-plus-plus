//================================================================================================
/// @file isobus_virtual_terminal_client_update_helper.hpp
///
/// @brief A helper class to update and track the state of an active working set.
/// @author Daan Steenbergen
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================

#ifndef ISOBUS_VIRTUAL_TERMINAL_CLIENT_UPDATE_HELPER_HPP
#define ISOBUS_VIRTUAL_TERMINAL_CLIENT_UPDATE_HELPER_HPP

#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client_state_tracker.hpp"

namespace isobus
{
	/// @brief A helper class to update and track the state of an active working set.
	class VirtualTerminalClientUpdateHelper : public VirtualTerminalClientStateTracker
	{
	public:
		/// @brief The constructor of class to help update the state of an active working set.
		/// @param[in] client The virtual terminal client that provides the active working set.
		explicit VirtualTerminalClientUpdateHelper(std::shared_ptr<VirtualTerminalClient> client);

		/// @brief The destructor of class to unregister event listeners.
		~VirtualTerminalClientUpdateHelper();

		/// @brief Sets the numeric value of a tracked object.
		/// @param[in] objectId The object id of the numeric value to set.
		/// @param[in] value The value to set the numeric value to.
		/// @return True if the value was set successfully, false otherwise.
		bool set_numeric_value(std::uint16_t objectId, std::uint32_t value);

		/// @brief Increases the numeric value of a tracked object.
		/// @param[in] objectId The object id of the numeric value to increase.
		/// @param[in] step The step size to increase the numeric value with.
		/// @return True if the value was increased successfully, false otherwise.
		bool increase_numeric_value(std::uint16_t objectId, std::uint32_t step = 1);

		/// @brief Decreases the numeric value of a tracked object.
		/// @param[in] objectId The object id of the numeric value to decrease.
		/// @param[in] step The step size to decrease the numeric value with.
		/// @return True if the value was decreased successfully, false otherwise.
		bool decrease_numeric_value(std::uint16_t objectId, std::uint32_t step = 1);

		/// @brief Register a callback function to validate a numeric value change of a tracked object.
		/// If the callback function returns true, the numeric value change will be acknowledged.
		/// Otherwise, if the callback function returns false, the numeric value change will
		/// be rejected by sending the current value back to the VT.
		/// @param[in] callback The callback function to register, or nullptr to unregister.
		void set_callback_validate_numeric_value(const std::function<bool(std::uint16_t, std::uint32_t)> &callback);

		/// @brief Sets the active data/alarm mask.
		/// @param[in] workingSetId The working set to set the active data/alarm mask for.
		/// @param[in] dataOrAlarmMaskId The data/alarm mask to set active.
		/// @return True if the data/alarm mask was set active successfully, false otherwise.
		bool set_active_data_or_alarm_mask(std::uint16_t workingSetId, std::uint16_t dataOrAlarmMaskId);

		/// @brief Sets the active soft key mask.
		/// @param[in] maskType The type of mask to set the active soft key mask for.
		/// @param[in] maskId The mask to set the active soft key mask for.
		/// @param[in] softKeyMaskId The soft key mask to set active.
		/// @return True if the soft key mask was set active successfully, false otherwise.
		bool set_active_soft_key_mask(VirtualTerminalClient::MaskType maskType, std::uint16_t maskId, std::uint16_t softKeyMaskId);

		/// @brief Sets the value of an attribute of a tracked object.
		/// @note If the to be tracked working set consists of more than the master,
		/// this function is incompatible with a VT prior to version 4. For working sets consisting
		/// of only the master, this function is compatible with any VT version.
		/// @param[in] objectId The object id of the attribute to set.
		/// @param[in] attribute The attribute to set.
		/// @param[in] value The value to set the attribute to.
		/// @return True if the attribute was set successfully, false otherwise.
		bool set_attribute(std::uint16_t objectId, std::uint8_t attribute, std::uint32_t value);

		/// @brief Sets the value of a float attribute of a tracked object.
		/// @attention ONLY use this function for float attributes defined in ISO11783-6,
		/// otherwise you will get incorrect results. Scale on output numbers, for example, is a float.
		/// @note If the to be tracked working set consists of more than the master,
		/// this function is incompatible with a VT prior to version 4. For working sets consisting
		/// of only the master, this function is compatible with any VT version.
		/// @param[in] objectId The object id of the attribute to set.
		/// @param[in] attribute The attribute to set.
		/// @param[in] value The value to set the attribute to.
		/// @return True if the attribute was set successfully, false otherwise.
		bool set_attribute(std::uint16_t objectId, std::uint8_t attribute, float value);

	private:
		/// @brief Processes a numeric value change event
		/// @param[in] event The numeric value change event to process.
		void process_numeric_value_change_event(const VirtualTerminalClient::VTChangeNumericValueEvent &event);

		std::shared_ptr<VirtualTerminalClient> vtClient; ///< Holds the vt client.

		std::function<bool(std::uint16_t, std::uint32_t)> callbackValidateNumericValue; ///< Holds the callback function to validate a numeric value change.
		EventCallbackHandle numericValueChangeEventHandle; ///< Holds the handle to the numeric value change event listener
	};
} // namespace isobus

#endif // ISOBUS_VIRTUAL_TERMINAL_CLIENT_UPDATE_HELPER_HPP
