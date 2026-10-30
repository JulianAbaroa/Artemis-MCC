#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Semantic.Type;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Object.Type;
import Perception.Classifier.Type;
import Tests.Support.Pipeline;
import std;

// B4h H.4: the zone attached to a flag is the capture zone of that flag, also when its tag says hill.

namespace
{
    namespace Subtype = Common::Semantic::Type::Subtype;

    using Common::Semantic::Type::Function;
    using Blam::MapModel::Definitions::Type::Object::ObjectKind;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Perception::Classifier::Type::Classified;
    using Tests::Support::Pipeline::Chain;

    using Common::Engine::Type::k_NoHandle;

    constexpr std::uint32_t k_Flag{ 0x1001 };
    constexpr std::uint32_t k_Zone{ 0x1002 };
    constexpr std::uint32_t k_Crate{ 0x1003 };

    constexpr std::string_view k_FlagTag{ "objects\\weapons\\multiplayer\\flag\\flag" };
    constexpr std::string_view k_HillTag{ "objects\\multi\\models\\mp_hill_beacon\\mp_hill_beacon" };
    constexpr std::string_view k_ReturnAreaTag{ "objects\\multi\\ctf\\ctf_flag_return_area" };

    class ClassifierFlagZoneTests : public ::testing::Test
    {
    protected:
        auto Add(std::uint32_t handle, std::string_view fourCC, std::string_view tag, ObjectKind kind, std::uint32_t parent) -> void
        {
            AliveObject object{};
            object.Handle = handle;
            object.FourCC = fourCC;
            object.TagName = tag;
            object.Profile.ObjectKind = kind;
            object.ParentHandle = parent;
            m_Chain.m_Tables.m_ObjectStore.AddObject(handle, object);
        }

        // Classifies the objects added so far and returns the one of the handle.
        auto Classify(std::uint32_t handle) -> Classified
        {
            m_Chain.m_Tables.m_ObjectStore.Publish();
            m_Chain.m_Environment.m_ClassifierService.UpdateClassification();

            const auto classifieds{ m_Chain.m_Environment.m_ClassifierStore.Acquire() };
            EXPECT_NE(classifieds, nullptr);
            if (!classifieds) return {};

            const auto it{ std::ranges::find(*classifieds, handle, &Classified::Handle) };
            EXPECT_NE(it, classifieds->end());
            return it == classifieds->end() ? Classified{} : *it;
        }

        Chain m_Chain{};
    };
}

TEST_F(ClassifierFlagZoneTests, HillZoneAttachedToAFlagIsAFlagZone)
{
    this->Add(k_Flag, "weap", k_FlagTag, ObjectKind::Weapon, k_NoHandle);
    this->Add(k_Zone, "bloc", k_HillTag, ObjectKind::Crate, k_Flag);

    const Classified zone{ this->Classify(k_Zone) };

    EXPECT_EQ(zone.Function, Function::ObjectiveZone);
    EXPECT_EQ(zone.Subtype, static_cast<std::uint8_t>(Subtype::ObjectiveZone::FlagZone));
    EXPECT_EQ(static_cast<std::uint8_t>(Subtype::ObjectiveZone::FlagZone), 4u);
    EXPECT_EQ(zone.BoundTo, k_Flag);
}

TEST_F(ClassifierFlagZoneTests, HillZoneWithoutParentStaysAHill)
{
    this->Add(k_Zone, "bloc", k_HillTag, ObjectKind::Crate, k_NoHandle);

    const Classified zone{ this->Classify(k_Zone) };

    EXPECT_EQ(zone.Function, Function::ObjectiveZone);
    EXPECT_EQ(zone.Subtype, static_cast<std::uint8_t>(Subtype::ObjectiveZone::Hill));
}

TEST_F(ClassifierFlagZoneTests, HillZoneAttachedToSomethingElseStaysAHill)
{
    this->Add(k_Crate, "bloc", "objects\\multi\\generic\\some_crate", ObjectKind::Crate, k_NoHandle);
    this->Add(k_Zone, "bloc", k_HillTag, ObjectKind::Crate, k_Crate);

    const Classified zone{ this->Classify(k_Zone) };

    EXPECT_EQ(zone.Subtype, static_cast<std::uint8_t>(Subtype::ObjectiveZone::Hill));
}

TEST_F(ClassifierFlagZoneTests, CatalogReturnAreaIsAFlagZoneWithOrWithoutTheFlag)
{
    this->Add(k_Flag, "weap", k_FlagTag, ObjectKind::Weapon, k_NoHandle);
    this->Add(k_Zone, "bloc", k_ReturnAreaTag, ObjectKind::Crate, k_Flag);

    const Classified zone{ this->Classify(k_Zone) };

    EXPECT_EQ(zone.Subtype, static_cast<std::uint8_t>(Subtype::ObjectiveZone::FlagZone));

    // The catalog says the same without a parent: the tag is a return area.
    this->Add(k_Crate, "bloc", k_ReturnAreaTag, ObjectKind::Crate, k_NoHandle);
    EXPECT_EQ(this->Classify(k_Crate).Subtype, static_cast<std::uint8_t>(Subtype::ObjectiveZone::FlagZone));
}

TEST_F(ClassifierFlagZoneTests, TheFlagItselfIsNotChanged)
{
    this->Add(k_Flag, "weap", k_FlagTag, ObjectKind::Weapon, k_NoHandle);
    this->Add(k_Zone, "bloc", k_HillTag, ObjectKind::Crate, k_Flag);

    const Classified flag{ this->Classify(k_Flag) };

    EXPECT_EQ(flag.Function, Function::ObjectiveItem);
    EXPECT_EQ(flag.Subtype, static_cast<std::uint8_t>(Subtype::ObjectiveItem::Flag));
}