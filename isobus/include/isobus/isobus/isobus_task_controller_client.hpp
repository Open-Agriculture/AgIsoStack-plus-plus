//================================================================================================
/// @file isobus_task_controller_client.hpp
///
/// @brief A class to manage a client connection to a ISOBUS field computer's task controller
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#ifndef ISOBUS_TASK_CONTROLLER_CLIENT_HPP
#define ISOBUS_TASK_CONTROLLER_CLIENT_HPP

#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/utility/processing_flags.hpp"

namespace isobus
{
	class TaskControllerClient
	{
	public:
		/// @brief The constructor for a TaskControllerClient
		/// @param[in] partner The TC server control function
		/// @param[in] clientSource The internal control function to communicate from
		TaskControllerClient(std::shared_ptr<PartneredControlFunction> partner, std::shared_ptr<InternalControlFunction> clientSource);

	private:
		enum class StateMachineState
		{
			Disconnected,
			WaitForStartUpDelay,
			WaitForServerStatusMessage,
			BeginSendingWorkingSetMaster,
			SendStatusMessage,
			RequestVersion,
			WaitForRequestVersionResponse,
			WaitForRequestVersionFromServer,
			SendRequestVersionResponse,
			RequestLanguage,
			WaitForLanguageResponse,

		};

		bool send_working_set_master() const;

		std::shared_ptr<PartneredControlFunction> partnerControlFunction; ///< The partner control function this client will send to
		std::shared_ptr<InternalControlFunction> myControlFunction; ///< The internal control function the client uses to send from
		std::uint32_t stateMachineTimestamp_ms = 0;
		std::uint32_t controlFunctionValidTimestamp_ms = 0;
		std::uint8_t numberOfWorkingSetMembers = 1;
	};
} // namespace isobus

#endif // ISOBUS_TASK_CONTROLLER_CLIENT_HPP
