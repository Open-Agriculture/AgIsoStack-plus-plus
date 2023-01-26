//================================================================================================
/// @file can_network_manager.hpp
///
/// @brief The main class that manages the ISOBUS stack including: callbacks, Name to Address
/// management, making control functions, and driving the various protocols.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_NETWORK_MANAGER
#define CAN_NETWORK_MANAGER

#include "isobus/isobus/can_address_claim_state_machine.hpp"
#include "isobus/isobus/can_badge.hpp"
#include "isobus/isobus/can_callbacks.hpp"
#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/can_extended_transport_protocol.hpp"
#include "isobus/isobus/can_frame.hpp"
#include "isobus/isobus/can_identifier.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_message.hpp"
#include "isobus/isobus/can_transport_protocol.hpp"

#include <array>
#include <list>
#include <mutex>

/// @brief This namespace encompases all of the ISO11783 stack's functionality to reduce global namespace pollution
namespace isobus
{
	class PartneredControlFunction;

	//================================================================================================
	/// @class CANNetworkManager
	///
	/// @brief The main CAN network manager object, handles protocol management and updating other
	/// stack components. Provides an interface for sending CAN messages.
	//================================================================================================
	class CANNetworkManager
	{
	public:
		static CANNetworkManager CANNetwork; ///< Static singleton of the one network manager. Use this to access stack functionality.

		/// @brief Initializer function for the network manager
		void initialize();

		/// @brief Called only by the stack, returns a control function based on certain port and address
		/// @param[in] CANPort CAN Channel index of the control function
		/// @param[in] CFAddress Address of the control function
		/// @returns A control function that matches the parameters, or nullptr if no match was found
		ControlFunction *get_control_function(std::uint8_t CANPort, std::uint8_t CFAddress, CANLibBadge<AddressClaimStateMachine>) const;

		/// @brief Called only by the stack, adds a new contorl function with specified parameters
		/// @param[in] CANPort The CAN channel index associated with the control function
		/// @param[in] newControlFunction The new control function to be processed
		/// @param[in] CFAddress The new control function's address
		void add_control_function(std::uint8_t CANPort, ControlFunction *newControlFunction, std::uint8_t CFAddress, CANLibBadge<AddressClaimStateMachine>);

		/// @brief This is how you register a callback for any PGN destined for the global address (0xFF)
		/// @param[in] parameterGroupNumber The PGN you want to register for
		/// @param[in] callback The callback that will be called when parameterGroupNumber is recieved from the global address (0xFF)
		/// @param[in] parent A generic context variable that helps identify what object the callback is destined for. Can be nullptr if you don't want to use it.
		void add_global_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parent);

		/// @brief This is how you remove a callback for any PGN destined for the global address (0xFF)
		/// @param[in] parameterGroupNumber The PGN of the callback to remove
		/// @param[in] callback The callback that will be removed
		/// @param[in] parent A generic context variable that helps identify what object the callback was destined for
		void remove_global_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parent);

		/// @brief Returns the number of global PGN callbacks that have been registered with the network manager
		/// @returns The number of global PGN callbacks that have been registered with the network manager
		std::uint32_t get_number_global_parameter_group_number_callbacks() const;

		/// @brief Registers a callback for ANY control function sending the associated PGN
		/// @param[in] parameterGroupNumber The PGN you want to register for
		/// @param[in] callback The callback that will be called when parameterGroupNumber is recieved from any control function
		/// @param[in] parent A generic context variable that helps identify what object the callback is destined for. Can be nullptr if you don't want to use it.
		void add_any_control_function_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parent);

		/// @brief This is how you remove a callback added with add_any_control_function_parameter_group_number_callback
		/// @param[in] parameterGroupNumber The PGN of the callback to remove
		/// @param[in] callback The callback that will be removed
		/// @param[in] parent A generic context variable that helps identify what object the callback was destined for
		void remove_any_control_function_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parent);

		/// @brief Returns an internal control function if the passed-in control function is an internal type
		/// @returns An internal control function casted from the passed in control function
		InternalControlFunction *get_internal_control_function(ControlFunction *controlFunction);

		/// @brief This is the main way to send a CAN message of any length.
		/// @details This function will automatically choose an appropriate transport protocol if needed.
		/// If you don't specify a destination (or use nullptr) you message will be sent as a broadcast
		/// if it is valid to do so.
		/// You can also get a callback on success or failure of the transmit.
		/// @returns `true` if the message was sent, otherwise `false`
		bool send_can_message(std::uint32_t parameterGroupNumber,
		                      const std::uint8_t *dataBuffer,
		                      std::uint32_t dataLength,
		                      InternalControlFunction *sourceControlFunction,
		                      ControlFunction *destinationControlFunction = nullptr,
		                      CANIdentifier::CANPriority priority = CANIdentifier::CANPriority::PriorityDefault6,
		                      TransmitCompleteCallback txCompleteCallback = nullptr,
		                      void *parentPointer = nullptr,
		                      DataChunkCallback frameChunkCallback = nullptr);

		/// @brief This is the main function used by the stack to receive CAN messages and add them to a queue.
		/// @details This function is called by the stack itself when you call can_lib_process_rx_message.
		/// @param[in] message The message to be received
		void receive_can_message(CANMessage &message);

		/// @brief The main update function for the network manager. Updates all protocols.
		void update();

		/// @brief Process the CAN Rx queue
		/// @param[in] rxFrame Frame to process
		/// @param[in] parentClass A generic context variable
		static void can_lib_process_rx_message(HardwareInterfaceCANFrame &rxFrame, void *parentClass);

		/// @brief Informs the network manager that a partner was deleted so that it can be purged from the address/cf tables
		/// @param[in] partner Pointer to the partner being deleted
		void on_partner_deleted(PartneredControlFunction *partner, CANLibBadge<PartneredControlFunction>);

	protected:
		// Using protected region to allow protocols use of special functions from the network manager
		friend class AddressClaimStateMachine; ///< Allows the network manager to work closely with the address claiming process
		friend class ExtendedTransportProtocolManager; ///< Allows the network manager to access the ETP manager
		friend class TransportProtocolManager; ///< Allows the network manager to work closely with the transport protocol manager object
		friend class DiagnosticProtocol; ///< Allows the diagnostic protocol to access the protected functions on the network manager
		friend class ParameterGroupNumberRequestProtocol; ///< Allows the PGN request protocol to access the network manager protected functions
		friend class FastPacketProtocol; ///< Allows the FP protocol to access the network manager protected functions
		friend class CANLibProtocol;

		/// @brief Adds a PGN callback for a protocol class
		/// @param[in] parameterGroupNumber The PGN to register for
		/// @param[in] callback The callback to call when the PGN is received
		/// @param[in] parentPointer A generic context variable that helps identify what object the callback was destined for
		/// @returns `true` if the callback was added, otherwise `false`
		bool add_protocol_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parentPointer);

		/// @brief Removes a PGN callback for a protocol class
		/// @param[in] parameterGroupNumber The PGN to register for
		/// @param[in] callback The callback to call when the PGN is received
		/// @param[in] parentPointer A generic context variable that helps identify what object the callback was destined for
		/// @returns `true` if the callback was removed, otherwise `false`
		bool remove_protocol_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parentPointer);

		/// @brief Sends a CAN message using raw addresses. Used only by the stack.
		/// @param[in] portIndex The CAN channel index to send the message from
		/// @param[in] sourceAddress The source address to send the CAN message from
		/// @param[in] destAddress The destination address to send the message to
		/// @param[in] parameterGroupNumber The PGN to use when sending the message
		/// @param[in] priority The CAN priority of the message being sent
		/// @param[in] data A pointer to the data buffer to send from
		/// @param[in] size The size of the messgage to send
		/// @returns `true` if the message was sent, otherwise `false`
		bool send_can_message_raw(std::uint32_t portIndex,
		                          std::uint8_t sourceAddress,
		                          std::uint8_t destAddress,
		                          std::uint32_t parameterGroupNumber,
		                          std::uint8_t priority,
		                          const void *data,
		                          std::uint32_t size,
		                          CANLibBadge<AddressClaimStateMachine>);

		/// @brief Processes completed protocol messages. Causes PGN callbacks to trigger.
		/// @param[in] protocolMessage The completed protocol message
		void protocol_message_callback(CANMessage *protocolMessage);

		std::vector<CANLibProtocol *> protocolList; ///< A list of all created protocol classes

	private:
		/// @brief Constructor for the network manager. Sets default values for members
		CANNetworkManager();

		/// @brief Updates the internal address table based on a received CAN message
		/// @param[in] message A message being received by the stack
		void update_address_table(CANMessage &message);

		/// @brief Updates the internal address table based on a received address claim
		/// @param[in] CANPort The CAN channel index of the CAN message being processed
		/// @param[in] claimedAddress The address claimed
		void update_address_table(std::uint8_t CANPort, std::uint8_t claimedAddress);

		/// @brief Creates new control function classes based on the frames coming in from the bus
		/// @param[in] rxFrame Raw frames coming in from the bus
		void update_control_functions(HardwareInterfaceCANFrame &rxFrame);

		/// @brief Checks if new partners have been created and matches them to existing control functions
		void update_new_partners();

		/// @brief Builds a CAN frame from a frame's discrete components
		/// @param[in] portIndex The CAN channel index of the CAN message being processed
		/// @param[in] sourceAddress The source address to send the CAN message from
		/// @param[in] destAddress The destination address to send the message to
		/// @param[in] parameterGroupNumber The PGN to use when sending the message
		/// @param[in] priority The CAN priority of the message being sent
		/// @param[in] data A pointer to the data buffer to send from
		/// @param[in] size The size of the messgage to send
		/// @returns The constucted frame based on the inputs
		HardwareInterfaceCANFrame construct_frame(std::uint32_t portIndex,
		                                          std::uint8_t sourceAddress,
		                                          std::uint8_t destAddress,
		                                          std::uint32_t parameterGroupNumber,
		                                          std::uint8_t priority,
		                                          const void *data,
		                                          std::uint32_t size);

		/// @brief Returns a control function based on a CAN address and channel index
		/// @param[in] CANPort The CAN channel index of the CAN message being processed
		/// @param[in] CFAddress The CAN address associated with a control function
		/// @returns A control function matching the address and CAN port passed in
		ControlFunction *get_control_function(std::uint8_t CANPort, std::uint8_t CFAddress) const;

		/// @brief Gets a message from the Rx Queue.
		/// @note This will only ever get an 8 byte message. Long messages are handled elsewhere.
		/// @returns The can message that was at the front of the buffer
		CANMessage get_next_can_message_from_rx_queue();

		/// @brief Returns the number of messages in the rx queue that need to be processed
		/// @returns The number of messages in the rx queue that need to be processed
		std::size_t get_number_can_messages_in_rx_queue();

		/// @brief Processes a can message for callbacks added with add_any_control_function_parameter_group_number_callback
		/// @param[in] currentMessage The message to process
		void process_any_control_function_pgn_callbacks(CANMessage &currentMessage);

		/// @brief Processes a can message for callbacks added with add_protocol_parameter_group_number_callback
		/// @param[in] currentMessage The message to process
		void process_protocol_pgn_callbacks(CANMessage &currentMessage);

		/// @brief Matches a CAN message to any matching PGN callback, and calls that callback
		/// @param[in] message A pointer to a CAN message to be processed
		void process_can_message_for_global_and_partner_callbacks(CANMessage *message);

		/// @brief Processes the internal receive message queue
		void process_rx_messages();

		/// @brief Sends a CAN message using raw addresses. Used only by the stack.
		/// @param[in] portIndex The CAN channel index to send the message from
		/// @param[in] sourceAddress The source address to send the CAN message from
		/// @param[in] destAddress The destination address to send the message to
		/// @param[in] parameterGroupNumber The PGN to use when sending the message
		/// @param[in] priority The CAN priority of the message being sent
		/// @param[in] data A pointer to the data buffer to send from
		/// @param[in] size The size of the messgage to send
		/// @returns `true` if the message was sent, otherwise `false`
		bool send_can_message_raw(std::uint32_t portIndex,
		                          std::uint8_t sourceAddress,
		                          std::uint8_t destAddress,
		                          std::uint32_t parameterGroupNumber,
		                          std::uint8_t priority,
		                          const void *data,
		                          std::uint32_t size);

		/// @brief Gets a PGN callback for the global address by index
		/// @param[in] index The index of the callback to get
		/// @returns A structure containing the global PGN callback data
		ParameterGroupNumberCallbackData get_global_parameter_group_number_callback(std::uint32_t index) const;

		ExtendedTransportProtocolManager extendedTransportProtocol; ///< Static instance of the protocol manager
		TransportProtocolManager transportProtocol; ///< Static instance of the transport protocol manager

		std::array<std::array<ControlFunction *, 256>, CAN_PORT_MAXIMUM> controlFunctionTable; ///< Table to maintain address to NAME mappings
		std::vector<ControlFunction *> activeControlFunctions; ///< A list of active control function used to track connected devices
		std::vector<ControlFunction *> inactiveControlFunctions; ///< A list of inactive control functions, used to track disconnected devices
		std::list<ParameterGroupNumberCallbackData> protocolPGNCallbacks; ///< A list of PGN callback registered by CAN protocols
		std::list<CANMessage> receiveMessageList; ///< A queue of Rx messages to process
		std::vector<ParameterGroupNumberCallbackData> globalParameterGroupNumberCallbacks; ///< A list of all global PGN callbacks
		std::vector<ParameterGroupNumberCallbackData> anyControlFunctionParameterGroupNumberCallbacks; ///< A list of all global PGN callbacks
		std::mutex receiveMessageMutex; ///< A mutex for receive messages thread safety
		std::mutex protocolPGNCallbacksMutex; ///< A mutex for PGN callback thread safety
		std::mutex anyControlFunctionCallbacksMutex; ///< Mutex to protect the "any CF" callbacks
		std::uint32_t updateTimestamp_ms; ///< Keeps track of the last time the CAN stack was update in milliseconds
		bool initialized; ///< True if the network manager has been initialized by the update function
	};

} // namespace isobus

#endif // CAN_NETWORK_MANAGER
