export module Tables.Object.Type:Vehicle;

import Common.Math.Type;
import Common.Tag.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Resolved::Definitions::Type::Vehi::SeatKind;
}

export namespace Tables::Object::Type::Vehicle
{
    enum class VehicleKind : std::uint8_t
    {
        Unknown,
        Banshee,
        Falcon, FalconSensor, FalconTurret, FalconGrenadeTurret, FalconChinGun,
        Ghost,
        Mongoose,
        Revenant, RevenantPlasmaTurret,
        Scorpion, ScorpionTurret, ScorpionCannon,
        ShadeTurret, ShadePlasmaCannon, ShadeFlakCannon,
        Warthog, WarthogChaingun, WarthogGauss, WarthogRocket, WarthogTroop,
        Wraith, WraithPlasmaTurret, WraithCannon,
        Sabre,
        Seraph,
        CartElectric,
        Forklift,
        Pickup,
        TruckCab,
        OniVan,
    };

    namespace Seat
    {
        // The vehicle datum has a variable block (descriptor 0x560, see Datum.Object.Type:Block) with
        // one element of 12 bytes per seat of the vehicle tag: a pair of floats and one static flag '3'.
        // The first float is a smoothed value that ramps towards the second one (the target) every tick.
        // The target is 1.0f when the seat is free and 0.0f when the seat is occupied.
        // The elements follow the order of the seats in the tag, so a seat is found by its index.
        //
        // The 'hijacker' seats are written after the normal seats, following the same structure and
        // behaviour. So, we can know whenever a specific seat is being hijacked by another player.
        // We don't know specificly which player, but with other data we get from other sides of this
        // project, we might be able to do this connection.
        //
        // note: The index of each seat was derived from the offsets that were used before the block was
        // found. The Grenadier of the Falcon had inconsistent offsets, its indices are assumed.
        struct Seat
        {
            std::size_t Index{};
            const char* Name{};
            bool IsHijackable{};
            bool IsHijackerSlot{};
        };

        struct SeatMarker
        {
            std::uint32_t SeatMarkerNameId{};
            std::int8_t NodeIndex{};
            Vec3 LocalTranslation{};
            Vec3 LocalRotation{};
            SeatKind SeatKind{};
            float EntryRadius{};
        };

        struct Layout
        {
            VehicleKind Kind{};
            std::vector<Seat> Seats{};
            std::vector<SeatMarker> SeatMarkers{};
        };
    }

    // TODO: Add the rest of the vehicles.
    inline const std::vector<Seat::Layout> k_Layouts =
    {
        {
            VehicleKind::Banshee,
            {
                // Seat.
                { 0, "Driver",                true, false },

                // Hijackers.
                { 1, "RightHijacker",         false, true },
                { 2, "LeftHijacker",          false, true },
            }
        },
        {
            VehicleKind::Falcon,
            {
                // Seats.
                { 0, "Driver",                true, false },
                { 2, "FrontRightPassenger",   false, false },
                { 1, "FrontLeftPassenger",    false, false },
                { 4, "BackRightPassenger",    false, false },
                { 3, "BackLeftPassenger",     false, false },
                { 5, "BackMiddlePassenger",   false, false },

                // Hijackers.
                { 7, "DriverRightHijacker",   false, true },
                { 6, "DriverLeftHijacker",    false, true },
            }
        },
        {
            VehicleKind::FalconTurret,
            {
                // Gunners.
                { 0, "Gunner",                true, false },

                // Hijackers.
                { 1, "GunnerHijacker",        false, true },
            }
        },
        {
            VehicleKind::FalconGrenadeTurret,
            {
                // Gunners.
                { 0, "Gunner",                true, false },

                // Hijackers.
                { 1, "GunnerHijacker",        false, true },
            }
        },
        {
            VehicleKind::Ghost,
            {
                // Seat.
                { 0, "Driver",                true, false },

                // Hijackers.
                { 2, "FrontRightHijacker",    false, true },
                { 3, "FrontLeftHijacker",     false, true },
                { 1, "BackHijacker",          false, true },
            }
        },
        {
            VehicleKind::Mongoose,
            {
                // Seats.
                { 0, "Driver",                true, false },
                { 1, "Passenger",             false, false },

                // Hijackers.
                { 2, "DriverHijacker",        false, true },
                { 3, "PassengerHijacker",     false, true },
            }
        },
        {
            VehicleKind::Revenant,
            {
                // Seats.
                { 0, "Driver",                true, false },
                { 1, "Passenger",             true, false },

                // Hijackers.
                { 2, "DriverHijacker",        false, true },
                { 3, "PassengerHijacker",     false, true },
            }
        },
        {
            VehicleKind::Scorpion,
            {
                // Seat.
                { 0, "Driver",                true, false },

                // Hijackers.
                { 5, "RightDriverHijacker",   false, true },
                { 6, "LeftDriverHijacker",    false, true },
                { 7, "BackHijacker",          false, true },
            }
        },
        {
            VehicleKind::ScorpionTurret,
            {
                // Gunner.
                { 0, "Gunner",                false, false },
            }
        },
        {
            VehicleKind::ShadeTurret,
            {
                // Seat.
                { 0, "Gunner",                false, false },
            }
        },
        {
            VehicleKind::Warthog,
            {
                // Seats.
                { 0, "Driver",                true, false },
                { 1, "Passenger",             true, false },

                // Hijackers.
                { 2, "DriverHijacker",        false, true },
                { 3, "PassengerHijacker",     false, true },
            }
        },
        {
            VehicleKind::WarthogChaingun,
            {
                // Gunner.
                { 0, "Gunner",                true, false },

                // Hijacker.
                { 1, "GunnerHijacker",        false, true },
            }
        },
        {
            VehicleKind::WarthogGauss,
            {
                // Gunner.
                { 0, "Gunner",                true, false },

                // Hijacker.
                { 1, "GunnerHijacker",        false, true },
            }
        },
        {
            VehicleKind::WarthogRocket,
            {
                // Gunner.
                { 0, "Gunner",                true, false },

                // Hijacker.
                { 1, "GunnerHijacker",        false, true },
            }
        },
        {
            VehicleKind::Wraith,
            {
                // Seat.
                { 0, "Driver",                true, false },

                // Hijackers.
                { 2, "RightDriverHijacker",   false, true },
                { 1, "LeftDriverHijacker",    false, true },
                { 3, "BackHijacker",          false, true },
            }
        },
        {
            VehicleKind::WraithPlasmaTurret,
            {
                // Gunner.
                { 0, "Gunner",                false, false },
            }
        },
    };

    // Returns the vehicle type based on the object's tag name.
    // Hardcoded solution, but those tag names are not going to change.
    inline auto ResolveVehicleType(const std::string& tagName) -> VehicleKind
    {
        namespace Vehicle = Common::Tag::Type::Vehicle;

        static const std::unordered_map<std::string, VehicleKind> map =
        {
            { Vehicle::k_Banshee,               VehicleKind::Banshee               },
            { Vehicle::k_Falcon,                VehicleKind::Falcon                },
            { Vehicle::k_FalconSensor,      VehicleKind::FalconGrenadeTurret   },
            { Vehicle::k_FalconTurretRight,   VehicleKind::FalconTurret            },
            { Vehicle::k_FalconTurretLeft,    VehicleKind::FalconTurret            },
            { Vehicle::k_FalconGrenadeRight,    VehicleKind::FalconGrenadeTurret   },
            { Vehicle::k_FalconGrenadeLeft, VehicleKind::FalconGrenadeTurret   },
            { Vehicle::k_FalconChinGun,     VehicleKind::FalconGrenadeTurret   },
            { Vehicle::k_Ghost,             VehicleKind::Ghost                 },
            { Vehicle::k_Mongoose,          VehicleKind::Mongoose              },
            { Vehicle::k_Revenant,          VehicleKind::Revenant              },
            { Vehicle::k_RevenantPlasmaTurret,VehicleKind::Revenant                },
            { Vehicle::k_Scorpion,          VehicleKind::Scorpion              },
            { Vehicle::k_ScorpionTurret,      VehicleKind::ScorpionTurret      },
            { Vehicle::k_ScorpionCannon,      VehicleKind::ScorpionCannon      },
            { Vehicle::k_Shade,             VehicleKind::ShadeTurret           },
            { Vehicle::k_ShadePlasmaCannon, VehicleKind::ShadePlasmaCannon     },
            { Vehicle::k_ShadeFlakCannon,       VehicleKind::ShadeFlakCannon       },
            { Vehicle::k_Warthog,               VehicleKind::Warthog               },
            { Vehicle::k_WarthogChaingun,       VehicleKind::WarthogChaingun       },
            { Vehicle::k_WarthogGauss,      VehicleKind::WarthogGauss          },
            { Vehicle::k_WarthogRocket,     VehicleKind::WarthogRocket         },
            { Vehicle::k_WarthogTroop,      VehicleKind::WarthogTroop          },
            { Vehicle::k_Wraith,                VehicleKind::Wraith                },
            { Vehicle::k_WraithPlasmaTurret,    VehicleKind::WraithPlasmaTurret    },
            { Vehicle::k_WraithCannon,      VehicleKind::WraithCannon          },
            { Vehicle::k_Sabre,             VehicleKind::Sabre                 },
            { Vehicle::k_Seraph,                VehicleKind::Seraph                },
            { Vehicle::k_CartElectric,      VehicleKind::CartElectric          },
            { Vehicle::k_Forklift,          VehicleKind::Forklift              },
            { Vehicle::k_Pickup,                VehicleKind::Pickup                },
            { Vehicle::k_TruckCab,          VehicleKind::TruckCab              },
            { Vehicle::k_OniVan,                VehicleKind::OniVan                },
        };

        auto it = map.find(tagName);
        return it != map.end() ? it->second : VehicleKind::Unknown;
    }

    inline auto GetSeatLayout(VehicleKind kind) -> const Seat::Layout*
    {
        for (const auto& layout : k_Layouts)
        {
            if (layout.Kind == kind) return &layout;
        }
        return nullptr;
    }

    struct Vehicle
    {
        std::uintptr_t Base{};
        VehicleKind Kind{};
        const Seat::Layout* SeatLayout{};

        // Target of each seat element, in the order of the tag. 1.0f = free, 0.0f = occupied.
        std::vector<float> SeatAvailability{};

        float BoostThrottle{};
        float BoostEnergy{};
        float BoostCooldown{};

        // A seat that the vehicle does not have is reported free.
        auto IsSeatFree(const Seat::Seat& seat) const -> bool
        {
            return seat.Index >= SeatAvailability.size() || SeatAvailability[seat.Index] == 1.0f;
        }
    };
}