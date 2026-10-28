#include <gtest/gtest.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

import Common.Text.System;
import std;

// WideToUtf8 of Common (M5a, D191, D194): portable, and the same bytes the DLL got from WideCharToMultiByte(CP_UTF8) before.
// windows.h is here only as the reference to compare against; the code under test does not use it.

namespace
{
    using Common::Text::System::WideToUtf8;

    // U+FFFD, what WideCharToMultiByte writes for a lone surrogate.
    constexpr std::string_view k_Replacement{ "\xEF\xBF\xBD" };

    // return: What the DLL did before M5a (PlayerTableService::WideToUtf8 after wcsnlen): CP_UTF8, no flags.
    auto Reference(std::u16string_view text) -> std::string
    {
        if (text.empty()) return std::string{};

        const auto* wide{ reinterpret_cast<const wchar_t*>(text.data()) };
        const int length{ static_cast<int>(text.size()) };

        const int size{ WideCharToMultiByte(CP_UTF8, 0, wide, length, nullptr, 0, nullptr, nullptr) };
        std::string out(static_cast<std::size_t>(size), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide, length, out.data(), size, nullptr, nullptr);

        return out;
    }

    auto Convert(std::u16string_view text) -> std::string
    {
        return WideToUtf8(std::span<const char16_t>{ text.data(), text.size() });
    }
}

TEST(WideToUtf8Tests, EmptyOrLeadingZeroGivesNothing)
{
    EXPECT_EQ(WideToUtf8(std::span<const char16_t>{}), "");

    const std::array<char16_t, 4> zeros{};
    EXPECT_EQ(WideToUtf8(zeros), "");
}

TEST(WideToUtf8Tests, AsciiIsCopied)
{
    EXPECT_EQ(Convert(u"Mini-Place 3"), "Mini-Place 3");
}

TEST(WideToUtf8Tests, StopsAtTheFirstZero)
{
    const std::array<char16_t, 8> buffer{ u'A', u'B', 0, u'C', u'D' };
    EXPECT_EQ(WideToUtf8(buffer), "AB");
}

TEST(WideToUtf8Tests, BufferWithoutZeroIsUsedWhole)
{
    // The gamertag field has no room for a terminator when it is full (16 code units).
    const std::array<char16_t, 4> buffer{ u'J', u'u', u'l', u'i' };
    EXPECT_EQ(WideToUtf8(buffer), "Juli");
}

TEST(WideToUtf8Tests, KnownCodePoints)
{
    EXPECT_EQ(Convert(u"é"), "\xC3\xA9");
    EXPECT_EQ(Convert(u"߿"), "\xDF\xBF");
    EXPECT_EQ(Convert(u"ࠀ"), "\xE0\xA0\x80");
    EXPECT_EQ(Convert(u"€"), "\xE2\x82\xAC");
    EXPECT_EQ(Convert(u"￿"), "\xEF\xBF\xBF");
    EXPECT_EQ(Convert(u"\U00010000"), "\xF0\x90\x80\x80");
    EXPECT_EQ(Convert(u"\U0001F600"), "\xF0\x9F\x98\x80");
    EXPECT_EQ(Convert(u"\U0010FFFF"), "\xF4\x8F\xBF\xBF");
}

TEST(WideToUtf8Tests, LoneSurrogatesBecomeTheReplacementCharacter)
{
    const std::string replacement{ k_Replacement };

    EXPECT_EQ(Convert(std::u16string{ char16_t{ 0xD800 } }), replacement);
    EXPECT_EQ(Convert(std::u16string{ char16_t{ 0xDFFF } }), replacement);
    EXPECT_EQ(Convert(std::u16string{ u'A', char16_t{ 0xDBFF } }), "A" + replacement);
    EXPECT_EQ(Convert(std::u16string{ char16_t{ 0xD83D }, u'B' }), replacement + "B");
    // A low surrogate before a high one is not a pair: two lone surrogates.
    EXPECT_EQ(Convert(std::u16string{ char16_t{ 0xDC00 }, char16_t{ 0xD800 } }), replacement + replacement);
}

TEST(WideToUtf8Tests, EveryBmpCodeUnitMatchesTheReference)
{
    std::size_t mismatches{ 0 };
    std::uint32_t first{ 0 };

    for (std::uint32_t unit{ 1 }; unit <= 0xFFFF; ++unit)
    {
        const std::u16string text{ static_cast<char16_t>(unit) };
        if (Convert(text) != Reference(text))
        {
            if (mismatches == 0) first = unit;
            ++mismatches;
        }
    }

    EXPECT_EQ(mismatches, 0u) << "first different code unit: " << std::format("0x{:04X}", first);
}

TEST(WideToUtf8Tests, SurrogatePairsMatchTheReference)
{
    std::size_t mismatches{ 0 };

    for (std::uint32_t high{ 0xD800 }; high <= 0xDBFF; ++high)
    {
        for (const std::uint32_t low : { 0xDC00u, 0xDC01u, 0xDE37u, 0xDFFFu })
        {
            const std::u16string text{ static_cast<char16_t>(high), static_cast<char16_t>(low) };
            if (Convert(text) != Reference(text)) ++mismatches;
        }
    }

    EXPECT_EQ(mismatches, 0u);
}

TEST(WideToUtf8Tests, MixedTextMatchesTheReference)
{
    const std::array<std::u16string, 5> texts{
        u"Julián",
        u"アルテミス",
        u"x\U0001F3AEyñ",
        std::u16string{ u'a', char16_t{ 0xD800 }, u'b', char16_t{ 0xDC00 }, u'c' },
        std::u16string{ char16_t{ 0xD83D }, char16_t{ 0xDE00 }, char16_t{ 0xDE00 } },
    };

    for (const std::u16string& text : texts)
    {
        EXPECT_EQ(Convert(text), Reference(text));
    }
}