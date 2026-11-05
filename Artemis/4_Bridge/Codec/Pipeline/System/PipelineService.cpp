module Bridge.Codec.Pipeline.System;

import std;

namespace Bridge::Codec::Pipeline::System
{
    auto PipelineService::Run(const Snapshot& snapshot, std::uint64_t generation,
        const std::function<void()>& endStage, const std::function<bool()>& canPublish) -> void
    {
        // return: Whether the tick may go on. False once canPublish says no: the engine began tearing down, so the next stage would only
        // write stores the Cleanup is emptying.
        const auto stage{ [&endStage, &canPublish] {
            if (endStage) endStage();
            return !canPublish || canPublish();
        } };

        m_Tables.m_ObjectService.UpdateObjectTable(snapshot);
        if (!stage()) return;
        m_Tables.m_PlayerService.UpdatePlayerTable(snapshot);
        if (!stage()) return;
        m_Tables.m_InteractionService.UpdateInteractionTable(snapshot);
        if (!stage()) return;
        m_Tables.m_GameService.Update(snapshot);
        if (!stage()) return;

        m_Environment.m_ClassifierService.UpdateClassification();
        if (!stage()) return;
        m_Environment.m_ObjectGraphService.UpdateGraph();
        if (!stage()) return;

        m_Environment.m_VitalityDetailsService.Update();
        if (!stage()) return;
        m_Environment.m_ZoneDetailsService.Update();
        if (!stage()) return;
        m_Environment.m_TeleporterDetailsService.Update();
        if (!stage()) return;
        m_Environment.m_LiftDetailsService.Update();
        if (!stage()) return;
        m_Environment.m_BarrierDetailsService.Update();
        if (!stage()) return;
        m_Environment.m_ExplosiveDetailsService.Update();
        if (!stage()) return;
        m_Environment.m_ProjectileDetailsService.Update();
        if (!stage()) return;
        m_Environment.m_WeaponDetailsService.Update();
        if (!stage()) return;
        m_Environment.m_EntitiesService.Update();
        if (!stage()) return;
        m_Environment.m_PlayersService.Update();
        if (!stage()) return;
        m_Environment.m_ObjectiveDetailsService.Update();
        if (!stage()) return;
        m_Environment.m_AimService.Update();
        if (!stage()) return;
        m_Environment.m_EventsService.Update();
        if (!stage()) return;
        m_Environment.m_MatchService.Update();
        if (!stage()) return;

        m_Egocentric.m_SelfService.Update();
        if (!stage()) return;
        m_Egocentric.m_AffordanceService.Update();
        if (!stage()) return;
        m_Egocentric.m_RaycastService.Update();
        if (!stage()) return;

        m_TickService.Assemble(generation);
        stage();
    }
}