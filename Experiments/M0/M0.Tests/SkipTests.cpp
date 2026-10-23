#include <gtest/gtest.h>
#include <gmock/gmock.h>

import std;

TEST(SkipTests, SkippedWithAReason)
{
    GTEST_SKIP() << "Needs a map file, not available in the M0 experiment.";
}