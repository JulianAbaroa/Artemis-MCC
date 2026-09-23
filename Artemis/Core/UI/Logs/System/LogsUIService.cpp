module;

#include "External/imgui/imgui.h"

module UI.Logs.System;

import std;

namespace
{
	using Level = Service::Logs::Type::Level;

	auto ToLower(std::string_view text) -> std::string
	{
		std::string out(text);

		std::ranges::transform(out, out.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		return out;
	}

	auto LevelColor(Level level) -> ImVec4
	{
		switch (level)
		{
		case Level::Error:   return ImVec4(1.0f, 0.33f, 0.33f, 1.0f);
		case Level::Warning: return ImVec4(1.0f, 0.79f, 0.23f, 1.0f);
		case Level::Info:    return ImVec4(0.4f, 0.8f, 0.4f, 1.0f);
		default:             return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}
}

namespace UI::Logs::System
{
	auto LogsUIService::Draw() -> void
	{
		const FilterState filter = this->DrawTopBar();

		ImGui::Separator();

		this->DrawScrollingRegion(filter);
	}

	auto LogsUIService::DrawTopBar() -> FilterState
	{
		this->DrawSearchBar();

		FilterState filter{};
		filter.LowerQuery = ToLower(m_LogsUIStore.GetSearchText());
		filter.IsFiltering = !filter.LowerQuery.empty();

		this->DrawClearButton(filter);

		ImGui::SameLine();

		this->DrawCopyButton(filter);

		ImGui::SameLine();

		bool isAutoScroll = m_SettingsStore.GetLogsAutoScroll();
		if (ImGui::Checkbox("Auto-Scroll", &isAutoScroll))
		{
			m_SettingsStore.SetLogsAutoScroll(isAutoScroll);
		}

		ImGui::SameLine();

		this->DrawHelpMarker();

		return filter;
	}

	auto LogsUIService::DrawSearchBar() -> void
	{
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Filter:");
		ImGui::SameLine();

		const float clearButtonWidth = 60.0f;
		const float availableWidth = ImGui::GetContentRegionAvail().x;

		ImGui::PushItemWidth(availableWidth - clearButtonWidth - ImGui::GetStyle().ItemSpacing.x);
		ImGui::InputTextWithHint("##log_filter", "Search for specific logs...",
			m_LogsUIStore.GetSearchBuffer(), m_LogsUIStore.GetSearchBufferSize());
		ImGui::PopItemWidth();

		ImGui::SameLine();

		if (ImGui::Button("Clear", ImVec2(clearButtonWidth, 0)))
		{
			m_LogsUIStore.ClearSearch();
		}
	}

	auto LogsUIService::DrawClearButton(const FilterState& filter) -> void
	{
		const char* label = filter.IsFiltering ? "Clear Filtered" : "Clear All Logs";

		if (!ImGui::Button(label)) return;

		if (filter.IsFiltering)
		{
			m_LogsService.RemoveLogsIf([&filter](const Entry& entry) {
				return Matches(entry, filter);
				});
		}
		else
		{
			m_LogsStore.ClearLogs();
		}

		m_LogsUIStore.ClearSelection();
	}

	auto LogsUIService::DrawCopyButton(const FilterState& filter) -> void
	{
		const char* label = filter.IsFiltering ? "Copy Filtered" : "Copy All";

		if (!ImGui::Button(label)) return;

		std::string output;

		m_LogsStore.ForEachLog([&](const Entry& entry) {
			if (!filter.IsFiltering || Matches(entry, filter))
			{
				output += entry.FullText + "\n";
			}
			});

		ImGui::SetClipboardText(output.c_str());
	}

	auto LogsUIService::DrawHelpMarker() -> void
	{
		const float helpIconWidth = ImGui::CalcTextSize("(?)").x;
		const float posX = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - helpIconWidth;
		ImGui::SetCursorPosX(posX);

		ImGui::TextDisabled("(?)");

		if (!ImGui::IsItemHovered()) return;

		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Log Controls:");
		ImGui::Separator();
		ImGui::BulletText("Buttons: Behavior changes based on current filter.");
		ImGui::BulletText("Left Click: Select a single line.");
		ImGui::BulletText("Shift + Left Click: Select a range.");
		ImGui::BulletText("Right Click: Copy selection or single line.");
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}

	auto LogsUIService::GetFilteredIndices(const FilterState& filter) const -> std::vector<int>
	{
		std::vector<int> filtered;
		if (!filter.IsFiltering) return filtered;

		int index = 0;

		m_LogsStore.ForEachLog([&](const Entry& entry) {
			if (Matches(entry, filter)) filtered.push_back(index);
			++index;
			});

		return filtered;
	}

	auto LogsUIService::DrawScrollingRegion(const FilterState& filter) -> void
	{
		if (!ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false,
			ImGuiWindowFlags_HorizontalScrollbar))
		{
			ImGui::EndChild();
			return;
		}

		const std::vector<int> filteredIndices = this->GetFilteredIndices(filter);
		const int totalLogs = static_cast<int>(m_LogsStore.GetTotalLogs());
		const int displayCount = filter.IsFiltering ?
			static_cast<int>(filteredIndices.size()) : totalLogs;

		ImGuiListClipper clipper;
		clipper.Begin(displayCount);

		bool isLogClicked = false;

		while (clipper.Step())
		{
			for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
			{
				const int realIndex = filter.IsFiltering ? filteredIndices[i] : i;
				const Entry entry = m_LogsStore.GetLogAt(static_cast<std::size_t>(realIndex));

				this->DrawLogLine(realIndex, entry, isLogClicked);
			}
		}

		const bool isBackgroundClicked = ImGui::IsWindowHovered() &&
			ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
			!ImGui::IsAnyItemHovered() && !isLogClicked;

		if (isBackgroundClicked) m_LogsUIStore.ClearSelection();

		if (m_SettingsStore.GetLogsAutoScroll() &&
			ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		{
			ImGui::SetScrollHereY(1.0f);
		}

		ImGui::EndChild();
	}

	auto LogsUIService::DrawLogLine(int realIndex, const Entry& entry,
		bool& isLogClicked) -> void
	{
		ImGui::PushID(realIndex);

		if (m_LogsUIStore.IsIndexSelected(realIndex))
		{
			const ImVec2 min = ImGui::GetCursorScreenPos();
			const ImVec2 max(min.x + ImGui::GetContentRegionAvail().x,
				min.y + ImGui::GetTextLineHeightWithSpacing());

			ImGui::GetWindowDrawList()->AddRectFilled(min, max, IM_COL32(66, 150, 250, 60));
		}

		ImGui::BeginGroup();

		ImGui::TextDisabled("%s", entry.Timestamp.c_str());
		ImGui::SameLine(0.0f, 0.0f);

		if (!entry.Tag.empty())
		{
			ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), " %s ", entry.Tag.c_str());
			ImGui::SameLine(0.0f, 0.0f);
		}

		if (!entry.MessagePrefix.empty())
		{
			ImGui::TextColored(LevelColor(entry.Level), "%s", entry.MessagePrefix.c_str());
			ImGui::SameLine(0.0f, 0.0f);
		}

		this->DrawLogMessage(entry.Message);

		ImGui::SameLine();
		ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
		ImGui::EndGroup();

		this->HandleLogInteraction(realIndex, entry, isLogClicked);

		ImGui::PopID();
	}

	auto LogsUIService::DrawLogMessage(const std::string& message) -> void
	{
		// The first 'quoted' fragment is highlighted.
		const std::size_t quoteStart = message.find('\'');
		const std::size_t quoteEnd = (quoteStart != std::string::npos) ?
			message.find('\'', quoteStart + 1) : std::string::npos;

		if (quoteStart == std::string::npos || quoteEnd == std::string::npos)
		{
			ImGui::TextUnformatted(message.c_str());
			return;
		}

		if (quoteStart > 0)
		{
			const std::string before = message.substr(0, quoteStart);
			ImGui::TextUnformatted(before.c_str());
			ImGui::SameLine(0.0f, 0.0f);
		}

		const std::string quoted = message.substr(quoteStart, quoteEnd - quoteStart + 1);
		ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s", quoted.c_str());

		if (quoteEnd + 1 < message.length())
		{
			const std::string after = message.substr(quoteEnd + 1);
			ImGui::SameLine(0.0f, 0.0f);
			ImGui::TextUnformatted(after.c_str());
		}
	}

	auto LogsUIService::HandleLogInteraction(int realIndex, const Entry& entry,
		bool& isLogClicked) -> void
	{
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			isLogClicked = true;

			if (ImGui::GetIO().KeyShift) m_LogsUIStore.ExtendSelection(realIndex);
			else                         m_LogsUIStore.SelectSingle(realIndex);
		}

		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
		{
			const float now = static_cast<float>(ImGui::GetTime());

			if (m_LogsUIStore.IsIndexSelected(realIndex))
			{
				std::string selectedText;

				for (int i = m_LogsUIStore.GetSelectionMin(); i <= m_LogsUIStore.GetSelectionMax(); ++i)
				{
					selectedText += m_LogsStore.GetLogAt(static_cast<std::size_t>(i)).FullText + "\n";
				}

				ImGui::SetClipboardText(selectedText.c_str());
				m_LogsUIStore.StartCopyAnimation(LogsUIStore::k_SelectionIndex, now);
			}
			else
			{
				ImGui::SetClipboardText(entry.FullText.c_str());
				m_LogsUIStore.StartCopyAnimation(realIndex, now);
			}
		}

		const int animatedIndex = m_LogsUIStore.GetAnimatedIndex();
		const bool isAnimatingSelection = animatedIndex == LogsUIStore::k_SelectionIndex &&
			m_LogsUIStore.IsIndexSelected(realIndex);

		if (animatedIndex != realIndex && !isAnimatingSelection) return;

		const float elapsed = static_cast<float>(ImGui::GetTime()) -
			m_LogsUIStore.GetAnimationStartTime();

		if (elapsed < k_AnimationDuration)
		{
			const float alpha = 1.0f - (elapsed / k_AnimationDuration);

			ImGui::GetWindowDrawList()->AddRectFilled(
				ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
				ImColor(1.0f, 1.0f, 1.0f, alpha * 0.3f));
		}
		else
		{
			m_LogsUIStore.StopCopyAnimation();
		}
	}

	auto LogsUIService::Matches(const Entry& entry, const FilterState& filter) -> bool
	{
		return ToLower(entry.FullText).find(filter.LowerQuery) != std::string::npos;
	}
}