#include <gtest/gtest.h>

import DevTools.Workbench.Skip.Type;
import DevTools.Workbench.Skip.System;
import std;

// The decision of the bench on a skip request (REC.2): a line "<scenario id> <match number>" applies only to the running match.

namespace
{
    using DevTools::Workbench::Skip::System::DecideSkip;
    using DevTools::Workbench::Skip::Type::SkipDecision;
}

TEST(SkipRequestTests, ARequestForTheRunningMatchApplies)
{
    EXPECT_EQ(DecideSkip("20261126-101500-ab12 2", "20261126-101500-ab12", 2), SkipDecision::Apply);
    EXPECT_EQ(DecideSkip("  s1\t1 \r\n", "s1", 1), SkipDecision::Apply);
}

TEST(SkipRequestTests, ARequestForAnotherScenarioOrMatchIsStale)
{
    EXPECT_EQ(DecideSkip("s1 2", "s2", 2), SkipDecision::Stale);
    EXPECT_EQ(DecideSkip("s1 2", "s1", 3), SkipDecision::Stale);
    EXPECT_EQ(DecideSkip("s1 2", "s1", 1), SkipDecision::Stale);
}

TEST(SkipRequestTests, GarbageIsInvalid)
{
    EXPECT_EQ(DecideSkip("", "s1", 1), SkipDecision::Invalid);
    EXPECT_EQ(DecideSkip("   \r\n", "s1", 1), SkipDecision::Invalid);
    EXPECT_EQ(DecideSkip("s1", "s1", 1), SkipDecision::Invalid);
    EXPECT_EQ(DecideSkip("s1 x", "s1", 1), SkipDecision::Invalid);
    EXPECT_EQ(DecideSkip("s1 1x", "s1", 1), SkipDecision::Invalid);
    EXPECT_EQ(DecideSkip("s1 0", "s1", 1), SkipDecision::Invalid);
    EXPECT_EQ(DecideSkip("s1 -1", "s1", 1), SkipDecision::Invalid);
    EXPECT_EQ(DecideSkip("s1 1 extra", "s1", 1), SkipDecision::Invalid);
    EXPECT_EQ(DecideSkip("s1 99999999999999999999999", "s1", 1), SkipDecision::Invalid);
}