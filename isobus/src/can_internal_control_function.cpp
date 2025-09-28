//================================================================================================
/// @file can_internal_control_function.cpp
///
/// @brief A representation of an ISOBUS ECU that we can send from. Use this class
/// when defining your own control functions that will claim an address within your program.
/// @author Adrian Del Grosso
/// @author Daan Steenbergen
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/can_internal_control_function.hpp"

#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/can_parameter_group_number_request_protocol.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"

#include <algorithm>
#include <cassert>
#include <random>

namespace isobus
{
	InternalControlFunction::InternalControlFunction(NAME desiredName, std::uint8_t preferredAddress, std::uint8_t CANPort) :
	  ControlFunction(desiredName, NULL_CAN_ADDRESS, CANPort, Type::Internal),
	  preferredAddress(preferredAddress)
	{
		assert(preferredAddress != BROADCAST_CAN_ADDRESS);
		assert(CANPort < CAN_PORT_MAXIMUM);

		if (NULL_CAN_ADDRESS == preferredAddress)
		{
			// If we don't have a preferred address, your NAME must be arbitrary address capable!
			assert(desiredName.get_arbitrary_address_capable());
		}

		std::default_random_engine generator;
		std::uniform_int_distribution<unsigned short> distribution(0, 255);
		randomClaimDelay_ms = static_cast<std::uint8_t>(distribution(generator) * 0.6); // Defined by ISO part 5
	}

	InternalControlFunction::State InternalControlFunction::get_current_state() const
	{
		return state;
	}

	void InternalControlFunction::set_current_state(State value)
	{
		stateChangeTimestamp_ms = SystemTiming::get_timestamp_ms();
		state = value;
	}

	void InternalControlFunction::process_rx_message_for_address_claiming(const CANMessage &message)
	{
		if (message.get_can_port_index() != canPortIndex)
		{
			return;
		}

		switch (message.get_identifier().get_parameter_group_number())
		{
			case static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest):
			{
				std::uint32_t requestedPGN = message.get_uint24_at(0);

				if ((static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim) == requestedPGN) &&
				    ((CANIdentifier::GLOBAL_ADDRESS == message.get_identifier().get_destination_address()) ||
				     ((get_address_valid()) &&
				      (get_address() == message.get_identifier().get_destination_address()))) &&
				    (State::AddressClaimingComplete == get_current_state()))
				{
					set_current_state(State::SendReclaimAddressOnRequest);
				}
			}
			break;

			case static_cast<std::uint32_t>(CANLibParameterGroupNumber::CommandedAddress):
			{
				constexpr std::uint8_t COMMANDED_ADDRESS_LENGTH = 9;

				if ((nullptr == message.get_destination_control_function()) &&
				    (COMMANDED_ADDRESS_LENGTH == message.get_data_length()) &&
				    (message.get_can_port_index() == get_can_port()))
				{
					std::uint64_t targetNAME = message.get_uint64_at(0);
					if (get_NAME().get_full_name() == targetNAME)
					{
						process_commanded_address(message.get_uint8_at(8));
					}
				}
			}

			default:
				break;
		}
	}

	bool InternalControlFunction::update_address_claiming()
	{
		bool hasClaimedAddress = false;
		switch (get_current_state())
		{
			case State::None:
			{
				set_current_state(State::WaitForClaim);
			}
			break;

			case State::WaitForClaim:
			{
				if (SystemTiming::time_expired_ms(stateChangeTimestamp_ms, randomClaimDelay_ms))
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
				if (SystemTiming::time_expired_ms(stateChangeTimestamp_ms, ADDRESS_CONTENTION_TIME_MS))
				{
					std::shared_ptr<ControlFunction> deviceAtOurPreferredAddress;
					if (NULL_CAN_ADDRESS != preferredAddress)
					{
						deviceAtOurPreferredAddress = CANNetworkManager::CANNetwork.get_control_function(get_can_port(), preferredAddress);
					}

					// Time to find a free address
					if ((nullptr == deviceAtOurPreferredAddress) && (NULL_CAN_ADDRESS != preferredAddress))
					{
						// Our address is free. This is the best outcome. Claim it.
						set_current_state(State::SendPreferredAddressClaim);
					}
					else if (get_NAME().get_arbitrary_address_capable())
					{
						// Our address is not free, but we can tolerate an arbitrary address
						set_current_state(State::SendArbitraryAddressClaim);
					}
					else if ((!get_NAME().get_arbitrary_address_capable()) &&
					         (nullptr != deviceAtOurPreferredAddress) &&
					         (deviceAtOurPreferredAddress->get_NAME().get_full_name() > get_NAME().get_full_name()))
					{
						// Our address is not free, we cannot be at an arbitrary address, but address is contendable
						set_current_state(State::ContendForPreferredAddress);
					}
					else
					{
						// Can't claim because we cannot tolerate an arbitrary address, and the CF at that spot wins contention
						set_current_state(State::UnableToClaim);
						LOG_ERROR("[AC]: Internal control function %016llx failed to claim its preferred address %u on channel %u, as it cannot tolerate for an arbitrary address and there is already a CF at the preferred address that wins contention.",
						          get_NAME().get_full_name(),
						          preferredAddress,
						          get_can_port());
						send_cannot_claim_source_address();
					}
				}
			}
			break;

			case State::SendPreferredAddressClaim:
			{
				if (send_address_claim(preferredAddress))
				{
					LOG_DEBUG("[AC]: Internal control function %016llx has claimed address %u on channel %u",
					          get_NAME().get_full_name(),
					          get_preferred_address(),
					          get_can_port());
					hasClaimedAddress = true;
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
				// Search the range of available dynamic addresses based on industry group
				// Ref: https://www.isobus.net/isobus/sourceAddress
				static constexpr std::uint8_t START_ADDRESS = 128;
				std::uint8_t endAddress = START_ADDRESS;
				switch (static_cast<NAME::IndustryGroup>(get_NAME().get_industry_group()))
				{
					case NAME::IndustryGroup::Global:
						endAddress = 247;
						break;
					case NAME::IndustryGroup::OnHighwayEquipment:
						endAddress = 158;
						break;
					case NAME::IndustryGroup::AgriculturalAndForestryEquipment:
						endAddress = 235;
						break;
					case NAME::IndustryGroup::ConstructionEquipment:
					case NAME::IndustryGroup::Marine:
					case NAME::IndustryGroup::IndustrialOrProcessControl:
						endAddress = 207;
						break;

					default:
						break;
				}

				for (std::uint8_t i = START_ADDRESS; i <= endAddress; i++)
				{
					if ((nullptr == CANNetworkManager::CANNetwork.get_control_function(get_can_port(), i)) && (send_address_claim(i)))
					{
						hasClaimedAddress = true;

						if (NULL_CAN_ADDRESS == get_preferred_address())
						{
							preferredAddress = i;
							LOG_DEBUG("[AC]: Internal control function %016llx has arbitrarily claimed address %u on channel %u",
							          get_NAME().get_full_name(),
							          i,
							          get_can_port());
						}
						else
						{
							LOG_DEBUG("[AC]: Internal control function %016llx could not use the preferred address, but has arbitrarily claimed address %u on channel %u",
							          get_NAME().get_full_name(),
							          i,
							          get_can_port());
						}
						set_current_state(State::AddressClaimingComplete);
						break;
					}
				}

				if (!hasClaimedAddress)
				{
					LOG_CRITICAL("[AC]: Internal control function %016llx failed to claim an address on channel %u",
					             get_NAME().get_full_name(),
					             get_can_port());

					set_current_state(State::UnableToClaim);
					send_cannot_claim_source_address();
				}
			}
			break;

			case State::SendReclaimAddressOnRequest:
			{
				if (send_address_claim(get_address()))
				{
					hasClaimedAddress = true;
					set_current_state(State::AddressClaimingComplete);
				}
			}
			break;

			case State::ContendForPreferredAddress:
			{
				if (send_address_claim(preferredAddress))
				{
					LOG_DEBUG("[AC]: Internal control function %016llx has won address contention and claimed address %u on channel %u",
					          get_NAME().get_full_name(),
					          get_preferred_address(),
					          get_can_port());
					hasClaimedAddress = true;
					set_current_state(State::AddressClaimingComplete);
				}
				else
				{
					set_current_state(State::None); // Bus went down? Restart.
				}
			}
			break;

			default:
				break;
		}
		return hasClaimedAddress;
	}

	std::uint8_t InternalControlFunction::get_preferred_address() const
	{
		return preferredAddress;
	}

	EventDispatcher<std::uint8_t> &InternalControlFunction::get_address_claimed_event_dispatcher()
	{
		return addressClaimedDispatcher;
	}

	std::weak_ptr<ParameterGroupNumberRequestProtocol> InternalControlFunction::get_pgn_request_protocol() const
	{
		return pgnRequestProtocol;
	}

	bool InternalControlFunction::send_request_to_claim() const
	{
		const auto parameterGroupNumber = static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim);
		static const std::array<std::uint8_t, 3> dataBuffer{
			static_cast<std::uint8_t>(parameterGroupNumber),
			static_cast<std::uint8_t>(parameterGroupNumber >> 8),
			static_cast<std::uint8_t>(parameterGroupNumber >> 16)
		};

		return CANNetworkManager::CANNetwork.send_can_message_raw(canPortIndex,
		                                                          NULL_CAN_ADDRESS,
		                                                          BROADCAST_CAN_ADDRESS,
		                                                          static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest),
		                                                          static_cast<std::uint8_t>(CANIdentifier::CANPriority::PriorityDefault6),
		                                                          dataBuffer.data(),
		                                                          dataBuffer.size(),
		                                                          {});
	}

	bool InternalControlFunction::send_address_claim(std::uint8_t addressToClaim)
	{
		assert(addressToClaim < BROADCAST_CAN_ADDRESS);

		std::uint64_t isoNAME = controlFunctionNAME.get_full_name();
		std::array<std::uint8_t, CAN_DATA_LENGTH> dataBuffer{
			static_cast<std::uint8_t>(isoNAME),
			static_cast<std::uint8_t>(isoNAME >> 8),
			static_cast<std::uint8_t>(isoNAME >> 16),
			static_cast<std::uint8_t>(isoNAME >> 24),
			static_cast<std::uint8_t>(isoNAME >> 32),
			static_cast<std::uint8_t>(isoNAME >> 40),
			static_cast<std::uint8_t>(isoNAME >> 48),
			static_cast<std::uint8_t>(isoNAME >> 56)
		};
		bool retVal = CANNetworkManager::CANNetwork.send_can_message_raw(canPortIndex,
		                                                                 addressToClaim,
		                                                                 BROADCAST_CAN_ADDRESS,
		                                                                 static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim),
		                                                                 static_cast<std::uint8_t>(CANIdentifier::CANPriority::PriorityDefault6),
		                                                                 dataBuffer.data(),
		                                                                 dataBuffer.size(),
		                                                                 {});
		if (retVal)
		{
			address = addressToClaim;
		}
		return retVal;
	}

	bool InternalControlFunction::send_cannot_claim_source_address()
	{
		return send_address_claim(0xFE);
	}

	void InternalControlFunction::process_commanded_address(std::uint8_t commandedAddress)
	{
		if (State::AddressClaimingComplete == get_current_state())
		{
			if (!controlFunctionNAME.get_arbitrary_address_capable())
			{
				LOG_ERROR("[AC]: Our address was commanded to a new value, but our ISO NAME doesn't support changing our address.");
			}
			else
			{
				std::shared_ptr<ControlFunction> deviceAtOurPreferredAddress = CANNetworkManager::CANNetwork.get_control_function(canPortIndex, commandedAddress);
				preferredAddress = commandedAddress;

				if (nullptr == deviceAtOurPreferredAddress)
				{
					// Commanded address is free. We'll claim it.
					set_current_state(State::SendPreferredAddressClaim);
					LOG_INFO("[AC]: Our address was commanded to a new value of %u", commandedAddress);
				}
				else if (deviceAtOurPreferredAddress->get_NAME().get_full_name() < controlFunctionNAME.get_full_name())
				{
					// We can steal the address of the device at our commanded address and force it to move
					set_current_state(State::SendArbitraryAddressClaim);
					LOG_INFO("[AC]: Our address was commanded to a new value of %u, and an ECU at the target address is being evicted.", commandedAddress);
				}
				else
				{
					// We can't steal the address of the device at our commanded address, so we'll just ignore the command
					// and log an error.
					LOG_ERROR("[AC]: Our address was commanded to a new value of %u, but we cannot move to the target address.", commandedAddress);
				}
			}
		}
		else
		{
			LOG_WARNING("[AC]: Our address was commanded to a new value, but we are not in a state to change our address.");
		}
	}

	bool InternalControlFunction::process_rx_message_for_address_violation(const CANMessage &message)
	{
		auto sourceAddress = message.get_identifier().get_source_address();

		if ((BROADCAST_CAN_ADDRESS != sourceAddress) &&
		    (NULL_CAN_ADDRESS != sourceAddress) &&
		    (get_address() == sourceAddress) &&
		    (message.get_can_port_index() == get_can_port()) &&
		    (State::AddressClaimingComplete == get_current_state()))
		{
			LOG_WARNING("[AC]: Address violation for address %u", get_address());

			set_current_state(State::SendReclaimAddressOnRequest);
			return true;
		}
		return false;
	}
} // namespace isobus
