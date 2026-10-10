export module Tables.Layer;

import Service.Layer;
import Platform.Layer;
import Map.Layer;
import Template.Layer;
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

		using InitRootNodeDetour = Tables::Object::Hook::InitRootNodeDetour;
		using PlayerTableLocator = Tables::Player::Hook::PlayerTableLocator;
		using InteractionTableLocator = Tables::Interaction::Hook::InteractionTableLocator;

	public:
		Layer(Service::Layer& service, Platform::Layer& platform, Map::Layer& map, Template::Layer& templateLayer) :
			m_ObjectService(service.m_LogsService, templateLayer.m_TemplateStore, m_ObjectStore, m_BoneOffsetsStore, m_BoneMatricesStore, m_DamageSectionsStore, map.m_TagResolverService),
			m_PlayerService(service.m_LogsService, platform.m_MemoryReaderService, m_PlayerStore),
			m_InteractionService(service.m_LogsService, platform.m_MemoryReaderService, m_InteractionStore),
			m_InitRootNodeDetour(service.m_LogsService, platform.m_AOBService, m_BoneOffsetsStore),
			m_PlayerTableLocator(service.m_LogsService, platform.m_AOBService, m_PlayerStore),
			m_InteractionTableLocator(service.m_LogsService, platform.m_AOBService, m_InteractionStore)
		{
			auto& lifecycle = platform.m_LifecycleService;

			lifecycle.OnEngineInitialized([this] {
				m_InitRootNodeDetour.Install();

				m_PlayerTableLocator.FindAndStoreTableBase();
				m_InteractionTableLocator.FindAndStoreTableBase();
			});

			lifecycle.OnUnhook([this] {
				m_InitRootNodeDetour.Uninstall();
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
		InitRootNodeDetour m_InitRootNodeDetour;
		PlayerTableLocator m_PlayerTableLocator;
		InteractionTableLocator m_InteractionTableLocator;
	};
}