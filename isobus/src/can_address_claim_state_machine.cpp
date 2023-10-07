//================================================================================================
/// @file can_address_claim_state_machine.cpp
///
/// @brief Defines a class for managing the address claiming process
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/can_address_claim_state_machine.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"

#include <cassert>
#include <limits>
#include <random>

namespace isobus
{
	AddressClaimStateMachine::AddressClaimStateMachine(std::uint8_t preferredAddressValue, NAME ControlFunctionNAME, std::uint8_t portIndex) :
	  m_isoname(ControlFunctionNAME),
	  m_portIndex(portIndex),
	  m_preferredAddress(preferredAddressValue)
	{
		assert(m_preferredAddress != BROADCAST_CAN_ADDRESS);
		assert(m_preferredAddress != NULL_CAN_ADDRESS);
		assert(portIndex < CAN_PORT_MAXIMUM);
		std::default_random_engine generator;
		std::uniform_int_distribution<unsigned int> distribution(0, 255);
		m_randomClaimDelay_ms = static_cast<std::uint8_t>(distribution(generator) * 0.6f); // Defined by ISO part 5
		CANNetworkManager::CANNetwork.add_global_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest), process_rx_message, this);
		CANNetworkManager::CANNetwork.add_global_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim), process_rx_message, this);
	}

	AddressClaimStateMachine ::~AddressClaimStateMachine()
	{
		CANNetworkManager::CANNetwork.remove_global_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest), process_rx_message, this);
		CANNetworkManager::CANNetwork.remove_global_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim), process_rx_message, this);
	}

	AddressClaimStateMachine::State AddressClaimStateMachine::get_current_state() const
	{
		return m_currentState;
	}

	void AddressClaimStateMachine::on_address_violation()
	{
		if (State::AddressClaimingComplete == get_current_state())
		{
			CANStackLogger::warn("[AC]: Address violation for address %u",
			                     get_claimed_address());

			set_current_state(State::SendReclaimAddressOnRequest);
		}
	}

	void AddressClaimStateMachine::process_commanded_address(std::uint8_t commandedAddress)
	{
		if (State::AddressClaimingComplete == get_current_state())
		{
			if (!m_isoname.get_arbitrary_address_capable())
			{
				CANStackLogger::error("[AC]: Our address was commanded to a new value, but our ISO NAME doesn't support changing our address.");
			}
			else
			{
				std::shared_ptr<ControlFunction> deviceAtOurPreferredAddress = CANNetworkManager::CANNetwork.get_control_function(m_portIndex, commandedAddress, {});
				m_preferredAddress = commandedAddress;

				if (nullptr == deviceAtOurPreferredAddress)
				{
					// Commanded address is free. We'll claim it.
					set_current_state(State::SendPreferredAddressClaim);
					CANStackLogger::info("[AC]: Our address was commanded to a new value of %u", commandedAddress);
				}
				else if (deviceAtOurPreferredAddress->get_NAME().get_full_name() < m_isoname.get_full_name())
				{
					// We can steal the address of the device at our commanded address and force it to move
					set_current_state(State::SendArbitraryAddressClaim);
					CANStackLogger::info("[AC]: Our address was commanded to a new value of %u, and an ECU at the target address is being evicted.", commandedAddress);
				}
				else
				{
					CANStackLogger::error("[AC]: Our address was commanded to a new value of %u, but we cannot move to the target address.", commandedAddress);
				}
			}
		}
	}

	void AddressClaimStateMachine::set_is_enabled(bool value)
	{
		m_enabled = value;
	}

	bool AddressClaimStateMachine::get_enabled() const
	{
		return m_enabled;
	}

	std::uint8_t AddressClaimStateMachine::get_claimed_address() const
	{
		return m_claimedAddress;
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
						std::shared_ptr<ControlFunction> deviceAtOurPreferredAddress = CANNetworkManager::CANNetwork.get_control_function(m_portIndex, m_preferredAddress, {});
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
						CANStackLogger::debug("[AC]: Internal control function %016llx has claimed address %u on channel %u",
						                      m_isoname.get_full_name(),
						                      m_preferredAddress,
						                      m_portIndex);
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
							CANStackLogger::debug("[AC]: Internal control function %016llx could not use the preferred address, but has claimed address %u on channel %u",
							                      m_isoname.get_full_name(),
							                      i,
							                      m_portIndex);
							set_current_state(State::AddressClaimingComplete);
							break;
						}
					}

					if (!addressFound)
					{
						CANStackLogger::critical("[AC]: Internal control function %016llx failed to claim an address on channel %u",
						                         m_isoname.get_full_name(),
						                         m_portIndex);
						set_current_state(State::UnableToClaim);
					}
				}
				break;

				case State::SendReclaimAddressOnRequest:
				{
					if (send_address_claim(m_claimedAddress))
					{
						set_current_state(State::AddressClaimingComplete);
					}
				}
				break;

				case State::ContendForPreferredAddress:
				{
					/// @todo Non-arbitratable address contention (there is not a good reason to use this, but we should add support anyways)
				}
				break;

				default:
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

	void AddressClaimStateMachine::process_rx_message(const CANMessage &message, void *parentPointer)
	{
		if (nullptr != parentPointer)
		{
			AddressClaimStateMachine *parent = reinterpret_cast<AddressClaimStateMachine *>(parentPointer);

			if ((message.get_can_port_index() == parent->m_portIndex) &&
			    (parent->get_enabled()))
			{
				switch (message.get_identifier().get_parameter_group_number())
				{
					case static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest):
					{
						const auto &messageData = message.get_data();
						std::uint32_t requestedPGN = messageData.at(0);
						requestedPGN |= (static_cast<std::uint32_t>(messageData.at(1)) << 8);
						requestedPGN |= (static_cast<std::uint32_t>(messageData.at(2)) << 16);

						if ((static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim) == requestedPGN) &&
						    (State::AddressClaimingComplete == parent->get_current_state()))
						{
							parent->set_current_state(State::SendReclaimAddressOnRequest);
						}
					}
					break;

					case static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim):
					{
						if (parent->m_claimedAddress == message.get_identifier().get_source_address())
						{
							const auto &messageData = message.get_data();
							std::uint64_t NAMEClaimed = messageData.at(0);
							NAMEClaimed |= (static_cast<uint64_t>(messageData.at(1)) << 8);
							NAMEClaimed |= (static_cast<uint64_t>(messageData.at(2)) << 16);
							NAMEClaimed |= (static_cast<uint64_t>(messageData.at(3)) << 24);
							NAMEClaimed |= (static_cast<uint64_t>(messageData.at(4)) << 32);
							NAMEClaimed |= (static_cast<uint64_t>(messageData.at(5)) << 40);
							NAMEClaimed |= (static_cast<uint64_t>(messageData.at(6)) << 48);
							NAMEClaimed |= (static_cast<uint64_t>(messageData.at(7)) << 56);

							// Check to see if another ECU is hijacking our address
							// This is not really a needed check, as we can be pretty sure that our address
							// has been stolen if we're running this logic. But, you never know, someone could be
							// spoofing us I guess, or we could be getting an echo? CAN Bridge from another channel?
							// Seemed safest to just confirm.
							if (NAMEClaimed != parent->m_isoname.get_full_name())
							{
								// Wait for things to shake out a bit, then claim a new address.
								parent->set_current_state(State::WaitForRequestContentionPeriod);
								parent->m_claimedAddress = NULL_CAN_ADDRESS;
								CANStackLogger::warn("[AC]: Internal control function %016llx on channel %u must re-arbitrate its address because it was stolen by another ECU with NAME %016llx.",
								                     parent->m_isoname.get_full_name(),
								                     parent->m_portIndex,
								                     NAMEClaimed);
							}
						}
					}
					break;

					default:
					{
					}
					break;
				}
			}
		}
	}

	void AddressClaimStateMachine::set_current_state(State value)
	{
		m_currentState = value;
	}

	bool AddressClaimStateMachine::send_request_to_claim() const
	{
		bool retVal = false;

		if (get_enabled())
		{
			const std::uint8_t addressClaimRequestLength = 3;
			const auto PGN = static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim);
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
			if (retVal)
			{
				m_claimedAddress = address;
			}
		}
		return retVal;
	}

} // namespace isobus
