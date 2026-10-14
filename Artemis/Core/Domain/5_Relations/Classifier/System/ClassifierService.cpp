module Relations.Classifier.System;

import Common.Semantic.Type;
import Common.Tag.Type;
import Common.ZoneShape.Type;
import Resolved.Definitions.Type;

namespace
{
    namespace TagName = Common::Tag::Type;
    namespace Subtype = Common::Semantic::Type::Subtype;

    using Common::Semantic::Type::Function;
    using Common::Semantic::Type::Binding;
    using Common::Semantic::Type::Capability;
    using Common::Semantic::Type::GroupOf;
    using Common::Semantic::Type::Catalog::TagLabel;
    using Common::Semantic::Type::Catalog::FindTagLabel;
    using Common::ZoneShape::Type::Kind;
    using Resolved::Definitions::Type::Object::ObjectKind;
    using Resolved::Definitions::Type::Object::MultiplayerObject;
    using Resolved::Definitions::Type::Object::MultiplayerObjectKind;
    using Relations::Classifier::Type::Source;
    using Relations::Classifier::Type::Classified;
    using Relations::Classifier::Type::TagClassification;

    using Classifieds = std::vector<Classified>;

    // Value of an empty handle.
    constexpr std::uint32_t k_NoHandle{ 0xFFFFFFFF };

    // Function and subtype one rule gives.
    struct Label
    {
        ::Function Function{ ::Function::Unknown };
        std::uint8_t Subtype{};
    };

    // Converts a subtype enum value to the number a classification keeps.
    template <typename TSubtype>
    constexpr auto SubtypeValue(TSubtype subtype) -> std::uint8_t
    {
        return static_cast<std::uint8_t>(subtype);
    }

    // Returns the multiplayer objects of a tag definition, or null if the store does not have the tag.
    template <typename TStore>
    auto MultiplayerObjectsIn(const TStore& store, const std::string& tagName) -> const std::vector<MultiplayerObject>*
    {
        const auto* definition{ store.Get(tagName) };
        return definition == nullptr ? nullptr : &definition->Base.MultiplayerObjects;
    }

    // Function and subtype of a multiplayer object kind (05, section 4). Only what the engine states without ambiguity.
    // return: Unknown for Ordinary and for kinds whose function depends on the tag (objectives that are not weapons).
    auto LabelOfMultiplayerKind(MultiplayerObjectKind multiplayerKind, ObjectKind kind) -> Label
    {
        switch (multiplayerKind)
        {
        case MultiplayerObjectKind::Weapon: return { .Function = Function::Weapon };
        case MultiplayerObjectKind::Grenade: return { .Function = Function::Grenade };
        case MultiplayerObjectKind::Projectile: return { .Function = Function::Projectile };
        case MultiplayerObjectKind::Powerup: return { .Function = Function::Powerup };
        case MultiplayerObjectKind::Equipment: return { .Function = Function::ArmorAbility };
        case MultiplayerObjectKind::AmmoPack: return { .Function = Function::AmmoPack };
        case MultiplayerObjectKind::LightLandVehicle:
        case MultiplayerObjectKind::HeavyLandVehicle:
        case MultiplayerObjectKind::FlyingVehicle: return { .Function = Function::Vehicle };
        case MultiplayerObjectKind::Turret: return { .Function = Function::Turret };
        case MultiplayerObjectKind::Device: return { .Function = Function::Device };
        case MultiplayerObjectKind::Teleporter2Way: return { .Function = Function::Teleporter, .Subtype = SubtypeValue(Subtype::Teleporter::TwoWay) };
        case MultiplayerObjectKind::TeleporterSender: return { .Function = Function::Teleporter, .Subtype = SubtypeValue(Subtype::Teleporter::Sender) };
        case MultiplayerObjectKind::TeleporterReceiver: return { .Function = Function::Teleporter, .Subtype = SubtypeValue(Subtype::Teleporter::Receiver) };
        case MultiplayerObjectKind::PlayerSpawnLocation: return { .Function = Function::SpawnPoint };
        case MultiplayerObjectKind::PlayerRespawnZone: return { .Function = Function::SpawnZone };
        case MultiplayerObjectKind::Fireteam1RespawnZone: return { .Function = Function::SpawnZone, .Subtype = SubtypeValue(Subtype::SpawnZone::Fireteam1) };
        case MultiplayerObjectKind::Fireteam2RespawnZone: return { .Function = Function::SpawnZone, .Subtype = SubtypeValue(Subtype::SpawnZone::Fireteam2) };
        case MultiplayerObjectKind::Fireteam3RespawnZone: return { .Function = Function::SpawnZone, .Subtype = SubtypeValue(Subtype::SpawnZone::Fireteam3) };
        case MultiplayerObjectKind::Fireteam4RespawnZone: return { .Function = Function::SpawnZone, .Subtype = SubtypeValue(Subtype::SpawnZone::Fireteam4) };
        case MultiplayerObjectKind::NamedLocationArea: return { .Function = Function::LocationArea };
        case MultiplayerObjectKind::DangerZone: return { .Function = Function::DangerZone };
        case MultiplayerObjectKind::SafeVolume: return { .Function = Function::Boundary, .Subtype = SubtypeValue(Subtype::Boundary::Safe) };
        case MultiplayerObjectKind::KillVolume: return { .Function = Function::Boundary, .Subtype = SubtypeValue(Subtype::Boundary::Kill) };
        case MultiplayerObjectKind::CinematicCameraPosition: return { .Function = Function::CameraMarker };

        case MultiplayerObjectKind::PrimaryObjective:
        case MultiplayerObjectKind::SecondaryObjective:
            // note: A weapon objective is the flag, bomb or ball. Stands, plates and hills need the catalog.
            return kind == ObjectKind::Weapon ? Label{ .Function = Function::ObjectiveItem } : Label{};

        default: return {};
        }
    }

    // Default function of an object kind (05, section 3). Equipment has none: it needs the catalog or the multiplayer kind.
    auto FunctionOfKind(ObjectKind kind, bool hasCollision) -> Function
    {
        switch (kind)
        {
        case ObjectKind::Biped: return Function::Combatant;
        case ObjectKind::Vehicle: return Function::Vehicle;
        case ObjectKind::Weapon: return Function::Weapon;
        case ObjectKind::Projectile: return Function::Projectile;
        case ObjectKind::Scenery:
        case ObjectKind::Crate: return hasCollision ? Function::Prop : Function::Decor;
        case ObjectKind::Machine:
        case ObjectKind::Control: return Function::Device;
        case ObjectKind::SoundScenery:
        case ObjectKind::EffectScenery: return Function::Decor;
        default: return Function::Unknown;
        }
    }

    // Capabilities that follow from the function, the subtype and the tag (05, section 6).
    auto StaticCapabilities(Function function, std::uint8_t subtype, bool hasCollision, bool hasVitality) -> Capability
    {
        Capability capabilities{ Capability::None };

        if (hasCollision) capabilities = capabilities | Capability::HasCollision;
        if (hasVitality) capabilities = capabilities | Capability::HasVitality;

        switch (function)
        {
        case Function::Vehicle:
            capabilities = capabilities | Capability::IsEnterable;
            break;

        case Function::Turret:
            capabilities = capabilities | Capability::IsEnterable | Capability::CanDamage;
            break;

        case Function::Weapon:
        case Function::Projectile:
        case Function::Explosive:
            capabilities = capabilities | Capability::CanDamage;
            break;

        case Function::Boundary:
        {
            const bool isKill{ subtype == SubtypeValue(Subtype::Boundary::Kill) || subtype == SubtypeValue(Subtype::Boundary::SoftKill) };
            if (isKill) capabilities = capabilities | Capability::CanDamage;
            break;
        }

        case Function::HealthStation:
            capabilities = capabilities | Capability::IsUsable;
            break;

        case Function::ObjectiveItem:
        case Function::ObjectiveSpawn:
        case Function::ObjectiveZone:
            capabilities = capabilities | Capability::IsObjective;
            break;

        default:
            break;
        }

        return capabilities;
    }

    // Link of a child object to its parent (05, section 5).
    // param parentKind: Invalid when the parent is not in the object table.
    // note: Carried = item held by a biped. Seated = biped inside a vehicle. Attached = any other child.
    auto BindingOf(ObjectKind kind, ObjectKind parentKind) -> Binding
    {
        const bool isItem{ kind == ObjectKind::Weapon || kind == ObjectKind::Equipment };
        if (isItem && parentKind == ObjectKind::Biped) return Binding::Carried;
        if (kind == ObjectKind::Biped && parentKind == ObjectKind::Vehicle) return Binding::Seated;

        return Binding::Attached;
    }
}

namespace Relations::Classifier::System
{
    auto ClassifierService::UpdateClassification() -> void
    {
        if (m_IsCacheStale.exchange(false)) m_TagClassifications.clear();

        auto objectTablePtr = m_ObjectStore.Acquire();
        if (!objectTablePtr) return;
        const ObjectTable& objectTable = *objectTablePtr;

        Classifieds results;
        results.reserve(objectTable.size());

        for (const auto& [handle, object] : objectTable)
        {
            if (object.Address == 0) continue;

            const TagClassification tag{ this->FindTagClassification(object) };

            Classified classified{};
            classified.Handle = handle;
            classified.Role = this->ClassifyNode(handle, object, objectTable);
            classified.Kind = object.Profile.ObjectKind;
            classified.Function = tag.Function;
            classified.Subtype = tag.Subtype;
            classified.Group = GroupOf(tag.Function);
            classified.Source = tag.Source;
            if (object.ParentHandle != k_NoHandle)
            {
                const auto parentIt{ objectTable.find(object.ParentHandle) };
                const ObjectKind parentKind{ parentIt == objectTable.end() ? ObjectKind::Invalid : parentIt->second.Profile.ObjectKind };

                classified.Binding = BindingOf(object.Profile.ObjectKind, parentKind);
                classified.BoundTo = object.ParentHandle;
            }
            classified.Capabilities = tag.Capabilities;

            if (object.Multiplayer && object.Multiplayer->Shape.Kind != Kind::None)
            {
                classified.Capabilities = classified.Capabilities | Capability::HasZone;
            }

            results.push_back(std::move(classified));
        }

        m_ClassifierStore.Publish(std::move(results));
    }

    auto ClassifierService::FindTagClassification(const AliveObject& object) -> TagClassification
    {
        // note: Before the definitions are frozen the multiplayer kinds and the vitality are missing, so nothing is cached.
        const bool isReady{ m_DefinitionsStore.Bipd.IsFrozen() && m_VitalityStore.IsFrozen() };
        if (!isReady) return this->ClassifyTag(object);

        const std::string key{ object.FourCC + '|' + object.TagName };

        const auto it{ m_TagClassifications.find(key) };
        if (it != m_TagClassifications.end()) return it->second;

        const TagClassification classification{ this->ClassifyTag(object) };
        m_TagClassifications.emplace(key, classification);

        if (classification.Function == Function::Unknown)
        {
            m_LogsService.Message("[ClassifierService] WARNING: No function for the {} tag {}.", object.FourCC, object.TagName);
        }

        return classification;
    }

    auto ClassifierService::ClassifyTag(const AliveObject& object) const -> TagClassification
    {
        const ObjectKind kind{ object.Profile.ObjectKind };
        const bool hasCollision{ object.Profile.HasColl };
        const bool hasVitality{ m_VitalityStore.Has(object.TagName) };

        // Multiplayer kind of the tag: the first one that is not Ordinary.
        Label multiplayer{};
        const std::vector<MultiplayerObject>* multiplayerObjects{ nullptr };

        switch (kind)
        {
        case ObjectKind::Biped: multiplayerObjects = MultiplayerObjectsIn(m_DefinitionsStore.Bipd, object.TagName); break;
        case ObjectKind::Vehicle: multiplayerObjects = MultiplayerObjectsIn(m_DefinitionsStore.Vehi, object.TagName); break;
        case ObjectKind::Weapon: multiplayerObjects = MultiplayerObjectsIn(m_DefinitionsStore.Weap, object.TagName); break;
        case ObjectKind::Equipment: multiplayerObjects = MultiplayerObjectsIn(m_DefinitionsStore.Eqip, object.TagName); break;
        case ObjectKind::Projectile: multiplayerObjects = MultiplayerObjectsIn(m_DefinitionsStore.Proj, object.TagName); break;
        case ObjectKind::Scenery: multiplayerObjects = MultiplayerObjectsIn(m_DefinitionsStore.Scen, object.TagName); break;
        case ObjectKind::Machine: multiplayerObjects = MultiplayerObjectsIn(m_DefinitionsStore.Mach, object.TagName); break;
        case ObjectKind::Control: multiplayerObjects = MultiplayerObjectsIn(m_DefinitionsStore.Ctrl, object.TagName); break;
        case ObjectKind::Crate: multiplayerObjects = MultiplayerObjectsIn(m_DefinitionsStore.Bloc, object.TagName); break;
        default: break;
        }

        if (multiplayerObjects != nullptr)
        {
            for (const MultiplayerObject& entry : *multiplayerObjects)
            {
                multiplayer = LabelOfMultiplayerKind(entry.MultiplayerObjectKind, kind);
                if (multiplayer.Function != Function::Unknown) break;
            }
        }

        TagClassification classification{};

        if (const TagLabel* label{ FindTagLabel(object.FourCC, object.TagName) }; label != nullptr)
        {
            // note: An unlabeled subtype of the catalog takes the one of the multiplayer kind when both agree on the function.
            const bool isSameFunction{ multiplayer.Function == label->Function };

            classification.Function = label->Function;
            classification.Subtype = (label->Subtype == 0 && isSameFunction) ? multiplayer.Subtype : label->Subtype;
            classification.Source = Source::Catalog;
        }
        else if (multiplayer.Function != Function::Unknown)
        {
            classification.Function = multiplayer.Function;
            classification.Subtype = multiplayer.Subtype;
            classification.Source = Source::MultiplayerKind;
        }
        else
        {
            classification.Function = FunctionOfKind(kind, hasCollision);
            classification.Source = classification.Function == Function::Unknown ? Source::None : Source::KindDefault;
        }

        classification.Capabilities = StaticCapabilities(classification.Function, classification.Subtype, hasCollision, hasVitality);

        return classification;
    }

    auto ClassifierService::ClassifyNode(uint32_t handle, const AliveObject& object,
        const ObjectTable& objectTable) const -> Role
    {
        switch (object.Profile.ObjectKind)
        {
        case ObjectKind::Biped:
            return Role::Biped;

        case ObjectKind::Vehicle:
            return this->ClassifyVehicle(object);

        case ObjectKind::Weapon:
            return this->ClassifyWeapon(object);

        case ObjectKind::Equipment:
            return this->ClassifyEquipment(object);

        case ObjectKind::Projectile:
            return Role::Projectile;

        case ObjectKind::Scenery:
            return this->ClassifyScenery(object);

        case ObjectKind::Machine:
            return Role::DeviceMachine;

        case ObjectKind::Control:
            return this->ClassifyDeviceControl(object);

        case ObjectKind::Crate:
            return this->ClassifyBloc(object);

        default:
            return Role::Unknown;
        }
    }

    auto ClassifierService::ClassifyVehicle(const AliveObject& object) const -> Role
    {
        const std::string& tag = object.TagName;

        if (TagName::Vehicle::IsVehicle(tag))
        {
            if (object.ParentHandle != 0xFFFFFFFF)
            {
                return Role::VehiclePart;
            }

            return Role::Vehicle;
        }

        if (TagName::Weapon::IsVehicle(tag))
        {
            return Role::Vehicle;
        }

        return Role::Unknown;
    }

    auto ClassifierService::ClassifyWeapon(const AliveObject& object) const -> Role
    {
        const std::string& tag = object.TagName;

        using namespace TagName;

        if (Objective::IsObjective(tag))
        {
            if (object.ParentHandle != 0xFFFFFFFF)
            {
                return Role::ObjectiveEquipped;
            }

            return Role::ObjectivePickup;
        }

        if (object.ParentHandle != 0xFFFFFFFF)
        {
            return Role::WeaponEquipped;
        }

        return Role::WeaponPickup;
    }

    auto ClassifierService::ClassifyEquipment(const AliveObject& object) const -> Role
    {
        const std::string& tag = object.TagName;

        if (TagName::Ammo::IsAmmo(tag))
        {
            return Role::AmmoPickup;
        }

        if (TagName::ArmorAbility::IsArmorAbility(tag))
        {
            if (object.ParentHandle != 0xFFFFFFFF)
            {
                return Role::ArmorAbilityEquipped;
            }

            return Role::ArmorAbilityPickup;
        }

        if (TagName::Weapon::IsGrenade(tag))
        {
            return Role::GrenadePickup;
        }

        if (TagName::Powerup::IsPowerup(tag))
        {
            return Role::Powerup;
        }

        if (TagName::Shield::IsShield(tag))
        {
            return Role::Shield;
        }

        return Role::Unknown;
    }

    auto ClassifierService::ClassifyScenery(const AliveObject& object) const -> Role
    {
        const std::string& tag = object.TagName;

        if (TagName::Spawn::IsSpawn(tag))
        {
            return Role::Spawn;
        }

        if (TagName::Boundary::IsBoundary(tag))
        {
            return Role::Boundary;
        }

        if (TagName::Shield::IsPortableShield(tag))
        {
            return Role::PortableShield;
        }

        if (TagName::SkyBox::IsSkyBox(tag))
        {
            return Role::None;
        }

        // TODO: More filters needs to be applied here.
        return Role::SceneryObstacle;
    }

    auto ClassifierService::ClassifyBloc(const AliveObject& object) const -> Role
    {
        const std::string& tag = object.TagName;

        if (TagName::Objective::IsObjectiveSpawn(tag))
        {
            return Role::ObjectiveSpawn;
        }

        if (TagName::Objective::IsObjectiveZone(tag))
        {
            return Role::ObjectiveZone;
        }

        if (TagName::Teleport::IsTeleport(tag))
        {
            return Role::Teleporter;
        }

        if (TagName::Explosive::IsExplosive(tag))
        {
            return Role::Explosive;
        }

        if (TagName::Pallets::IsPallet(tag))
        {
            return Role::Pallet;
        }

        if (TagName::Lift::IsLift(tag))
        {
            return Role::Lift;
        }

        if (TagName::Shield::IsShield(tag))
        {
            return Role::Shield;
        }

        // TODO: More filters needs to be applied here.
        return Role::CrateObstacle;
    }

    auto ClassifierService::ClassifyDeviceControl(const AliveObject& object) const -> Role
    {
        if (TagName::ControlDevice::IsHealthStation(object.TagName))
        {
            return Role::HealthStation;
        }

        return Role::Unknown;
    }

    auto ClassifierService::Cleanup() -> void
    {
        m_ClassifierStore.Cleanup();
        m_IsCacheStale.store(true);

        m_LogsService.Message("[ClassifierService] INFO: Cleanup completed.");
    }
}