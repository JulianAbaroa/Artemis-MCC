export module Platform.Layer;

import Service.Layer;
import Platform.Lifecycle.State;
import Platform.Lifecycle.System;
import Platform.Lifecycle.Hook;
import Platform.Input.Hook;
import Platform.Input.State;
import Platform.Input.System;
import Platform.Memory.State;
import Platform.Memory.System;
import Platform.Render.Hook;
import Platform.Render.State;
import Platform.Render.System;
import Platform.Render.Type;
import std;

export namespace Platform
{
	class Layer
	{
	private:
		using LifecycleStore = Platform::Lifecycle::State::LifecycleStore;
		using InputStore = Platform::Input::State::InputStore;
		using MouseDeltaStore = Platform::Input::State::MouseDeltaStore;
		using MemoryScannerStore = Platform::Memory::State::MemoryScannerStore;
		using RenderStore = Platform::Render::State::RenderStore;

		using LifecycleService = Platform::Lifecycle::System::LifecycleService;
		using InputService = Platform::Input::System::InputService;
		using AOBService = Platform::Memory::System::AOBService;
		using MemoryReaderService = Platform::Memory::System::MemoryReaderService;
		using MemoryScannerService = Platform::Memory::System::MemoryScannerService;
		using RenderService = Platform::Render::System::RenderService;
		using SwapChainLocator = Platform::Render::System::SwapChainLocator;

		using EngineInitializeDetour = Platform::Lifecycle::Hook::EngineInitializeDetour;
		using DestroySubsystemsDetour = Platform::Lifecycle::Hook::DestroySubsystemsDetour;
		using GetButtonStateDetour = Platform::Input::Hook::GetButtonStateDetour;
		using GetRawInputDataDetour = Platform::Input::Hook::GetRawInputDataDetour;
		using WndProcDetour = Platform::Input::Hook::WndProcDetour;
		using PresentDetour = Platform::Render::Hook::PresentDetour;
		using ResizeBuffersDetour = Platform::Render::Hook::ResizeBuffersDetour;

		using FrameContext = Platform::Render::Type::FrameContext;

		using MilliSeconds = std::chrono::milliseconds;

	public:
		explicit Layer(Service::Layer& service) :
			m_LifecycleService(service.m_LogsService, m_LifecycleStore),
			m_InputService(service.m_LogsService, m_InputStore, m_MouseDeltaStore),
			m_AOBService(service.m_LogsService),
			m_MemoryScannerService(service.m_LogsService, m_MemoryScannerStore),
			m_RenderService(service.m_LogsService, m_RenderStore),
			m_SwapChainLocator(service.m_LogsService),
			m_EngineInitializeDetour(service.m_LogsService, m_AOBService, m_LifecycleStore, m_LifecycleService),
			m_DestroySubsystemsDetour(service.m_LogsService, m_AOBService, m_LifecycleStore, m_LifecycleService),
			m_GetButtonStateDetour(service.m_LogsService, m_AOBService, m_InputStore),
			m_GetRawInputDataDetour(service.m_LogsService, service.m_SettingsStore, m_InputService, m_MouseDeltaStore),
			m_WndProcDetour(service.m_LogsService, m_InputService, m_LifecycleStore, m_LifecycleService),
			m_PresentDetour(service.m_LogsService, service.m_TelemetryStore, m_RenderStore, m_RenderService, m_SwapChainLocator),
			m_ResizeBuffersDetour(service.m_LogsService, m_RenderStore, m_RenderService, m_SwapChainLocator)
		{
			m_LifecycleService.OnEngineInitialized([this] {
				m_GetButtonStateDetour.Install();
			});

			m_LifecycleService.OnUnhook([this] {
				m_GetButtonStateDetour.Uninstall();
			});

			m_LifecycleService.OnCleanup([this] {
				m_InputStore.Cleanup();
				m_MouseDeltaStore.Cleanup();
			});

			m_RenderService.OnInitialized([this](const FrameContext& frame) {
				m_WndProcDetour.Install(frame.Window);
				m_InputService.BindWindow(frame.Window);
			});

			m_LifecycleService.OnShutdown([this] {
				this->ShutdownRenderAndInput();
			});
		}
		~Layer() = default;

		Layer(const Layer&) = delete;
		Layer& operator=(const Layer&) = delete;

		// --- State ---
		LifecycleStore m_LifecycleStore;
		InputStore m_InputStore;
		MouseDeltaStore m_MouseDeltaStore;
		MemoryScannerStore m_MemoryScannerStore;
		RenderStore m_RenderStore;

		// --- System ---
		LifecycleService m_LifecycleService;
		InputService m_InputService;
		AOBService m_AOBService;
		MemoryReaderService m_MemoryReaderService;
		MemoryScannerService m_MemoryScannerService;
		RenderService m_RenderService;
		SwapChainLocator m_SwapChainLocator;

		// --- Hook ---
		EngineInitializeDetour m_EngineInitializeDetour;
		DestroySubsystemsDetour m_DestroySubsystemsDetour;
		GetButtonStateDetour m_GetButtonStateDetour;
		GetRawInputDataDetour m_GetRawInputDataDetour;
		WndProcDetour m_WndProcDetour;
		PresentDetour m_PresentDetour;
		ResizeBuffersDetour m_ResizeBuffersDetour;

	private:
		static constexpr MilliSeconds k_RenderShutdownTimeout{ 500 };

		auto ShutdownRenderAndInput() -> void
		{
			m_WndProcDetour.Uninstall();
			m_GetRawInputDataDetour.Uninstall();

			m_RenderService.RequestShutdown();
			m_RenderService.WaitForShutdown(k_RenderShutdownTimeout);

			m_PresentDetour.Uninstall();
			m_ResizeBuffersDetour.Uninstall();

			m_RenderService.Shutdown();
		}
	};
}