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

	private:
		/// @brief Processes a numeric value change event
		/// @param[in] event The numeric value change event to process.
		void process_numeric_value_change_event(const VirtualTerminalClient::VTChangeNumericValueEvent &event);

		std::shared_ptr<VirtualTerminalClient> vtClient; ///< Holds the vt client.

		std::function<bool(std::uint16_t, std::uint32_t)> callbackValidateNumericValue; ///< Holds the callback function to validate a numeric value change.
		std::shared_ptr<void> numericValueChangeEventHandle; ///< Holds the handle to the numeric value change event listener
	};
}; // namespace isobus

#endif // ISOBUS_VIRTUAL_TERMINAL_CLIENT_UPDATE_HELPER_HPP
