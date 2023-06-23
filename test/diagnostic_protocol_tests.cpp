#include <gtest/gtest.h>

#include "isobus/isobus/isobus_diagnostic_protocol.hpp"

using namespace isobus;

TEST(DIAGNOSTIC_PROTOCOL_TESTS, CreateAndDestroyProtocolObjects)
{
	NAME TestDeviceNAME(0);
	auto TestInternalECU = InternalControlFunction::create(TestDeviceNAME, 0x1C, 0);

	auto diagnosticProtocol = std::make_unique<DiagnosticProtocol>(TestInternalECU);
	EXPECT_NO_THROW(diagnosticProtocol->initialize());

	EXPECT_NO_THROW(diagnosticProtocol->terminate());
	diagnosticProtocol.reset();

	//! @todo try to reduce the reference count, such that that we don't use a control function after it is destroyed
	ASSERT_TRUE(TestInternalECU->destroy(2));
}
