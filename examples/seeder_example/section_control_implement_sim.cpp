#include "section_control_implement_sim.hpp"
#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/isobus_standard_data_description_indices.hpp"
#include "isobus/utility/system_timing.hpp"
#include "isobus/utility/to_string.hpp"

#include <cassert>

void SectionControlImplementSimulator::set_number_of_sections(std::uint8_t value)
{
	sectionStates.resize(value);
	switchStates.resize(value);
}

std::uint8_t SectionControlImplementSimulator::get_number_of_sections() const
{
	return sectionStates.size();
}

void SectionControlImplementSimulator::set_section_state(std::uint8_t index, bool value)
{
	if (index < sectionStates.size())
	{
		sectionStates.at(index) = value;
	}
}

bool SectionControlImplementSimulator::get_section_state(std::uint8_t index) const
{
	assert(index < sectionStates.size());
	return sectionStates.at(index);
}

void SectionControlImplementSimulator::set_switch_state(std::uint8_t index, bool value)
{
	if (index < switchStates.size())
	{
		switchStates.at(index) = value;
	}
}

bool SectionControlImplementSimulator::get_switch_state(std::uint8_t index) const
{
	assert(index < switchStates.size());
	return switchStates.at(index);
}

std::uint32_t SectionControlImplementSimulator::get_actual_rate() const
{
	bool anySectionOn = false;

	for (std::uint8_t i = 0; i < get_number_of_sections(); i++)
	{
		if (true == get_section_state(i))
		{
			anySectionOn = true;
			break;
		}
	}
	return targetRate * (anySectionOn ? 1 : 0);
}

void SectionControlImplementSimulator::set_target_rate(std::uint32_t value)
{
	targetRate = value;
}

bool SectionControlImplementSimulator::get_actual_work_state() const
{
	return setpointWorkState;
}

void SectionControlImplementSimulator::set_target_work_state(bool value)
{
	setpointWorkState = value;
}

void SectionControlImplementSimulator::set_is_mode_auto(bool isAuto)
{
	isAutoMode = isAuto;
}

bool SectionControlImplementSimulator::get_is_mode_auto() const
{
	return isAutoMode;
}

std::uint32_t SectionControlImplementSimulator::get_prescription_control_state() const
{
	return static_cast<std::uint32_t>(get_is_mode_auto());
}

std::uint32_t SectionControlImplementSimulator::get_section_control_state() const
{
	return static_cast<std::uint32_t>(get_is_mode_auto());
}

bool SectionControlImplementSimulator::create_ddop(std::shared_ptr<isobus::DeviceDescriptorObjectPool> poolToPopulate, isobus::NAME clientName) const
{
	bool retVal = true;
	std::uint16_t elementCounter = 0;
	constexpr std::int32_t BOOM_WIDTH = 9144; // 30ft expressed in mm
	assert(0 != get_number_of_sections()); // You need at least 1 section for this example
	const std::int32_t SECTION_WIDTH = (BOOM_WIDTH / get_number_of_sections());
	poolToPopulate->clear();

	// English, decimal point, 12 hour time, ddmmyyyy, all units imperial
	constexpr std::array<std::uint8_t, 7> localizationData = { 'e', 'n', 0b01010000, 0x00, 0b01010101, 0b01010101, 0xFF };

	// Make a pool with 1 granular product
	// Set up device
	retVal &= poolToPopulate->add_device("Isobus Seeder", "1.0.0", "123", "IS1.0", localizationData, std::vector<std::uint8_t>(), clientName.get_full_name());
	retVal &= poolToPopulate->add_device_element("Seeder", elementCounter++, 0, isobus::task_controller_object::DeviceElementObject::Type::Device, static_cast<std::uint16_t>(ImplementDDOPObjectIDs::MainDeviceElement));
	retVal &= poolToPopulate->add_device_process_data("Actual Work State", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualWorkState), isobus::task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::DeviceActualWorkState));
	retVal &= poolToPopulate->add_device_process_data("Request Default PD", static_cast<std::uint16_t>(ImplementDDOPObjectIDs::RequestDefaultProcessData), isobus::task_controller_object::Object::NULL_OBJECT_ID, 0, static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::Total), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::RequestDefaultProcessData));
	retVal &= poolToPopulate->add_device_process_data("Total Time", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::EffectiveTotalTime), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::TimePresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet) | static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::Total), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::DeviceTotalTime));
	retVal &= poolToPopulate->add_device_element("Connector", elementCounter++, static_cast<std::uint16_t>(ImplementDDOPObjectIDs::MainDeviceElement), isobus::task_controller_object::DeviceElementObject::Type::Connector, static_cast<std::uint16_t>(ImplementDDOPObjectIDs::Connector));
	retVal &= poolToPopulate->add_device_process_data("Connector X", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetX), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), 0, static_cast<std::uint16_t>(ImplementDDOPObjectIDs::ConnectorXOffset));
	retVal &= poolToPopulate->add_device_process_data("Connector Y", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetY), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), 0, static_cast<std::uint16_t>(ImplementDDOPObjectIDs::ConnectorYOffset));
	retVal &= poolToPopulate->add_device_property("Type", 9, static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ConnectorType), isobus::task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint16_t>(ImplementDDOPObjectIDs::ConnectorType));

	// Set up Boom
	retVal &= poolToPopulate->add_device_element("AgIsoStack Example", elementCounter++, static_cast<std::uint16_t>(ImplementDDOPObjectIDs::MainDeviceElement), isobus::task_controller_object::DeviceElementObject::Type::Function, static_cast<std::uint16_t>(ImplementDDOPObjectIDs::MainBoom));
	retVal &= poolToPopulate->add_device_property("Offset X", 0, static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetX), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::BoomXOffset));
	retVal &= poolToPopulate->add_device_property("Offset Y", 0, static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetY), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::BoomYOffset));
	retVal &= poolToPopulate->add_device_property("Offset Z", 0, static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetZ), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::BoomZOffset));
	retVal &= poolToPopulate->add_device_process_data("Actual Working Width", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualWorkingWidth), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::LongWidthPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::ActualWorkingWidth));
	retVal &= poolToPopulate->add_device_process_data("Setpoint Work State", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointWorkState), isobus::task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet) | static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::SetpointWorkState));
	retVal &= poolToPopulate->add_device_process_data("Area Total", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::TotalArea), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::AreaPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::Total), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::AreaTotal));
	retVal &= poolToPopulate->add_device_process_data("Section Control State", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SectionControlState), isobus::task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet) | static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange) | static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::TimeInterval), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::SectionControlState));

	// Set up bin/tank
	retVal &= poolToPopulate->add_device_element("Product", elementCounter++, 9, isobus::task_controller_object::DeviceElementObject::Type::Bin, static_cast<std::uint16_t>(ImplementDDOPObjectIDs::GranularProduct));
	retVal &= poolToPopulate->add_device_process_data("Bin Capacity", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::MaximumMassContent), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::MassPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange) | static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::TimeInterval), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::BinCapacity));
	retVal &= poolToPopulate->add_device_process_data("Bin Level", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualMassContent), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::MassPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet) | static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange) | static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::TimeInterval), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::BinLevel));
	retVal &= poolToPopulate->add_device_process_data("Lifetime Total Mass", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::LifetimeApplicationTotalVolume), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::MassPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::Total), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::LifetimeApplicationVolumeTotal));
	retVal &= poolToPopulate->add_device_process_data("Rx Control State", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::PrescriptionControlState), isobus::task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet) | static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange) | static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::TimeInterval), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::PrescriptionControlState));
	retVal &= poolToPopulate->add_device_process_data("Target Rate", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCountPerAreaApplicationRate), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::MassPerAreaPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet) | static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::TargetRate));
	retVal &= poolToPopulate->add_device_process_data("Actual Rate", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCountPerAreaApplicationRate), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::MassPerAreaPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange) | static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::TimeInterval), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::ActualRate));
	retVal &= poolToPopulate->add_device_property("Operation Type", 2, static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCulturalPractice), isobus::task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint16_t>(ImplementDDOPObjectIDs::ActualCulturalPractice));

	// Set up sections for section control
	// Using 7 ft sections
	for (std::uint_fast8_t i = 0; i < get_number_of_sections(); i++)
	{
		std::int32_t individualSectionWidth = BOOM_WIDTH / get_number_of_sections();
		retVal &= poolToPopulate->add_device_element("Section " + isobus::to_string(static_cast<int>(i)), elementCounter++, 9, isobus::task_controller_object::DeviceElementObject::Type::Section, static_cast<std::uint16_t>(ImplementDDOPObjectIDs::Section1) + i);
		retVal &= poolToPopulate->add_device_property("Offset X", -20, static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetX), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::LongWidthPresentation), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::Section1XOffset) + i);
		retVal &= poolToPopulate->add_device_property("Offset Y", ((-BOOM_WIDTH) / 2) + (i * SECTION_WIDTH) + (SECTION_WIDTH / 2), static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetY), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::LongWidthPresentation), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::Section1YOffset) + i);
		retVal &= poolToPopulate->add_device_property("Width", individualSectionWidth, static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualWorkingWidth), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::LongWidthPresentation), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::Section1Width) + i);
		auto section = std::static_pointer_cast<isobus::task_controller_object::DeviceElementObject>(poolToPopulate->get_object_by_id(i + static_cast<std::uint16_t>(ImplementDDOPObjectIDs::Section1)));
		section->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::Section1YOffset) + i);
		section->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::Section1XOffset) + i);
		section->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::Section1Width) + i);
	}

	std::uint16_t sectionCounter = 0;
	while (sectionCounter < get_number_of_sections())
	{
		retVal &= poolToPopulate->add_device_process_data("Actual Work State 1-16", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCondensedWorkState1_16) + (sectionCounter / NUMBER_SECTIONS_PER_CONDENSED_MESSAGE), isobus::task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::ActualCondensedWorkingState1To16) + (sectionCounter / NUMBER_SECTIONS_PER_CONDENSED_MESSAGE));
		retVal &= poolToPopulate->add_device_process_data("Setpoint Work State 1-16", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCondensedWorkState1_16) + (sectionCounter / NUMBER_SECTIONS_PER_CONDENSED_MESSAGE), isobus::task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable) | static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(ImplementDDOPObjectIDs::SetpointCondensedWorkingState1To16) + (sectionCounter / NUMBER_SECTIONS_PER_CONDENSED_MESSAGE));
		sectionCounter += NUMBER_SECTIONS_PER_CONDENSED_MESSAGE;
	}

	//// Set up presentations
	retVal &= poolToPopulate->add_device_value_presentation("mm", 0, 1.0f, 0, static_cast<std::uint16_t>(ImplementDDOPObjectIDs::ShortWidthPresentation));
	retVal &= poolToPopulate->add_device_value_presentation("m", 0, 0.001f, 0, static_cast<std::uint16_t>(ImplementDDOPObjectIDs::LongWidthPresentation));
	retVal &= poolToPopulate->add_device_value_presentation("m^2", 0, 1.0f, 0, static_cast<std::uint16_t>(ImplementDDOPObjectIDs::AreaPresentation));
	retVal &= poolToPopulate->add_device_value_presentation("Kg", 0, 0.001f, 0, static_cast<std::uint16_t>(ImplementDDOPObjectIDs::MassPresentation));
	retVal &= poolToPopulate->add_device_value_presentation("minutes", 0, 1.0f, 1, static_cast<std::uint16_t>(ImplementDDOPObjectIDs::TimePresentation));
	retVal &= poolToPopulate->add_device_value_presentation("Kg/ha", 0, 0.001f, 1, static_cast<std::uint16_t>(ImplementDDOPObjectIDs::MassPerAreaPresentation));

	// Add child linkages to device elements if all objects were added OK
	if (retVal)
	{
		auto sprayer = std::static_pointer_cast<isobus::task_controller_object::DeviceElementObject>(poolToPopulate->get_object_by_id(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::MainDeviceElement)));
		auto connector = std::static_pointer_cast<isobus::task_controller_object::DeviceElementObject>(poolToPopulate->get_object_by_id(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::Connector)));
		auto boom = std::static_pointer_cast<isobus::task_controller_object::DeviceElementObject>(poolToPopulate->get_object_by_id(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::MainBoom)));
		auto product = std::static_pointer_cast<isobus::task_controller_object::DeviceElementObject>(poolToPopulate->get_object_by_id(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::GranularProduct)));

		sprayer->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::DeviceActualWorkState));
		sprayer->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::DeviceTotalTime));

		connector->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::ConnectorXOffset));
		connector->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::ConnectorYOffset));
		connector->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::ConnectorType));

		boom->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::BoomXOffset));
		boom->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::BoomYOffset));
		boom->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::BoomZOffset));
		boom->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::ActualWorkingWidth));
		boom->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::SectionControlState));

		sectionCounter = 0;
		while (sectionCounter < get_number_of_sections())
		{
			boom->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::ActualCondensedWorkingState1To16) + (sectionCounter / NUMBER_SECTIONS_PER_CONDENSED_MESSAGE));
			boom->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::SetpointCondensedWorkingState1To16) + (sectionCounter / NUMBER_SECTIONS_PER_CONDENSED_MESSAGE));
			sectionCounter += NUMBER_SECTIONS_PER_CONDENSED_MESSAGE;
		}

		for (std::uint_fast8_t i = 0; i < get_number_of_sections(); i++)
		{
			boom->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::Section1) + i);
		}

		product->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::BinCapacity));
		product->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::BinLevel));
		product->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::LifetimeApplicationVolumeTotal));
		product->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::PrescriptionControlState));
		product->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::ActualCulturalPractice));
		product->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::TargetRate));
		product->add_reference_to_child_object(static_cast<std::uint16_t>(ImplementDDOPObjectIDs::ActualRate));
	}
	return retVal;
}

bool SectionControlImplementSimulator::request_value_command_callback(std::uint16_t,
                                                                      std::uint16_t DDI,
                                                                      std::uint32_t &value,
                                                                      void *parentPointer)
{
	if (nullptr != parentPointer)
	{
		auto sim = reinterpret_cast<SectionControlImplementSimulator *>(parentPointer);
		switch (DDI)
		{
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::MaximumVolumeContent):
			{
				value = 4542494; // 1200 Gallons in ml
			}
			break;

			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualVolumeContent):
			{
				value = 3785000; // 1000 Gal in ml;
			}
			break;

			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SectionControlState):
			{
				value = sim->get_section_control_state();
			}
			break;

			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::PrescriptionControlState):
			{
				value = sim->get_prescription_control_state();
			}
			break;

			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCondensedWorkState1_16):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCondensedWorkState17_32):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCondensedWorkState33_48):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCondensedWorkState49_64):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCondensedWorkState65_80):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCondensedWorkState81_96):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCondensedWorkState97_112):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCondensedWorkState113_128):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCondensedWorkState129_144):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCondensedWorkState145_160):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCondensedWorkState161_176):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCondensedWorkState177_192):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCondensedWorkState193_208):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCondensedWorkState209_224):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCondensedWorkState225_240):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCondensedWorkState241_256):
			{
				std::uint16_t sectionIndexOffset = NUMBER_SECTIONS_PER_CONDENSED_MESSAGE * (DDI - static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualCondensedWorkState1_16));
				value = 0;

				for (std::uint_fast8_t i = 0; i < NUMBER_SECTIONS_PER_CONDENSED_MESSAGE; i++)
				{
					if ((i + (sectionIndexOffset)) < sim->get_number_of_sections())
					{
						value |= ((true == sim->get_section_state(i + sectionIndexOffset)) ? static_cast<std::uint32_t>(0x01) : static_cast<std::uint32_t>(0x00)) << (2 * i);
					}
					else
					{
						value |= (static_cast<std::uint32_t>(0x03) << (2 * i));
					}
				}
			}
			break;

			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualVolumePerAreaApplicationRate):
			{
				value = sim->get_actual_rate();
			}
			break;

			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ActualWorkState):
			{
				value = sim->get_actual_work_state();
			}
			break;

			default:
			{
				value = 0;
			}
			break;
		}
	}
	// The actual use of the return value here is for the TC client to know if it needs to keep calling more callbacks to search
	// for one that can satisfy the element number + DDI combination it needs.
	// But in the example this is the only value command callback, so we always want to return true.
	return true;
}

bool SectionControlImplementSimulator::value_command_callback(std::uint16_t,
                                                              std::uint16_t DDI,
                                                              std::uint32_t processVariableValue,
                                                              void *parentPointer)
{
	if (nullptr != parentPointer)
	{
		auto sim = reinterpret_cast<SectionControlImplementSimulator *>(parentPointer);
		switch (DDI)
		{
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCondensedWorkState1_16):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCondensedWorkState17_32):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCondensedWorkState33_48):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCondensedWorkState49_64):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCondensedWorkState65_80):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCondensedWorkState81_96):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCondensedWorkState97_112):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCondensedWorkState113_128):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCondensedWorkState129_144):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCondensedWorkState145_160):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCondensedWorkState161_176):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCondensedWorkState177_192):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCondensedWorkState193_208):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCondensedWorkState209_224):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCondensedWorkState225_240):
			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCondensedWorkState241_256):
			{
				std::uint16_t sectionIndexOffset = NUMBER_SECTIONS_PER_CONDENSED_MESSAGE * (DDI - static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointCondensedWorkState1_16));

				for (std::uint_fast8_t i = 0; i < NUMBER_SECTIONS_PER_CONDENSED_MESSAGE; i++)
				{
					sim->set_section_state(sectionIndexOffset + i, (0x01 == (processVariableValue >> (2 * i) & 0x03)));
				}
			}
			break;

			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointVolumePerAreaApplicationRate):
			{
				sim->set_target_rate(processVariableValue);
			}
			break;

			case static_cast<std::uint16_t>(isobus::DataDescriptionIndex::SetpointWorkState):
			{
				sim->set_target_work_state(processVariableValue);
			}
			break;

			default:
			{
			}
			break;
		}
	}
	// The actual use of the return value here is for the TC client to know if it needs to keep calling more callbacks to search
	// for one that can satisfy the element number + DDI combination it needs.
	// But in the example this is the only value command callback, so we always want to return true.
	return true;
}
