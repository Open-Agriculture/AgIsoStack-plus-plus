//================================================================================================
/// @file isobus_virtual_terminal_client_state_tracker.hpp
///
/// @brief A helper class to track the state of an active working set.
/// @author Daan Steenbergen
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================

#ifndef ISOBUS_VIRTUAL_TERMINAL_CLIENT_STATE_TRACKER_HPP
#define ISOBUS_VIRTUAL_TERMINAL_CLIENT_STATE_TRACKER_HPP

#include "isobus/isobus/can_control_function.hpp"
#include "isobus/isobus/can_message.hpp"

#include <map>
#include <vector>

namespace isobus
{
	/// @brief A helper class to update and track the state of an active working set.
	/// @details The state is from the client's perspective. It might not be the same
	/// as the state of the server, but tries to be as close as possible.
	class VirtualTerminalClientStateTracker
	{
	public:
		/// @brief The constructor to track the state of an active working set provided by a client.
		/// @param[in] client The control function of the client. May be external.
		explicit VirtualTerminalClientStateTracker(std::shared_ptr<ControlFunction> client);

		/// @brief The destructor.
		~VirtualTerminalClientStateTracker();

		/// @brief Initializes the state tracker.
		void initialize();

		/// @brief Terminate the state tracker.
		void terminate();

		//! TODO: void initialize_with_defaults(ObjectPool &objectPool);

		/// @brief Adds a numeric value to track.
		/// @param[in] objectId The object id of the numeric value to track.
		/// @param[in] initialValue The initial value of the numeric value to track.
		void add_tracked_numeric_value(std::uint16_t objectId, std::uint32_t initialValue = 0);

		/// @brief Removes a numeric value from tracking.
		/// @param[in] objectId The object id of the numeric value to remove from tracking.
		void remove_tracked_numeric_value(std::uint16_t objectId);

		/// @brief Gets the current numeric value of a tracked object.
		/// @param[in] objectId The object id of the numeric value to get.
		/// @return The current numeric value of the tracked object.
		std::uint32_t get_numeric_value(std::uint16_t objectId) const;

	protected:
		std::shared_ptr<ControlFunction> client; ///< The control function of the virtual terminal client to track.

		//! TODO: std::map<std::uint16_t, bool> shownStates; ///< Holds the 'hide/show' state of tracked objects.
		//! TODO: std::map<std::uint16_t, bool> enabledStates; ///< Holds the 'enable/disable' state of tracked objects.
		//! TODO: std::map<std::uint16_t, bool> selectedStates; ///< Holds the 'selected for input' state of tracked objects.
		//! TODO: add current audio signal state
		//! TODO: std::uint8_t audioVolumeState; ///< Holds the current audio volume.
		//! TODO: std::map<std::uint16_t, std::pair<std::uint16_t, std::uint16_t>> positionStates; ///< Holds the 'position (x,y)' state of tracked objects.
		//! TODO: std::map<std::uint16_t, std::pair<std::uint16_t, std::uint16_t>> sizeStates; ///< Holds the 'size (width,height)' state of tracked objects.
		//! TODO: std::map<std::uint16_t, std::uint8_t> backgroundColourStates; ///< Holds the 'background colour' state of tracked objects.
		std::map<std::uint16_t, std::uint32_t> numericValueStates; ///< Holds the 'numeric value' state of tracked objects.
		//! TODO: std::map<std::uint16_t, std::string> stringValueStates; ///< Holds the 'string value' state of tracked objects.
		//! TODO: std::map<std::uint16_t, std::uint8_t> endPointStates; ///< Holds the 'end point' state of tracked objects.
		//! TODO: add font attribute state
		//! TODO: add line attribute state
		//! TODO: add fill attribute state
		std::uint16_t currentActiveMask; ///< Holds the currently active mask.
		//! TODO: std::uint16_t currentWorkingSet; ///< Holds the working set of the current active mask.
		//! TODO: std::map<std::uint16_t, std::uint16_t> softKeyMasks; ///< Holds the data/alarms masks with their associated soft keys masks for tracked objects.
		//! TODO: std::map<std::uint16_t, std::pair<std::uint8_t, std::uint32_t>> attributeStates; ///< Holds the 'attribute' state of tracked objects.
		//! TODO: std::map<std::uint16_t, std::uint8_t> alarmMaskPrioritiesStates; ///< Holds the 'alarm mask priority' state of tracked objects.
		//! TODO: std::map<std::uint16_t, std::pair<std::uint8_t, std::uint16_t>> listItemStates; ///< Holds the 'list item' state of tracked objects.
		//! TODO: add lock/unlock mask state
		//! TODO: add object label state
		//! TODO: add polygon point state
		//! TODO: add polygon scale state
		//! TODO: add graphics context state
		//! TODO: std::uint16_t currentColourMap; ///< Holds the current colour map/palette object.

	private:
		/// @brief Processes a received message.
		/// @param[in] message The received message.
		/// @param[in] parentPointer The pointer to the parent object, which should be the VirtualTerminalClientStateTracker.
		static void process_rx_message(const CANMessage &message, void *parentPointer);

		/// @brief Processes a received message.
		/// @param[in] message The received message.
		void process_rx_message(const CANMessage &message);
	};
} // namespace isobus

#endif // ISOBUS_VIRTUAL_TERMINAL_CLIENT_STATE_TRACKER_HPP
