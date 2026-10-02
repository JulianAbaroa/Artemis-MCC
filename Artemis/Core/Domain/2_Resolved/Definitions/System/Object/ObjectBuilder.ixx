export module Resolved.Definitions.System:Object;

import Map.Reader.System;
import Resolved.Definitions.Type;
import std;

export namespace Resolved::Definitions::System
{
	class ObjectBuilder
	{
    private:
        using ResolvedObject = Resolved::Definitions::Type::Object::Object;
        using ResolvedMultiplayerObject = Resolved::Definitions::Type::Object::MultiplayerObject;
        using ObjectKind = Resolved::Definitions::Type::Object::Kind;
        using MultiplayerObjectKind = Resolved::Definitions::Type::Object::MultiplayerObjectKind;
        using TagResolverService = Map::Reader::System::TagResolverService;

    public:
        explicit ObjectBuilder(TagResolverService& tagResolverService) :
            m_TagResolverService(tagResolverService) {}
        ~ObjectBuilder() = default;

        template <typename TData, typename TMultiplayerObjectEntry>
        auto Build(const std::string& tagName, const TData& data,
            const std::vector<TMultiplayerObjectEntry>& multiplayerObject) -> ResolvedObject
        {
            ResolvedObject out{};
            out.TagName = tagName;

            // Classification
            if (data.ObjectType <= static_cast<std::uint16_t>(ObjectKind::EffectScenery))
            {
                out.Kind = static_cast<ObjectKind>(data.ObjectType);
            }
            else out.Kind = ObjectKind::Invalid;

            out.BoundingRadius = data.BoundingRadius;
            out.BoundingOffset = { data.BoundingOffset.X, data.BoundingOffset.Y, data.BoundingOffset.Z };

            // Damage
            out.CollisionDamageTagName = m_TagResolverService.ResolveTagReferenceName(data.CollisionDamage);
            out.BrittleCollisionDamageTagName = m_TagResolverService.ResolveTagReferenceName(data.BrittleCollisionDamage);

            // Model
            out.ModelTagName = m_TagResolverService.ResolveTagReferenceName(data.Model);

            // Multiplayer
            out.MultiplayerObjects.reserve(multiplayerObject.size());
            for (const auto& entry : multiplayerObject)
            {
                ResolvedMultiplayerObject mp{};

                if (entry.Type <= static_cast<std::uint8_t>(MultiplayerObjectKind::CinematicCameraPosition))
                {
                    mp.Kind = static_cast<MultiplayerObjectKind>(entry.Type);
                }
                else mp.Kind = MultiplayerObjectKind::Invalid;

                out.MultiplayerObjects.push_back(std::move(mp));
            }

            return out;
        }

    private:
        TagResolverService& m_TagResolverService;
	};
}