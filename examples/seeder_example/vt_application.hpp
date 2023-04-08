//================================================================================================
/// @file vt_application.hpp
///
/// @brief This is the definition of the VT portion of the seeder example
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef VT_APPLICATION_HPP
#define VT_APPLICATION_HPP

#include "isobus/isobus/isobus_virtual_terminal_client.hpp"

class SeederVtApplication
{
public:
	enum class UpdateVTStateFlags : std::uint32_t
	{
		UpdateActiveDataMask = 0,

		section1EnableState_ObjPtr,
		section2EnableState_ObjPtr,
		section3EnableState_ObjPtr,
		section4EnableState_ObjPtr,
		section5EnableState_ObjPtr,
		section6EnableState_ObjPtr,
		selectedStatisticsContainer_ObjPtr,

		statisticsSelection_VarNum,
		canAddress_VarNum,
		busload_VarNum,

		NumberOfFlags
	};

	SeederVtApplication(std::shared_ptr<isobus::PartneredControlFunction> VTPartner, std::shared_ptr<isobus::InternalControlFunction> source);

	bool Initialize();

	static void handle_vt_key_events(const isobus::VirtualTerminalClient::VTKeyEvent &event);

	isobus::VirtualTerminalClient VTClientInterface;

	void Update();

private:
	static void processFlags(std::uint32_t flag, void *parentPointer);

	isobus::ProcessingFlags txFlags;
};

#endif // VT_APPLICATION_HPP
