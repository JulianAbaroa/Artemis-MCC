export module Bridge.Codec.Pipeline.System;

import Blam.Layout.Snapshot.Type;
import Blam.Runtime.Layer;
import Perception.Layer;
import Views.Layer;
import Bridge.Codec.Tick.System;
import std;

export namespace Bridge::Codec::Pipeline::System
{
    // Stages of Run in the order they run, one name per endStage call.
    inline constexpr std::array<std::string_view, 23> k_PipelineStageNames{
        "ObjectTable", "PlayerTable", "InteractionTable", "GameTable", "Classifier", "ObjectGraph",
        "VitalityDetails", "ZoneDetails", "TeleporterDetails", "LiftDetails", "BarrierDetails", "ProjectileDetails", "WeaponDetails", "Entities", "Players",
        "ObjectiveDetails", "Aim", "Events", "Match", "Self", "Affordance", "Raycast", "Assemble" };

    // Runs the pure chain of one tick over a Snapshot: Tables, Environment (with Relations), Egocentric and the assembly of the Tick (D205).
    // note: The one place where the order of the tick lives. The AI thread and the tests call it.
    class PipelineService
    {
    private:
        using Snapshot = Blam::Layout::Snapshot::Type::Snapshot;
        using TickService = Bridge::Codec::Tick::System::TickService;

    public:
        PipelineService(Blam::Runtime::Layer& tables, Perception::Layer& environment,
            Views::Layer& egocentric, TickService& tickService) :
            m_Tables(tables), m_Environment(environment), m_Egocentric(egocentric),
            m_TickService(tickService) {}
        ~PipelineService() = default;

        PipelineService(const PipelineService&) = delete;
        auto operator=(const PipelineService&) -> PipelineService& = delete;

        // Updates every service from the Snapshot and assembles the Tick of the generation.
        // param endStage: Called after each of the stages of k_PipelineStageNames. Empty to skip it.
        auto Run(const Snapshot& snapshot, std::uint64_t generation,
            const std::function<void()>& endStage) -> void;

    private:
        Blam::Runtime::Layer& m_Tables;
        Perception::Layer& m_Environment;
        Views::Layer& m_Egocentric;
        TickService& m_TickService;
    };
}