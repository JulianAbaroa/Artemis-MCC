#include <gtest/gtest.h>

import Common.Log.Type;
import Common.Log.System;
import std;

// Characterization of the "[Tag] LEVEL: text" syntax: the expected values are what the log parser produced before M1,
// except ErrorWithNothingAfter (D166).

namespace
{
    using Common::Log::Type::LogLevel;
    using Common::Log::Type::LogMessage;
    using Common::Log::System::LevelPrefix;
    using Common::Log::System::ParseMessage;

    auto Parsed(LogLevel level, std::string tag, std::string message) -> LogMessage
    {
        return LogMessage{ .Level = level, .Tag = std::move(tag), .Message = std::move(message) };
    }
}

TEST(LogParseTests, TagAndInfo)
{
    EXPECT_EQ(ParseMessage("[MapLoader] INFO: Loaded 3 maps."), Parsed(LogLevel::Info, "[MapLoader]", "Loaded 3 maps."));
}

TEST(LogParseTests, TagAndWarning)
{
    EXPECT_EQ(ParseMessage("[Game] WARNING: Event ring moved."), Parsed(LogLevel::Warning, "[Game]", "Event ring moved."));
}

TEST(LogParseTests, TagAndError)
{
    EXPECT_EQ(ParseMessage("[Hook] ERROR: Failed to create the hook."), Parsed(LogLevel::Error, "[Hook]", "Failed to create the hook."));
}

TEST(LogParseTests, TagWithoutLevel)
{
    EXPECT_EQ(ParseMessage("[Bench] run 4 of 60"), Parsed(LogLevel::Default, "[Bench]", "run 4 of 60"));
}

TEST(LogParseTests, LevelWithoutTag)
{
    EXPECT_EQ(ParseMessage("INFO: Started."), Parsed(LogLevel::Info, "", "Started."));
}

TEST(LogParseTests, NeitherTagNorLevel)
{
    EXPECT_EQ(ParseMessage("plain text"), Parsed(LogLevel::Default, "", "plain text"));
}

TEST(LogParseTests, EmptyBody)
{
    EXPECT_EQ(ParseMessage(""), Parsed(LogLevel::Default, "", ""));
}

TEST(LogParseTests, TagWithoutSpaceBeforeLevel)
{
    EXPECT_EQ(ParseMessage("[Map]INFO: x"), Parsed(LogLevel::Info, "[Map]", "x"));
}

TEST(LogParseTests, OnlyOneSpaceAfterTheTagIsRemoved)
{
    EXPECT_EQ(ParseMessage("[Map]  INFO: x"), Parsed(LogLevel::Default, "[Map]", " INFO: x"));
}

TEST(LogParseTests, LevelIsCaseSensitive)
{
    EXPECT_EQ(ParseMessage("[Map] info: x"), Parsed(LogLevel::Default, "[Map]", "info: x"));
}

TEST(LogParseTests, BracketsInsideTheTextAfterATag)
{
    EXPECT_EQ(ParseMessage("[Map] INFO: Loaded [ctf]."), Parsed(LogLevel::Info, "[Map]", "Loaded [ctf]."));
}

// Quirk kept from before M1: the first '[' ... ']' anywhere is the tag and the text before it is dropped.
TEST(LogParseTests, FirstBracketPairAnywhereIsTheTag)
{
    EXPECT_EQ(ParseMessage("Loaded [abc] now"), Parsed(LogLevel::Default, "[abc]", "now"));
}

TEST(LogParseTests, UnclosedBracketIsNotATag)
{
    EXPECT_EQ(ParseMessage("[Unclosed INFO: x"), Parsed(LogLevel::Default, "", "[Unclosed INFO: x"));
}

// Quirk kept from before M1: the level prefix is assumed to be followed by one space, which is dropped with it.
TEST(LogParseTests, LevelWithoutSpaceLosesTheFirstCharacter)
{
    EXPECT_EQ(ParseMessage("[Map] ERROR:xyz"), Parsed(LogLevel::Error, "[Map]", "yz"));
}

// Changed in M1 (D166): before, substr past the end threw std::out_of_range inside the logger.
TEST(LogParseTests, ErrorWithNothingAfter)
{
    EXPECT_EQ(ParseMessage("[Map] ERROR:"), Parsed(LogLevel::Error, "[Map]", ""));
    EXPECT_EQ(ParseMessage("WARNING:"), Parsed(LogLevel::Warning, "", ""));
}

TEST(LogParseTests, LevelPrefixOfEachLevel)
{
    EXPECT_EQ(LevelPrefix(LogLevel::Default), "");
    EXPECT_EQ(LevelPrefix(LogLevel::Info), "INFO: ");
    EXPECT_EQ(LevelPrefix(LogLevel::Warning), "WARNING: ");
    EXPECT_EQ(LevelPrefix(LogLevel::Error), "ERROR: ");
}