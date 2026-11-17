export module Tests.Support.ViewProbe;

import Common.Engine.Type;
import Common.Math.Type;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Object.Type;
import Perception.Players.Type;
import Views.Raycast.System;
import Views.Raycast.Type;
import Tests.Support.Bench;
import Tests.Support.Pipeline;
import std;

// What the offline benches of the views per agent (F4.B) share: the eye of the local player derived from its tag (A) and taken from the
// bone of its head (B), against the camera the engine gives it.

namespace
{
    using Common::Math::Type::Vec3;
    using ::Blam::MapModel::Definitions::Type::Bipd::Bipd;
    using ::Blam::Runtime::Object::Type::Alive::AliveObject;
    using ::Blam::Runtime::Object::Type::Alive::ObjectTable;
    using Perception::Players::Type::Body;
    using Perception::Players::Type::Player;
    using Tests::Support::Bench::Series;
    using Tests::Support::Bench::Stats;
    using Tests::Support::Pipeline::Chain;
    using Views::Raycast::System::DerivedEye;
    using Views::Raycast::System::HeadBoneEye;
    using Views::Raycast::Type::EyePoint;

    using Common::Engine::Type::k_NoHandle;

    // CrouchFraction at or below which the biped stands and at or above which it is crouched, in the sense of this probe.
    constexpr float k_StandingFraction{ 0.001f };
    constexpr float k_CrouchedFraction{ 0.999f };
    // Own velocity (world units per tick) under which the biped counts as standing still.
    constexpr float k_StillSpeed{ 0.01f };

    auto Distance(const Vec3& a, const Vec3& b) -> double
    {
        const double x{ static_cast<double>(a.X) - static_cast<double>(b.X) };
        const double y{ static_cast<double>(a.Y) - static_cast<double>(b.Y) };
        const double z{ static_cast<double>(a.Z) - static_cast<double>(b.Z) };
        return std::sqrt((x * x) + (y * y) + (z * z));
    }
}

export namespace Tests::Support::ViewProbe
{
    // return: The eye A of the biped of the player, from the tag of the biped. Invalid when the player has no live biped with a tag.
    auto PlayerEye(const Chain& chain, const ObjectTable& objects, const Player& player) -> EyePoint
    {
        if (!player.Body) return {};

        const auto object{ objects.find(player.BipedHandle) };
        if (object == objects.end()) return {};

        const Bipd* definition{ chain.m_Resolved.m_DefinitionsStore.m_Bipd.Get(object->second.TagName) };
        if (!definition) return {};

        return EyePoint{ DerivedEye(object->second, *definition, player.Body->CrouchFraction), true };
    }

    // The pose of the local player in a tick.
    enum class EyeState : std::uint8_t
    {
        StandingStill,
        StandingMoving,
        CrouchedStill,
        CrouchedMoving,
        Transition,
        Air,
    };

    inline constexpr std::size_t k_EyeStates{ 6 };

    // return: The label of the pose in the reports.
    auto EyeStateName(EyeState state) -> std::string_view
    {
        switch (state)
        {
        case EyeState::StandingStill: return "standing, still";
        case EyeState::StandingMoving: return "standing, moving";
        case EyeState::CrouchedStill: return "crouched, still";
        case EyeState::CrouchedMoving: return "crouched, moving";
        case EyeState::Transition: return "crouch transition";
        case EyeState::Air: return "in the air";
        }

        return "?";
    }

    // Measures, tick by tick, how far the two eyes are from the camera of the local players of a film.
    class EyeProbe
    {
    public:
        // Called after each record (or each decision) of a pass.
        auto Observe(const Chain& chain) -> void
        {
            const auto players{ chain.m_Environment.m_PlayersStore.Acquire() };
            const auto objects{ chain.m_Tables.m_ObjectStore.Acquire() };
            if (!players || !objects) return;

            for (const Player& player : players->Rows)
            {
                if (!player.LocalIndex || !player.IsAlive()) continue;

                const auto object{ objects->find(player.BipedHandle) };
                if (object == objects->end()) continue;

                const AliveObject& biped{ object->second };
                if (biped.ParentHandle != k_NoHandle)
                {
                    ++m_InVehicle;
                    continue;
                }

                const Bipd* definition{ chain.m_Resolved.m_DefinitionsStore.m_Bipd.Get(biped.TagName) };
                if (!definition)
                {
                    ++m_NoTag;
                    continue;
                }

                const Body& body{ *player.Body };
                const EyeState state{ this->Classify(body) };
                Slot& slot{ m_Slots[static_cast<std::size_t>(state)] };

                ++m_Samples;
                const Vec3 derived{ DerivedEye(biped, *definition, body.CrouchFraction) };
                slot.A.Add(Distance(derived, player.CameraPosition));

                const auto* bones{ chain.m_Tables.m_BoneMatricesStore.Get(player.BipedHandle) };
                const EyePoint head{ bones ? HeadBoneEye(*bones, definition->HeadNodeIndex) : EyePoint{} };
                if (head.Valid) slot.B.Add(Distance(head.Position, player.CameraPosition));
                else ++m_NoBone;
            }
        }

        // return: The distance of the eye A (derived) to the camera in the pose, in world units.
        auto StatsA(EyeState state) const -> Stats
        {
            return m_Slots[static_cast<std::size_t>(state)].A.Summary();
        }

        // return: The distance of the eye B (head bone) to the camera in the pose, in world units.
        auto StatsB(EyeState state) const -> Stats
        {
            return m_Slots[static_cast<std::size_t>(state)].B.Summary();
        }

        // return: The lines of the table of the eye of the film.
        auto Lines() const -> std::vector<std::string>
        {
            std::vector<std::string> lines{};
            lines.push_back(std::format("Eye of the local player against its camera, world units: {} samples, {} in a vehicle (left out), {} without bipd tag, {} without a valid head bone.",
                m_Samples, m_InVehicle, m_NoTag, m_NoBone));
            for (std::size_t index{ 0 }; index < k_EyeStates; ++index)
            {
                const EyeState state{ static_cast<EyeState>(index) };
                lines.push_back(this->Line("A derived", EyeStateName(state), this->StatsA(state)));
                lines.push_back(this->Line("B head bone", EyeStateName(state), this->StatsB(state)));
            }

            return lines;
        }

    private:
        struct Slot
        {
            Series A{};
            Series B{};
        };

        static auto Line(std::string_view eye, std::string_view state, const Stats& stats) -> std::string
        {
            return std::format("  eye {:<12} {:<18} n={:<6} p50={:>8.4f} p99={:>8.4f} max={:>8.4f}", eye, state, stats.N, stats.P50, stats.P99, stats.Max);
        }

        static auto Classify(const Body& body) -> EyeState
        {
            if (!body.IsGrounded) return EyeState::Air;
            if (body.CrouchFraction > k_StandingFraction && body.CrouchFraction < k_CrouchedFraction) return EyeState::Transition;

            const float speed{ std::sqrt((body.OwnVelocity.X * body.OwnVelocity.X) + (body.OwnVelocity.Y * body.OwnVelocity.Y) + (body.OwnVelocity.Z * body.OwnVelocity.Z)) };
            const bool isStill{ speed < k_StillSpeed };
            if (body.CrouchFraction >= k_CrouchedFraction) return isStill ? EyeState::CrouchedStill : EyeState::CrouchedMoving;
            return isStill ? EyeState::StandingStill : EyeState::StandingMoving;
        }

        std::array<Slot, k_EyeStates> m_Slots{};
        std::size_t m_Samples{};
        std::size_t m_InVehicle{};
        std::size_t m_NoTag{};
        std::size_t m_NoBone{};
    };
}