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
    inline constexpr std::array<std::string_view, 24> k_PipelineStageNames{
        "ObjectTable", "PlayerTable", "InteractionTable", "GameTable", "Classifier", "ObjectGraph",
        "VitalityDetails", "ZoneDetails", "TeleporterDetails", "LiftDetails", "BarrierDetails", "ExplosiveDetails", "ProjectileDetails", "WeaponDetails", "Entities", "Players",
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
        // param canPublish: Asked after every stage. When it says false the run stops there: the later stages do not run, the Tick is not
        // assembled and the last one stays as it was (the engine started tearing down while the tick ran, F1 C.7). A stage already
        // running when the teardown starts still finishes. Empty to always go on.
        auto Run(const Snapshot& snapshot, std::uint64_t generation,
            const std::function<void()>& endStage, const std::function<bool()>& canPublish = {}) -> void;

        // Decodes the light snapshot of a tick that does not run the pipeline (sparse mode, Q101) and generates the flag events of that tick.
        // note: Touches only the light stores of Blam.Runtime and the memory of the events service. Nothing is published to the Tick, and the
        // stores of the full tick stay as the last full tick left them.
        auto RunLight(const Snapshot& snapshot) -> void;

    private:
        Blam::Runtime::Layer& m_Tables;
        Perception::Layer& m_Environment;
        Views::Layer& m_Egocentric;
        TickService& m_TickService;
    };
}