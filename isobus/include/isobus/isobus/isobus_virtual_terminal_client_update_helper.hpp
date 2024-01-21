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

	private:
		std::shared_ptr<VirtualTerminalClient> client; ///< Holds the vt client.
	};
}; // namespace isobus

#endif // ISOBUS_VIRTUAL_TERMINAL_CLIENT_UPDATE_HELPER_HPP
