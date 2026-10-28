#include <gtest/gtest.h>

import Resolved.Definitions.Type;
import std;

// The raw kind id of an entry of the object table to ObjectKind (M4a, D186): the conversion DatumService did before.

namespace
{
    using Resolved::Definitions::Type::Object::ObjectKind;
    using Resolved::Definitions::Type::Object::ToObjectKind;
}

TEST(ObjectKindTest, EngineIdsMapToTheirKind)
{
    EXPECT_EQ(ToObjectKind(0x00), ObjectKind::Biped);
    EXPECT_EQ(ToObjectKind(0x01), ObjectKind::Vehicle);
    EXPECT_EQ(ToObjectKind(0x03), ObjectKind::Equipment);
    EXPECT_EQ(ToObjectKind(0x0A), ObjectKind::Crate);
    EXPECT_EQ(ToObjectKind(0x0D), ObjectKind::EffectScenery);
    for (std::uint8_t id = 0; id <= 0x0D; ++id)
    {
        EXPECT_EQ(static_cast<std::uint16_t>(ToObjectKind(id)), id);
    }
}

TEST(ObjectKindTest, UnknownIdsAreInvalid)
{
    EXPECT_EQ(ToObjectKind(0x0E), ObjectKind::Invalid);
    EXPECT_EQ(ToObjectKind(0x7F), ObjectKind::Invalid);
    EXPECT_EQ(ToObjectKind(0xFF), ObjectKind::Invalid);
}