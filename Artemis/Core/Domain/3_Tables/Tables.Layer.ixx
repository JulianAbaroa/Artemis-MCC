export module Tables.Layer;

import Service.Layer;
import Platform.Layer;
import Map.Layer;
import Tables.Object.State;
import Tables.Object.System;
import Tables.Object.Hook;
import Tables.Player.State;
import Tables.Player.System;
import Tables.Player.Hook;
import Tables.Interaction.State;
import Tables.Interaction.System;
import Tables.Interaction.Hook;

export namespace Tables
{
	class Layer
	{
	private:
		using ObjectTableStore = Tables::Object::State::ObjectTableStore;
		using BoneOffsetsStore = Tables::Object::State::BoneOffsetsStore;
		using BoneMatricesStore = Tables::Object::State::BoneMatricesStore;
		using DamageSectionsStore = Tables::Object::State::DamageSectionsStore;
		using PlayerTableStore = Tables::Player::State::PlayerTableStore;
		using InteractionStore = Tables::Interaction::State::InteractionStore;

		using ObjectTableService = Tables::Object::System::ObjectTableService;
		using PlayerTableService = Tables::Player::System::PlayerTableService;
		using InteractionService = Tables::Interaction::System::InteractionService;

		using ObjectTableLocator = Tables::Object::Hook::ObjectTableLocator;
		using CreateObjectDetour = Tables::Object::Hook::CreateObjectDetour;
		using ReleaseObjectDetour = Tables::Object::Hook::ReleaseObjectDetour;
		using InitRootNodeDetour = Tables::Object::Hook::InitRootNodeDetour;
		using PlayerTableLocator = Tables::Player::Hook::PlayerTableLocator;
		using CreatePlayerDetour = Tables::Player::Hook::CreatePlayerDetour;
		using InteractionTableLocator = Tables::Interaction::Hook::InteractionTableLocator;

	public:
		Layer(Service::Layer& service, Platform::Layer& platform, Map::Layer& map) :
			m_ObjectService(service.m_LogsService, platform.m_MemoryReaderService, map.m_TagStore.bipd, map.m_TagStore.bloc, map.m_TagStore.coll, map.m_TagStore.ctrl, map.m_TagStore.eqip, map.m_TagStore.hlmt, map.m_TagStore.lbsp, map.m_TagStore.mach, map.m_TagStore.mode, map.m_TagStore.phmo, map.m_TagStore.play, map.m_TagStore.proj, map.m_TagStore.sbsp, map.m_TagStore.scen, map.m_TagStore.scnr, map.m_TagStore.sldt, map.m_TagStore.vehi, map.m_TagStore.weap, map.m_TagStore.zone, m_ObjectStore, m_BoneOffsetsStore, m_BoneMatricesStore, m_DamageSectionsStore, map.m_TagResolverService),
			m_PlayerService(service.m_LogsService, platform.m_MemoryReaderService, m_PlayerStore),
			m_InteractionService(service.m_LogsService, platform.m_MemoryReaderService, m_InteractionStore),
			m_ObjectTableLocator(service.m_LogsService, platform.m_AOBService, m_ObjectStore),
			m_CreateObjectDetour(service.m_LogsService, platform.m_AOBService, m_ObjectService),
			m_ReleaseObjectDetour(service.m_LogsService, platform.m_AOBService, m_ObjectService),
			m_InitRootNodeDetour(service.m_LogsService, platform.m_AOBService, m_BoneOffsetsStore),
			m_PlayerTableLocator(service.m_LogsService, platform.m_AOBService, m_PlayerStore),
			m_CreatePlayerDetour(service.m_LogsService, platform.m_AOBService, m_PlayerService),
			m_InteractionTableLocator(service.m_LogsService, platform.m_AOBService, m_InteractionStore)
		{
			auto& lifecycle = platform.m_LifecycleService;

			lifecycle.OnEngineInitialized([this] {
				m_CreateObjectDetour.Install();
				m_ReleaseObjectDetour.Install();
				m_InitRootNodeDetour.Install();
				m_CreatePlayerDetour.Install();

				m_ObjectTableLocator.FindAndStoreTableBase();
				m_PlayerTableLocator.FindAndStoreTableBase();
				m_InteractionTableLocator.FindAndStoreTableBase();
			});

			lifecycle.OnUnhook([this] {
				m_CreateObjectDetour.Uninstall();
				m_ReleaseObjectDetour.Uninstall();
				m_InitRootNodeDetour.Uninstall();
				m_CreatePlayerDetour.Uninstall();
			});

			lifecycle.OnCleanup([this] {
				m_ObjectService.Cleanup();
				m_PlayerService.Cleanup();
				m_InteractionService.Cleanup();
			});
		}
		~Layer() = default;

		Layer(const Layer&) = delete;
		Layer& operator=(const Layer&) = delete;

		// --- State ---
		ObjectTableStore m_ObjectStore;
		BoneOffsetsStore m_BoneOffsetsStore;
		BoneMatricesStore m_BoneMatricesStore;
		DamageSectionsStore m_DamageSectionsStore;
		PlayerTableStore m_PlayerStore;
		InteractionStore m_InteractionStore;

		// --- System ---
		ObjectTableService m_ObjectService;
		PlayerTableService m_PlayerService;
		InteractionService m_InteractionService;

		// --- Hook ---
		ObjectTableLocator m_ObjectTableLocator;
		CreateObjectDetour m_CreateObjectDetour;
		ReleaseObjectDetour m_ReleaseObjectDetour;
		InitRootNodeDetour m_InitRootNodeDetour;
		PlayerTableLocator m_PlayerTableLocator;
		CreatePlayerDetour m_CreatePlayerDetour;
		InteractionTableLocator m_InteractionTableLocator;
	};
}