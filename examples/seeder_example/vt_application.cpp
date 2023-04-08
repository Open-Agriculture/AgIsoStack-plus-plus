//================================================================================================
/// @file vt_application.cpp
///
/// @brief This is the implementation of the VT portion of the seeder example
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#include "vt_application.hpp"

#include "isobus/utility/iop_file_interface.hpp"

#include <cassert>
#include <iostream>

SeederVtApplication::SeederVtApplication(std::shared_ptr<isobus::PartneredControlFunction> VTPartner, std::shared_ptr<isobus::InternalControlFunction> source) :
  VTClientInterface(VTPartner, source),
  txFlags(static_cast<std::uint32_t>(UpdateVTStateFlags::NumberOfFlags), processFlags, this)
{
}

bool SeederVtApplication::Initialize()
{
	std::vector<std::uint8_t> objectPool = isobus::IOPFileInterface::read_iop_file("BasePool.iop");

	if (objectPool.empty())
	{
		std::cout << "Failed to load object pool from BasePool.iop" << std::endl;
		return false;
	}
	std::cout << "Loaded object pool from BasePool.iop" << std::endl;

	// Generate a unique version string for this object pool (this is optional, and is entirely application specific behavior)
	std::string objectPoolHash = isobus::IOPFileInterface::hash_object_pool_to_version(objectPool);

	VTClientInterface.set_object_pool(0, isobus::VirtualTerminalClient::VTVersion::Version3, objectPool.data(), objectPool.size(), objectPoolHash);
	auto softKeyListener = VTClientInterface.add_vt_soft_key_event_listener(handle_vt_key_events);
	auto buttonListener = VTClientInterface.add_vt_button_event_listener(handle_vt_key_events);
	VTClientInterface.initialize(true);

	return true;
}

void SeederVtApplication::handle_vt_key_events(const isobus::VirtualTerminalClient::VTKeyEvent &event)
{
	auto application = reinterpret_cast<SeederVtApplication *>(event.parentPointer);

	assert(nullptr != application);

	switch (event.keyEvent)
	{
		case isobus::VirtualTerminalClient::KeyActivationCode::ButtonUnlatchedOrReleased:
		{
			switch (event.objectID)
			{
			}
		}
		break;
	}
}

void SeederVtApplication::Update()
{
	txFlags.process_all_flags();
}

void SeederVtApplication::processFlags(std::uint32_t flag, void *parentPointer)
{
	auto currentFlag = static_cast<UpdateVTStateFlags>(flag);
	auto seeder = reinterpret_cast<SeederVtApplication *>(parentPointer);
	bool transmitSuccessful = false;

	assert(nullptr != seeder);

	switch (currentFlag)
	{
		case UpdateVTStateFlags::UpdateActiveDataMask:
		case UpdateVTStateFlags::section1EnableState_ObjPtr:
		case UpdateVTStateFlags::section2EnableState_ObjPtr:
		case UpdateVTStateFlags::section3EnableState_ObjPtr:
		case UpdateVTStateFlags::section4EnableState_ObjPtr:
		case UpdateVTStateFlags::section5EnableState_ObjPtr:
		case UpdateVTStateFlags::section6EnableState_ObjPtr:
		case UpdateVTStateFlags::selectedStatisticsContainer_ObjPtr:
		case UpdateVTStateFlags::statisticsSelection_VarNum:
		case UpdateVTStateFlags::canAddress_VarNum:
		{
		}
		break;

		case UpdateVTStateFlags::busload_VarNum:
		{
			//seeder->VTClientInterface.send_change_numeric_value()
		}
		break;
	}
}
