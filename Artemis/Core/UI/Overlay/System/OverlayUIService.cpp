module;

#include "External/imgui/imgui.h"

module UI.Overlay.System;

import Viewer.Overlay.Type;
import std;

namespace
{
	using Mode = Viewer::Overlay::Type::Mode;

	using Viewer::Selection::State::k_NoSelection;

	const ImVec4 k_Green{ 0.0f, 1.0f, 0.0f, 1.0f };
	const ImVec4 k_Orange{ 1.0f, 0.6f, 0.0f, 1.0f };
	const ImVec4 k_Red{ 1.0f, 0.0f, 0.0f, 1.0f };
	const ImVec4 k_Info{ 0.6f, 0.8f, 1.0f, 1.0f };

	auto ModeLabel(Mode mode) -> const char*
	{
		switch (mode)
		{
		case Mode::Default:    return "Telemetry";
		case Mode::Collidable: return "Collidable";
		case Mode::Health:     return "Health";
		case Mode::Fixture:    return "Fixture";
		case Mode::Affordance: return "Affordance";
		default:               return "?";
		}
	}
}

namespace UI::Overlay::System
{
	auto OverlayUIService::Draw() -> void
	{
		if (!m_OverlayVisibilityStore.IsVisible()) return;

		ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f),
			ImGuiCond_Always, ImVec2(0.0f, 0.0f));

		ImGui::SetNextWindowBgAlpha(0.2f);

		const ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoInputs |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoNav |
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_AlwaysAutoResize;

		if (ImGui::Begin("##overlay", nullptr, flags))
		{
			this->DrawNavBar();
			ImGui::Separator();

			if (m_OverlayStore.GetMode() == Mode::Default)
			{
				this->DrawDefault();
			}
			else
			{
				this->DrawSelectedPanel(m_TickStore.Acquire());
			}
		}

		ImGui::End();
	}

	auto OverlayUIService::DrawNavBar() -> void
	{
		ImGui::TextDisabled("<");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.45f, 0.78f, 0.96f, 1.0f), "%s",
			ModeLabel(m_OverlayStore.GetMode()));
		ImGui::SameLine();
		ImGui::TextDisabled(">");
	}

	auto OverlayUIService::DrawDefault() -> void
	{
		this->DrawFramerate();
		this->DrawTelemetry();
	}

	auto OverlayUIService::DrawSelectedPanel(const std::shared_ptr<const Tick>& tick) -> void
	{
		const std::uint32_t handle = m_SelectionStore.GetSelected();

		if (handle == k_NoSelection)
		{
			ImGui::TextDisabled("No selection.");
			m_LastHandle = k_NoSelection;
			return;
		}

		if (handle != m_LastHandle)
		{
			m_OverlayStore.ResetPage();
			m_LastHandle = handle;
		}

		const float screenHeight = static_cast<float>(m_RenderStore.GetHeight());
		const float pageHeight = screenHeight * 0.5f;

		ImGui::BeginChild("##panel_body", ImVec2(0.0f, pageHeight), false,
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		if (tick) this->DrawPanel(*tick, handle);
		else      ImGui::TextDisabled("No data.");

		const float contentHeight = ImGui::GetCursorPosY();

		int totalPages = 1;
		if (pageHeight > 0.0f)
		{
			totalPages = static_cast<int>(std::ceil(contentHeight / pageHeight));
		}
		if (totalPages < 1) totalPages = 1;

		m_OverlayStore.ClampPage(totalPages - 1);
		const int page = m_OverlayStore.GetPage();
		ImGui::SetScrollY(page * pageHeight);

		ImGui::EndChild();

		if (totalPages > 1)
		{
			ImGui::Separator();
			ImGui::TextDisabled("Page %d/%d  (Up/Down)", page + 1, totalPages);
		}
	}

	auto OverlayUIService::DrawPanel(const Tick& tick, std::uint32_t handle) -> void
	{
		switch (m_OverlayStore.GetMode())
		{
		case Mode::Collidable:
			CollidablesUI::Draw(tick, handle);
			break;

		case Mode::Health:
			VitalitiesUI::Draw(tick, handle, m_VitalityStore);
			break;

		case Mode::Fixture:
			FixturesUI::Draw(tick, handle);
			break;

		case Mode::Affordance:
			AffordancesUI::Draw(tick, handle);
			break;

		default:
			break;
		}
	}

	auto OverlayUIService::DrawFramerate() -> void
	{
		const float framerate = ImGui::GetIO().Framerate;
		const int fps = static_cast<int>(framerate + 0.5f);

		const ImVec4 color = fps >= 45 ? k_Green : fps >= 30 ? k_Orange : k_Red;

		ImGui::Text("Framerate:");
		ImGui::SameLine();
		ImGui::TextColored(color, "%d", fps);
	}

	auto OverlayUIService::DrawTelemetry() -> void
	{
		auto& telemetry = m_TelemetryStore;

		const float tickHz = telemetry.m_TickHz.load(std::memory_order_relaxed);
		const float presentHz = telemetry.m_PresentHz.load(std::memory_order_relaxed);
		const float averageTickMs = telemetry.m_SimMs.load(std::memory_order_relaxed);
		const float averageSweepMs = telemetry.m_SweepMs.load(std::memory_order_relaxed);
		const std::uint32_t dropped = telemetry.m_DroppedOut.load(std::memory_order_relaxed);

		ImGui::Separator();

		ImGui::Text("Tick:");
		ImGui::SameLine();
		ImGui::TextColored(k_Info, "%.1f Hz", tickHz);

		ImGui::Text("Present:");
		ImGui::SameLine();
		ImGui::TextColored(k_Info, "%.1f Hz", presentHz);

		const float intervalMs = tickHz > 0.0f ? (1000.0f / tickHz) : 16.67f;
		const float budgetMs = (std::max)(intervalMs - averageTickMs, 0.0f);

		const ImVec4 sweepColor =
			(budgetMs > 0.0f && averageSweepMs < budgetMs * 0.5f) ? k_Green
			: (budgetMs > 0.0f && averageSweepMs < budgetMs * 0.9f) ? k_Orange
			: k_Red;

		ImGui::Text("Sweep:");
		ImGui::SameLine();
		ImGui::TextColored(sweepColor, "%.3f ms", averageSweepMs);
		ImGui::SameLine();
		ImGui::Text("/ budget %.2f ms", budgetMs);

		const ImVec4 dropColor = dropped == 0 ? k_Green : ImVec4(1.0f, 0.4f, 0.0f, 1.0f);

		ImGui::Text("Dropped:");
		ImGui::SameLine();
		ImGui::TextColored(dropColor, "%u /s", dropped);
	}
}