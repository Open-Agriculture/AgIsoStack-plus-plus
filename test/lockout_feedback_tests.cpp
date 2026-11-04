#include <gtest/gtest.h>

#include "lockout_feedback.h"

#include <cstring>

namespace
{
using namespace ui;

constexpr std::size_t INSTRUCTION_MAX_CHARS = 240 / 8; // Str_*Instruction: 240 px, Fit8x12
constexpr std::size_t REASON_MAX_CHARS = 240 / 8; // Str_*LockReason: 240 px, Fit8x12

TEST(LockoutFeedbackTest, ClassifiesOnlyOutputBoardFaults)
{
    EXPECT_EQ(InhibitReason::None, classify_inhibit(false));
    EXPECT_EQ(InhibitReason::OutputBoardMissing, classify_inhibit(true));
}

TEST(LockoutFeedbackTest, MessageSelection)
{
    EXPECT_STREQ("LOCKED: OUTPUT BOARD MISSING", lockout_message(InhibitReason::OutputBoardMissing));
    EXPECT_EQ(nullptr, lockout_message(InhibitReason::None));
}

TEST(LockoutFeedbackTest, ReasonTextNamesOutputBoardOnly)
{
    EXPECT_STREQ("REASON: OUTPUT BOARD", lockout_reason_text(InhibitReason::OutputBoardMissing));
    EXPECT_STREQ(" ", lockout_reason_text(InhibitReason::None));
}

TEST(LockoutFeedbackTest, TextsFitTheirObjectWidths)
{
    const InhibitReason all[] = { InhibitReason::None, InhibitReason::OutputBoardMissing };
    for (InhibitReason reason : all)
    {
        const char *message = lockout_message(reason);
        if (nullptr != message)
        {
            EXPECT_LE(std::strlen(message), INSTRUCTION_MAX_CHARS);
        }
        EXPECT_LE(std::strlen(lockout_reason_text(reason)), REASON_MAX_CHARS);
    }
    EXPECT_LE(std::strlen(LOCKOUT_RESTORE_TEXT), INSTRUCTION_MAX_CHARS);
}

TEST(LockoutFeedbackTest, ReasonChangeIsReportedOnceOnly)
{
    LockoutFeedback feedback;
    EXPECT_FALSE(feedback.update(InhibitReason::None).reasonChanged);
    EXPECT_TRUE(feedback.update(InhibitReason::OutputBoardMissing).reasonChanged);
    for (int i = 0; i < 10; i++)
    {
        EXPECT_FALSE(feedback.update(InhibitReason::OutputBoardMissing).reasonChanged);
    }
    EXPECT_TRUE(feedback.update(InhibitReason::None).reasonChanged);
    EXPECT_FALSE(feedback.update(InhibitReason::None).reasonChanged);
}

TEST(LockoutFeedbackTest, NothingShownUntilAPressIsBlocked)
{
    LockoutFeedback feedback;
    LockoutFeedback::Update update = feedback.update(InhibitReason::None);
    EXPECT_FALSE(update.refreshUnfold || update.refreshFold || update.restoreUnfold || update.restoreFold);
    update = feedback.update(InhibitReason::None);
    EXPECT_FALSE(update.restoreUnfold || update.restoreFold);
}

TEST(LockoutFeedbackTest, BlockedPressShowsOnlyThatScreenAndClearsWhenInhibitEnds)
{
    LockoutFeedback feedback;
    feedback.update(InhibitReason::OutputBoardMissing);
    EXPECT_TRUE(feedback.on_blocked_press(ActionGroup::Unfold, InhibitReason::OutputBoardMissing));
    EXPECT_TRUE(feedback.shown(ActionGroup::Unfold));
    EXPECT_FALSE(feedback.shown(ActionGroup::Fold));

    LockoutFeedback::Update update = feedback.update(InhibitReason::OutputBoardMissing);
    EXPECT_FALSE(update.restoreUnfold);

    update = feedback.update(InhibitReason::None);
    EXPECT_TRUE(update.restoreUnfold);
    EXPECT_FALSE(update.restoreFold);
    EXPECT_FALSE(feedback.shown(ActionGroup::Unfold));

    // The restore is reported once only
    update = feedback.update(InhibitReason::None);
    EXPECT_FALSE(update.restoreUnfold);
}

TEST(LockoutFeedbackTest, OutputBoardFaultShowsLockoutForBothActionScreens)
{
    LockoutFeedback feedback;
    const LockoutFeedback::Update update = feedback.update(InhibitReason::OutputBoardMissing);
    EXPECT_TRUE(update.reasonChanged);
    EXPECT_TRUE(update.refreshUnfold);
    EXPECT_TRUE(update.refreshFold);
    EXPECT_TRUE(feedback.shown(ActionGroup::Unfold));
    EXPECT_TRUE(feedback.shown(ActionGroup::Fold));
}

TEST(LockoutFeedbackTest, PressWithoutInhibitShowsNothing)
{
    LockoutFeedback feedback;
    EXPECT_FALSE(feedback.on_blocked_press(ActionGroup::Unfold, InhibitReason::None));
    EXPECT_FALSE(feedback.shown(ActionGroup::Unfold));
}

TEST(LockoutFeedbackTest, BlockedPressNeverSelectsAnActionButton)
{
    // The lockout model is independent of ActionSelection, so a blocked press
    // cannot light a button.
    ActionSelection selection;
    LockoutFeedback feedback;
    feedback.update(InhibitReason::OutputBoardMissing);
    feedback.on_blocked_press(ActionGroup::Unfold, InhibitReason::OutputBoardMissing);
    EXPECT_EQ(0, selection.active_index());
}
} // namespace
