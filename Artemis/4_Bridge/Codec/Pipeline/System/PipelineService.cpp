module Bridge.Codec.Pipeline.System;

import std;

namespace Bridge::Codec::Pipeline::System
{
    auto PipelineService::Run(const Snapshot& snapshot, std::uint64_t generation,
        const std::function<void()>& endStage) -> void
    {
        const auto stage{ [&endStage] {
            if (endStage) endStage();
        } };

        m_Tables.m_ObjectService.UpdateObjectTable(snapshot);
        stage();
        m_Tables.m_PlayerService.UpdatePlayerTable(snapshot);
        stage();
        m_Tables.m_InteractionService.UpdateInteractionTable(snapshot);
        stage();
        m_Tables.m_GameService.Update(snapshot);
        stage();

        m_Environment.m_ClassifierService.UpdateClassification();
        stage();
        m_Environment.m_ObjectGraphService.UpdateGraph();
        stage();

        m_Environment.m_VitalityDetailsService.Update();
        stage();
        m_Environment.m_ZoneDetailsService.Update();
        stage();
        m_Environment.m_TeleporterDetailsService.Update();
        stage();
        m_Environment.m_LiftDetailsService.Update();
        stage();
        m_Environment.m_BarrierDetailsService.Update();
        stage();
        m_Environment.m_ProjectileDetailsService.Update();
        stage();
        m_Environment.m_EntitiesService.Update();
        stage();
        m_Environment.m_PlayersService.Update();
        stage();
        m_Environment.m_ObjectiveDetailsService.Update();
        stage();
        m_Environment.m_AimService.Update();
        stage();
        m_Environment.m_EventsService.Update();
        stage();
        m_Environment.m_MatchService.Update();
        stage();

        m_Egocentric.m_SelfService.Update();
        stage();
        m_Egocentric.m_AffordanceService.Update();
        stage();
        m_Egocentric.m_RaycastService.Update();
        stage();

        m_TickService.Assemble(generation);
        stage();
    }
}