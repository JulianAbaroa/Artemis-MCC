export module Tables.Object.Type:Crate;

import Common.Tag.Type;
import Common.ZoneShape.Type;
import std;

namespace
{
    using Common::ZoneShape::Type::ZoneShape;

    using Common::Tag::Type::Teleport::k_TeleportReceiver;
    using Common::Tag::Type::Teleport::k_TeleportTwoWay;
    using Common::Tag::Type::Explosive::k_PlasmaBattery;
    using Common::Tag::Type::Teleport::k_TeleportSender;
    using Common::Tag::Type::Objective::k_CapturePlate;
    using Common::Tag::Type::Explosive::k_PropaneTank;
    using Common::Tag::Type::Objective::k_HillMarker;
    using Common::Tag::Type::Explosive::k_FusionCoil;
    using Common::Tag::Type::Objective::k_FlagStand;
    using Common::Tag::Type::Pallets::k_PalletLarge;
    using Common::Tag::Type::Explosive::k_Landmine;
    using Common::Tag::Type::Pallets::k_Pallet;
}

export namespace Tables::Object::Type::Crate
{
    enum class CrateKind : std::uint8_t
    {
        Unknown = 0,

        // Zone / Objective.
        FlagStand, CapturePlate, HillMarker,
        TeleportSender, TeleportReceiver, TeleportTwoWay,

        // Destructible.
        FusionCoil, Landmine, PlasmaBattery,
        PropaneTank, Pallete,
    };

    namespace Teleport
    {
        enum class Allowed : std::uint8_t
        {
            None = 0x00,
            NoPlayers = 0x01,
            AllowGround = 0x02,
            AllowHeavy = 0x04,
            AllowFlying = 0x08,
            AllowProjctile = 0x10,
        };

        inline Allowed operator|(Allowed a, Allowed b)
        {
            return static_cast<Allowed>(
                static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
        }

        inline auto HasFlag(Allowed mask, Allowed flag) -> bool
        {
            return (static_cast<std::uint8_t>(mask) &
                static_cast<std::uint8_t>(flag)) != 0;
        }

        struct Teleport
        {
            ZoneShape ZoneShape{};
            std::uint8_t Channel{};
            Allowed Allowed{};
        };
    }

    inline auto ResolveCrateType(const std::string& tagName) -> CrateKind
    {
        static const std::unordered_map<std::string, CrateKind> map =
        {
            // Objectives.
            { k_FlagStand,             CrateKind::FlagStand         },
            { k_CapturePlate,          CrateKind::CapturePlate      },
            { k_HillMarker,            CrateKind::HillMarker        },

            // Teleports.
            { k_TeleportSender,         CrateKind::TeleportSender    },
            { k_TeleportReceiver,       CrateKind::TeleportReceiver  },
            { k_TeleportTwoWay,         CrateKind::TeleportTwoWay    },

            // Palletes.
            { k_Pallet,                  CrateKind::Pallete,          },
            { k_PalletLarge,             CrateKind::Pallete,          },

            // Explosives.
            { k_FusionCoil,            CrateKind::FusionCoil        },
            { k_Landmine,              CrateKind::Landmine          },
            { k_PlasmaBattery,         CrateKind::PlasmaBattery     },
            { k_PropaneTank,           CrateKind::PropaneTank       },
        };

        auto it = map.find(tagName);
        return it != map.end() ? it->second : CrateKind::Unknown;
    }

    struct Crate
    {
        CrateKind Kind{};

        std::optional<Teleport::Teleport> Teleport{};
    };
}