#pragma once

#include "can_NAME.hpp"

namespace isobus
{
    
class AddressClaimStateMachine
{
public:
    enum class State
    {
        None,
        WaitForClaim,
        SendRequestForClaim,
        WaitForRequestContentionPeriod,
        SendPreferredAddressClaim,
        ContendForPreferredAddress,
        SendArbitraryAddressClaim,
        UnableToClaim,
        AddressClaimingComplete
    };

    AddressClaimStateMachine(std::uint8_t preferredAddressValue, NAME ControlFunctionNAME, std::uint8_t portIndex);

    State get_current_state() const;

    void set_is_enabled(bool value);
    bool get_enabled() const;

    void update();
private:
    void set_current_state(State value);
    bool send_request_to_claim();
    bool send_address_claim(std::uint8_t address);

    NAME m_isoname;
    State m_currentState;
    std::uint32_t m_timestamp_ms;
    std::uint8_t m_portIndex;
    std::uint8_t m_preferredAddress;
    std::uint8_t m_randomClaimDelay_ms;
    bool m_enabled;
};

} // namespace isobus
