#include <gtest/gtest.h>

#include "lockout_feedback.h"
#include "mcp23017_output_safety.h"

namespace
{
TEST(MCP23017OutputSafetyTest, AllOffUsesElectricalOffLevelForEveryOutputPort)
{
    const std::uint8_t logicalOff = 0x00;
    EXPECT_EQ(0x00, hardware::relay_output_byte(logicalOff, false));
    EXPECT_EQ(0xFF, hardware::relay_output_byte(logicalOff, true));
    EXPECT_EQ(0x00, hardware::relay_output_byte(logicalOff, false)); // MCP1 port A
    EXPECT_EQ(0x00, hardware::relay_output_byte(logicalOff, false)); // MCP1 port B
    EXPECT_EQ(0x00, hardware::relay_output_byte(logicalOff, false)); // MCP2 port A
    EXPECT_EQ(0xFF, hardware::relay_output_byte(logicalOff, true));  // MCP1 port A
    EXPECT_EQ(0xFF, hardware::relay_output_byte(logicalOff, true));  // MCP1 port B
    EXPECT_EQ(0xFF, hardware::relay_output_byte(logicalOff, true));  // MCP2 port A
}

TEST(MCP23017OutputSafetyTest, EnergizedBitsMapBitByBitForBothPolarities)
{
    for (unsigned int bit = 0; bit < 8; ++bit)
    {
        const std::uint8_t logicalOn = static_cast<std::uint8_t>(1U << bit);
        EXPECT_EQ(logicalOn, hardware::relay_output_byte(logicalOn, false));
        EXPECT_EQ(static_cast<std::uint8_t>(~logicalOn), hardware::relay_output_byte(logicalOn, true));
    }
}

TEST(MCP23017OutputSafetyTest, BothBoardsAndHealthyWritesDoNotCreateFault)
{
    hardware::OutputBoardHealth health;
    health.record_probe(0, true);
    health.record_probe(1, true);
    health.record_write(true);
    health.record_write(true);
    EXPECT_TRUE(health.board_found(0));
    EXPECT_TRUE(health.board_found(1));
    EXPECT_FALSE(health.has_fault());
    EXPECT_EQ(ui::InhibitReason::None, ui::classify_inhibit(health.has_fault()));
}

TEST(MCP23017OutputSafetyTest, EitherMissingBoardSelectsOutputBoardLockout)
{
    for (std::size_t missingBoard = 0; missingBoard < 2; ++missingBoard)
    {
        hardware::OutputBoardHealth health;
        health.record_probe(0, missingBoard != 0);
        health.record_probe(1, missingBoard != 1);
        EXPECT_TRUE(health.has_fault());
        EXPECT_EQ(ui::InhibitReason::OutputBoardMissing, ui::classify_inhibit(health.has_fault()));
        EXPECT_STREQ("LOCKED: OUTPUT BOARD MISSING",
                     ui::lockout_message(ui::InhibitReason::OutputBoardMissing));
    }
}

TEST(MCP23017OutputSafetyTest, FailedOutputWriteLatchesTheOutputBoardLockout)
{
    hardware::OutputBoardHealth health;
    health.record_probe(0, true);
    health.record_probe(1, true);
    health.record_write_failure();
    EXPECT_TRUE(health.has_fault());
    EXPECT_EQ(ui::InhibitReason::OutputBoardMissing, ui::classify_inhibit(health.has_fault()));
}

TEST(MCP23017OutputSafetyTest, LaterHealthyProbeDoesNotClearLatchedFault)
{
    hardware::OutputBoardHealth health;
    health.record_probe(0, false);
    health.record_probe(0, true);
    health.record_probe(1, true);
    EXPECT_TRUE(health.board_found(0));
    EXPECT_TRUE(health.has_fault());
}
} // namespace
