#pragma once

#include "can_identifier.hpp"
#include "can_internal_control_function.hpp"
#include "can_lib_badge.hpp"
#include "can_address_claim_state_machine.hpp"
#include "can_message.hpp"
#include "can_frame.hpp"
#include "can_types.hpp"
#include "can_lib_callbacks.hpp"

#include <array>
#include <mutex>

namespace isobus
{
    
class CANNetworkManager
{
public:
    static CANNetworkManager CANNetwork;

    void initialize();

    ControlFunction *get_control_function(std::uint8_t CANPort, std::uint8_t CFAddress, CANLibBadge<AddressClaimStateMachine>) const;
    void add_control_function(std::uint8_t CANPort, ControlFunction *newControlFunction, std::uint8_t CFAddress, CANLibBadge<AddressClaimStateMachine>);

    void add_global_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback);
    void remove_global_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback);

    std::uint32_t get_number_global_parameter_group_number_callbacks() const;

    InternalControlFunction *get_internal_control_function(ControlFunction *controlFunction);

    bool send_can_message(std::uint32_t parameterGroupNumber,
                          const std::uint8_t *dataBuffer,
                          std::uint32_t dataLength,
                          InternalControlFunction *sourceControlFunction,
                          ControlFunction *destinationControlFunction = nullptr,
                          CANIdentifier::CANPriority priority = CANIdentifier::CANPriority::PriorityDefault6);

    void receive_can_message(CANMessage message);

    void update();

    static void can_lib_process_rx_message(HardwareInterfaceCANFrame &rxFrame, void *parentClass);

protected:
    // Using protected region to allow protocols use of special functions from the network manager
    friend class AddressClaimStateMachine;
    friend class TransportProtocolManager;
    bool add_protocol_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parentPointer);
    bool remove_protocol_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parentPointer);
    bool send_can_message_raw(std::uint32_t portIndex,
                              std::uint8_t sourceAddress,
                              std::uint8_t destAddress,
                              std::uint32_t parameterGroupNumber,
                              std::uint8_t priority,
                              const void *data,
                              std::uint32_t size,
                              CANLibBadge<AddressClaimStateMachine>);

private:
    struct CANLibProtocolPGNCallbackInfo
    {
        bool operator==(const CANLibProtocolPGNCallbackInfo& obj);
        CANLibCallback callback;
        void *parent;
        std::uint32_t parameterGroupNumber;
    };

    void update_address_table(CANMessage &message);
    void update_control_functions(HardwareInterfaceCANFrame &rxFrame);

    HardwareInterfaceCANFrame construct_frame(std::uint32_t portIndex,
                              std::uint8_t sourceAddress,
                              std::uint8_t destAddress,
                              std::uint32_t parameterGroupNumber,
                              std::uint8_t priority,
                              const void *data,
                              std::uint32_t size);
    ControlFunction *get_control_function(std::uint8_t CANPort, std::uint8_t CFAddress) const;
    void process_rx_messages();
    bool send_can_message_raw(std::uint32_t portIndex,
                              std::uint8_t sourceAddress,
                              std::uint8_t destAddress,
                              std::uint32_t parameterGroupNumber,
                              std::uint8_t priority,
                              const void *data,
                              std::uint32_t size);
    ParameterGroupNumberCallbackData get_global_parameter_group_number_callback(std::uint32_t index) const;
                          
    std::array<std::array<ControlFunction*, 256>, CAN_PORT_MAXIMUM> controlFunctionTable;
    std::vector<ControlFunction *> activeControlFunctions;
    std::vector<ControlFunction *> inactiveControlFunctions;
    std::list<CANLibProtocolPGNCallbackInfo> protocolPGNCallbacks;
    std::list<CANMessage> receiveMessageList;
    std::vector<ParameterGroupNumberCallbackData> globalParameterGroupNumberCallbacks;
    std::mutex receiveMessageMutex;
    std::mutex protocolPGNCallbacksMutex;
    std::uint32_t updateTimestamp_ms;
    bool initialized;
};

} // namespace isobus
