export module Export.Pipeline.System;

import Layout.Snapshot.Type;
import Tables.Layer;
import Relations.Layer;
import Environment.Layer;
import Egocentric.Layer;
import Export.Tick.System;
import std;

export namespace Export::Pipeline::System
{
    // Stages of Run in the order they run, one name per endStage call.
    inline constexpr std::array<std::string_view, 21> k_PipelineStageNames{
        "ObjectTable", "PlayerTable", "InteractionTable", "GameTable", "Classifier", "ObjectGraph",
        "VitalityDetails", "ZoneDetails", "TeleporterDetails", "LiftDetails", "BarrierDetails", "Entities", "Players",
        "Collidable", "Aim", "Events", "Match", "Self", "Affordance", "Raycast", "Assemble" };

    // Runs the pure chain of one tick over a Snapshot: Tables, Relations, Environment, Egocentric and the assembly of the Tick (D205).
    // note: The one place where the order of the tick lives. The AI thread and the tests call it.
    class PipelineService
    {
    private:
        using Snapshot = Layout::Snapshot::Type::Snapshot;
        using TickService = Export::Tick::System::TickService;

    public:
        PipelineService(Tables::Layer& tables, Relations::Layer& relations, Environment::Layer& environment,
            Egocentric::Layer& egocentric, TickService& tickService) :
            m_Tables(tables), m_Relations(relations), m_Environment(environment), m_Egocentric(egocentric),
            m_TickService(tickService) {}
        ~PipelineService() = default;

        PipelineService(const PipelineService&) = delete;
        auto operator=(const PipelineService&) -> PipelineService& = delete;

        // Updates every service from the Snapshot and assembles the Tick of the generation.
        // param isViewerActive: The viewer camera is on, the collidable service skips what it hides.
        // param endStage: Called after each of the stages of k_PipelineStageNames. Empty to skip it.
        auto Run(const Snapshot& snapshot, std::uint64_t generation, bool isViewerActive,
            const std::function<void()>& endStage) -> void;

    private:
        Tables::Layer& m_Tables;
        Relations::Layer& m_Relations;
        Environment::Layer& m_Environment;
        Egocentric::Layer& m_Egocentric;
        TickService& m_TickService;
    };
}