#include "can_address_claim_state_machine.hpp"
#include "can_lib_parameter_group_numbers.hpp"
#include "can_network_manager.hpp"
#include "can_types.hpp"
#include "system_timing.hpp"

#include <cassert>
#include <limits>
#include <random>

namespace isobus
{
	AddressClaimStateMachine::AddressClaimStateMachine(std::uint8_t preferredAddressValue, NAME ControlFunctionNAME, std::uint8_t portIndex) :
	  m_isoname(ControlFunctionNAME),
	  m_currentState(State::None),
	  m_timestamp_ms(0),
	  m_portIndex(portIndex),
	  m_preferredAddress(preferredAddressValue),
	  m_enabled(true)
	{
		assert(m_preferredAddress != BROADCAST_CAN_ADDRESS);
		assert(m_preferredAddress != NULL_CAN_ADDRESS);
		assert(portIndex < CAN_PORT_MAXIMUM);
		std::default_random_engine generator;
		std::uniform_int_distribution<std::uint8_t> distribution(0, 255);
		m_randomClaimDelay_ms = distribution(generator) * 0.6f; // Defined by ISO part 5
	}

	AddressClaimStateMachine::State AddressClaimStateMachine::get_current_state() const
	{
		return m_currentState;
	}

	void AddressClaimStateMachine::set_is_enabled(bool value)
	{
		m_enabled = value;
	}

	bool AddressClaimStateMachine::get_enabled() const
	{
		return m_enabled;
	}

	void AddressClaimStateMachine::update()
	{
		if (get_enabled())
		{
			switch (get_current_state())
			{
				case State::None:
				{
					set_current_state(State::WaitForClaim);
				}
				break;

				case State::WaitForClaim:
				{
					if (0 == m_timestamp_ms)
					{
						m_timestamp_ms = SystemTiming::get_timestamp_ms();
					}
					if (SystemTiming::time_expired_ms(m_timestamp_ms, m_randomClaimDelay_ms))
					{
						set_current_state(State::SendRequestForClaim);
					}
				}
				break;

				case State::SendRequestForClaim:
				{
					if (send_request_to_claim())
					{
						set_current_state(State::WaitForRequestContentionPeriod);
					}
				}
				break;

				case State::WaitForRequestContentionPeriod:
				{
					const std::uint32_t addressContentionTime_ms = 250;

					if (SystemTiming::time_expired_ms(m_timestamp_ms, addressContentionTime_ms + m_randomClaimDelay_ms))
					{
						ControlFunction *deviceAtOurPreferredAddress = CANNetworkManager::CANNetwork.get_control_function(m_portIndex, m_preferredAddress, {});
						// Time to find a free address
						if (nullptr == deviceAtOurPreferredAddress)
						{
							// Our address is free. This is the best outcome. Claim it.
							set_current_state(State::SendPreferredAddressClaim);
						}
						else if ((!m_isoname.get_arbitrary_address_capable()) &&
						         (deviceAtOurPreferredAddress->get_NAME().get_full_name() > m_isoname.get_full_name()))
						{
							// Our address is not free, we cannot be at an arbitrary address, and address is contendable
							set_current_state(State::ContendForPreferredAddress);
						}
						else if (!m_isoname.get_arbitrary_address_capable())
						{
							// Can't claim because we cannot tolerate an arbitrary address, and the CF at that spot wins contention
							set_current_state(State::UnableToClaim);
						}
						else
						{
							// We will move to another address if whoever is in our spot has a lower NAME
							if (deviceAtOurPreferredAddress->get_NAME().get_full_name() < m_isoname.get_full_name())
							{
								set_current_state(State::SendArbitraryAddressClaim);
							}
							else
							{
								set_current_state(State::SendPreferredAddressClaim);
							}
						}
					}
				}
				break;

				case State::SendPreferredAddressClaim:
				{
					if (send_address_claim(m_preferredAddress))
					{
						set_current_state(State::AddressClaimingComplete);
					}
					else
					{
						set_current_state(State::None);
					}
				}
				break;

				case State::SendArbitraryAddressClaim:
				{
					// Search the range of generally available addresses
					bool addressFound = false;

					for (std::uint8_t i = 128; i <= 247; i++)
					{
						if ((nullptr == CANNetworkManager::CANNetwork.get_control_function(m_portIndex, i, {})) && (send_address_claim(i)))
						{
							addressFound = true;
							set_current_state(State::AddressClaimingComplete);
							break;
						}
					}

					if (!addressFound)
					{
						set_current_state(State::UnableToClaim);
					}
				}
				break;

				case State::ContendForPreferredAddress:
				{
					// TODO
				}
				break;

				default:
				case State::AddressClaimingComplete:
				case State::UnableToClaim:
				{
				}
				break;
			}
		}
		else
		{
			set_current_state(State::None);
		}
	}

	void AddressClaimStateMachine::set_current_state(State value)
	{
		m_currentState = value;
	}

	bool AddressClaimStateMachine::send_request_to_claim()
	{
		bool retVal = false;

		if (get_enabled())
		{
			const std::uint8_t addressClaimRequestLength = 3;
			const std::uint32_t PGN = static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim);
			std::uint8_t dataBuffer[addressClaimRequestLength];

			dataBuffer[0] = (PGN & std::numeric_limits<std::uint8_t>::max());
			dataBuffer[1] = ((PGN >> 8) & std::numeric_limits<std::uint8_t>::max());
			dataBuffer[2] = ((PGN >> 16) & std::numeric_limits<std::uint8_t>::max());

			retVal = CANNetworkManager::CANNetwork.send_can_message_raw(m_portIndex,
			                                                            NULL_CAN_ADDRESS,
			                                                            BROADCAST_CAN_ADDRESS,
			                                                            static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest),
			                                                            static_cast<std::uint8_t>(CANIdentifier::CANPriority::PriorityDefault6),
			                                                            dataBuffer,
			                                                            3,
			                                                            {});
		}
		return retVal;
	}

	bool AddressClaimStateMachine::send_address_claim(std::uint8_t address)
	{
		bool retVal = false;

		assert(address < BROADCAST_CAN_ADDRESS);

		if (get_enabled())
		{
			std::uint64_t isoNAME = m_isoname.get_full_name();
			std::uint8_t dataBuffer[CAN_DATA_LENGTH];

			dataBuffer[0] = static_cast<uint8_t>(isoNAME);
			dataBuffer[1] = static_cast<uint8_t>(isoNAME >> 8);
			dataBuffer[2] = static_cast<uint8_t>(isoNAME >> 16);
			dataBuffer[3] = static_cast<uint8_t>(isoNAME >> 24);
			dataBuffer[4] = static_cast<uint8_t>(isoNAME >> 32);
			dataBuffer[5] = static_cast<uint8_t>(isoNAME >> 40);
			dataBuffer[6] = static_cast<uint8_t>(isoNAME >> 48);
			dataBuffer[7] = static_cast<uint8_t>(isoNAME >> 56);
			retVal = CANNetworkManager::CANNetwork.send_can_message_raw(m_portIndex,
			                                                            address,
			                                                            BROADCAST_CAN_ADDRESS,
			                                                            static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim),
			                                                            static_cast<std::uint8_t>(CANIdentifier::CANPriority::PriorityDefault6),
			                                                            dataBuffer,
			                                                            CAN_DATA_LENGTH,
			                                                            {});
		}
		return retVal;
	}

} // namespace isobus
