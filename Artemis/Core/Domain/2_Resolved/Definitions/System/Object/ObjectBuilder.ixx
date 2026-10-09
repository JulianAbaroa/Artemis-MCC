export module Resolved.Definitions.System:Object;

import Map.Reader.System;
import Resolved.Definitions.Type;
import std;

export namespace Resolved::Definitions::System
{
    // Builds the data every object family shares.
    class ObjectBuilder
    {
    private:
        using Object = Resolved::Definitions::Type::Object::Object;
        using MultiplayerObject = Resolved::Definitions::Type::Object::MultiplayerObject;
        using ObjectKind = Resolved::Definitions::Type::Object::ObjectKind;
        using MultiplayerObjectKind = Resolved::Definitions::Type::Object::MultiplayerObjectKind;
        using TagResolverService = Map::Reader::System::TagResolverService;

    public:
        explicit ObjectBuilder(TagResolverService& tagResolverService) :
            m_TagResolverService(tagResolverService) {}
        ~ObjectBuilder() = default;

        // param data: Object data of the family. It must have the common object fields.
        template <typename TData, typename TMultiplayerObjectEntry>
        auto Build(const std::string& tagName, const TData& data,
            const std::vector<TMultiplayerObjectEntry>& multiplayerObject) -> Object
        {
            Object out{};

            out.TagName = tagName;

            // Classification.
            if (data.ObjectType <= static_cast<std::uint16_t>(ObjectKind::EffectScenery))
            {
                out.ObjectKind = static_cast<ObjectKind>(data.ObjectType);
            }
            else
            {
                out.ObjectKind = ObjectKind::Invalid;
            }

            out.BoundingRadius = data.BoundingRadius;
            out.BoundingOffset = { data.BoundingOffset.X, data.BoundingOffset.Y, data.BoundingOffset.Z };

            // Damage.
            out.CollisionDamageTagName = m_TagResolverService.ResolveTagReferenceName(data.CollisionDamage);
            out.BrittleCollisionDamageTagName = m_TagResolverService.ResolveTagReferenceName(data.BrittleCollisionDamage);

            // Model.
            out.ModelTagName = m_TagResolverService.ResolveTagReferenceName(data.Model);

            // Multiplayer.
            out.MultiplayerObjects.reserve(multiplayerObject.size());
            for (const auto& entry : multiplayerObject)
            {
                MultiplayerObject multiplayer{};

                if (entry.Type <= static_cast<std::uint8_t>(MultiplayerObjectKind::CinematicCameraPosition))
                {
                    multiplayer.MultiplayerObjectKind = static_cast<MultiplayerObjectKind>(entry.Type);
                }
                else
                {
                    multiplayer.MultiplayerObjectKind = MultiplayerObjectKind::Invalid;
                }

                out.MultiplayerObjects.push_back(std::move(multiplayer));
            }

            return out;
        }

    private:
        TagResolverService& m_TagResolverService;
    };
}