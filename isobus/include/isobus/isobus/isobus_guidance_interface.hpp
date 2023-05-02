//================================================================================================
/// @file isobus_guidance_interface.hpp
///
/// @brief Defines an interface for sending and receiving ISOBUS guidance messages
///
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef ISOBUS_GUIDANCE_INTERFACE_HPP
#define ISOBUS_GUIDANCE_INTERFACE_HPP

#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/utility/event_dispatcher.hpp"
#include "isobus/utility/processing_flags.hpp"

#include <memory>
#include <vector>

namespace isobus
{
	/// @brief An interface for sending and receiving ISOBUS guidance messages
	class GuidanceInterface
	{
	public:
		/// @brief Constructor for a GuidanceInterface
		/// @param[in] sourceControlFunction The internal control function to use when sending messages, or nullptr for listen only
		/// @param[in] destinationControlFunction The destination control function for transmitted messages, or nullptr for broadcasts
		GuidanceInterface(std::shared_ptr<InternalControlFunction> source, std::shared_ptr<ControlFunction> destination);

		/// @brief An interface for sending the agricultural
		/// guidance system command message.
		///
		/// @details This message is sent by an automatic guidance control system to the
		/// machine steering system. It provides steering commands
		/// and serves as heartbeat between guidance system and steering control system.
		class GuidanceSystemCommand
		{
		public:
			/// @brief This parameter indicates whether the guidance system is
			/// attempting to control steering with this command.
			enum class CurvatureCommandStatus : std::uint8_t
			{
				NotIntendedToSteer = 0, ///< Steering Disengaged
				IntendedToSteer = 1, ///< Steering Engaged
				Error = 2,
				NotAvailable = 3
			};

			/// @brief Sets the curvature command status that will be encoded into
			/// the CAN message. This parameter indicates whether the guidance system is
			/// attempting to control steering with this command
			/// @param[in] newStatus The status to encode into the message
			void set_status(CurvatureCommandStatus newStatus);

			/// @brief Returns the curvature command status that was set with set_status
			/// @returns The curvature command status that was set with set_status
			CurvatureCommandStatus get_status() const;

			/// @brief Desired course curvature over ground that a machine's
			/// steering system is required to achieve.
			/// @details The value you set here will be encoded into the
			/// guidance curvature command message.
			///
			/// The desired path is determined by the automatic guidance system expressed
			/// as the inverse of the instantaneous radius of curvature of the turn.
			/// Curvature is positive when the vehicle is moving forward and turning to the driver's right
			///
			/// @param[in] curvature Commanded curvature in km^-1 (inverse kilometers). Range is -8032 to 8031.75 km-1
			void set_curvature(float curvature);

			/// @brief Returns the curvature command that was set with set_curvature
			/// @returns Commanded curvature in km^-1 (inverse kilometers). Range is -8032 to 8031.75 km-1
			float get_curvature() const;

		private:
			float commandedCurvature = 0.0f; ///< The commanded curvature in km^-1 (inverse kilometers)
			CurvatureCommandStatus commandedStatus = CurvatureCommandStatus::NotAvailable; ///< The current status for the command
		};

		/// @brief An interface for sending and receiving the ISOBUS agricultural guidance machine message
		class AgriculturalGuidanceMachineInfo
		{
		public:
			/// @brief State of a lockout switch that allows operators to
			/// disable automatic steering system functions.
			/// @details https://www.isobus.net/isobus/pGNAndSPN/1221?type=SPN
			enum class MechanicalSystemLockout : std::uint8_t
			{
				NotActive = 0,
				Active = 1,
				Error = 2,
				NotAvailable = 3
			};

			/// @brief Sets the estimated course curvature over ground for the machine.
			/// @param[in] curvature The curvature in km^-1 (inverse kilometers). Range is -8032 to 8031.75 km-1
			void set_estimated_curvature(float curvature);

			/// @brief Returns the estimated curvature that was previously set with set_estimated_curvature
			/// @returns The estimated curvature in km^-1 (inverse kilometers). Range is -8032 to 8031.75 km-1
			float get_estimated_curvature() const;

		private:
			float estimatedCurvature = 0.0f; ///< Curvature in km^-1 (inverse kilometers). Range is -8032 to 8031.75 km-1
		};

		void initialize();

		AgriculturalGuidanceMachineInfo AgriculturalGuidanceMachineInfoTransmitData;
		GuidanceSystemCommand GuidanceSystemCommandTransmitData;

		std::size_t get_number_received_guidance_system_command_sources() const;
		std::size_t get_number_received_agricultural_guidance_machine_info_message_sources() const;

		bool send_guidance_curvature_command();

		virtual void update();

	private:
		enum class TransmitFlags : std::uint32_t
		{
			SendGuidanceSystemCommand = 0,
			SendGuidanceMachineInfo,

			NumberOfFlags
		};

		static void process_flags(std::uint32_t flag, void *parentPointer);

		ProcessingFlags txFlags; ///< Tx flag for sending messages periodically
		std::shared_ptr<InternalControlFunction> sourceControlFunction; ///< The control function to use when sending messages
		std::shared_ptr<ControlFunction> destinationControlFunction; ///< The optional destination to which messages will be sent. If nullptr it will be broadcast instead.
		std::vector<AgriculturalGuidanceMachineInfo> receivedAgriculturalGuidanceMachineInfoMessages; ///< A list of all received estimated curvatures
		std::vector<GuidanceSystemCommand> receivedGuidanceSystemCommandMessages; ///< A list of all received curvature commands and statuses
		bool initialized = false; ///< Stores if the interface has been initialized
	};
} // namespace isobus

#endif // ISOBUS_GUIDANCE_HPP