export module Relations.Classifier.Type;

import Common.Engine.Type;
import Common.Semantic.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Common::Semantic::Type::Function;
    using Common::Semantic::Type::Group;
    using Common::Semantic::Type::Binding;
    using Common::Semantic::Type::Capability;
    using Resolved::Definitions::Type::Object::ObjectKind;

    using Common::Engine::Type::k_NoHandle;
}

export namespace Relations::Classifier::Type
{
    // Rule that decided the function of a tag (D19).
    enum class Source : std::uint8_t
    {
        // No rule matched. The function is Unknown.
        None = 0,

        // Hand label of Data/Tags/ObjectTags.xml.
        Catalog,

        // Multiplayer object kind of the tag.
        MultiplayerKind,

        // Default of the object kind.
        KindDefault,
    };

    // Classification shared by every object of one tag. Function and subtype never change with the state of the object (M2).
    struct TagClassification
    {
        ::Function Function{ ::Function::Unknown };

        // Value of the subtype enum of the function (Subtype::<Function>). 0 is None.
        std::uint8_t Subtype{};

        Type::Source Source{ Type::Source::None };

        // Function the multiplayer object kind of the tag gives, Unknown if none. Kept to audit the catalog against the engine.
        ::Function MultiplayerFunction{ ::Function::Unknown };

        // Capabilities that follow from the tag and the function only.
        Capability Capabilities{ Capability::None };
    };

    // Classification of one live object.
    struct Classified
    {
        std::uint32_t Handle{};

        ::ObjectKind Kind{ ::ObjectKind::Invalid };
        ::Function Function{ ::Function::Unknown };
        std::uint8_t Subtype{};
        ::Group Group{ ::Group::Unknown };
        Type::Source Source{ Type::Source::None };

        // Link to the parent and the parent handle, or 0xFFFFFFFF when Free.
        ::Binding Binding{ ::Binding::Free };
        std::uint32_t BoundTo{ k_NoHandle };

        // Static capabilities of the tag, plus HasZone from the multiplayer block of the object (cylinder or box shape only).
        // note: Environment Entities adds the ones that depend on the object (IsDynamic, IsPickable, IsDestructible).
        Capability Capabilities{ Capability::None };
    };

    // Classification of every live object.
    struct Classifieds : std::vector<Classified>
    {
        using vector::vector;
    };

    // Classification of every tag, by tag name.
    struct TagClassifications : std::unordered_map<std::string, TagClassification>
    {
        using unordered_map::unordered_map;
    };
}