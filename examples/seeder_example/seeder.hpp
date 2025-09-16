//================================================================================================
/// @file seeder.hpp
///
/// @brief This is the definition of an example seeder application
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#ifndef SEEDER_HPP
#define SEEDER_HPP

#include "isobus/isobus/isobus_diagnostic_protocol.hpp"
#include "vt_application.hpp"

class Seeder
{
public:
	Seeder() = default;

	bool initialize(const std::string &interfaceName = "");

	void terminate();

	void update();

private:
	std::unique_ptr<SeederVtApplication> VTApplication = nullptr;
	std::unique_ptr<isobus::DiagnosticProtocol> diagnosticProtocol = nullptr;
};

#endif // SEEDER_HPP
