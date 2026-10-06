module;

#include "External/imgui/imgui.h"

module Viewer.Hud.System;
import :Canvas;

import Viewer.Camera.System;
import Viewer.Camera.Type;
import Viewer.Options.State;
import Viewer.Options.Type;
import Viewer.Style.Type;
import Viewer.Hud.Type;
import std;

namespace
{
    using Viewer::Options::Type::Scalar;
    using Viewer::Hud::Type::k_NoLabelKey;

    constexpr float k_ScreenMargin{ 48.0f };
    constexpr float k_PaddingX{ 6.0f };
    constexpr float k_PaddingY{ 3.0f };
    constexpr float k_RowSpacing{ 1.0f };
    constexpr float k_DividerHeight{ 6.0f };
    constexpr float k_StemLength{ 10.0f };
    constexpr float k_AccentHeight{ 2.0f };
    constexpr float k_FarFadeStart{ 0.6f };
    constexpr float k_FarFadeMinimum{ 0.35f };
    constexpr float k_ArrowHeadLength{ 10.0f };
    constexpr float k_ArrowHeadHalfWidth{ 5.0f };
    constexpr float k_ArrowMinimumLength{ 4.0f };
    constexpr float k_DashLength{ 9.0f };
    constexpr float k_DashGap{ 6.0f };
    constexpr float k_DeclutterGap{ 3.0f };
    constexpr int k_DeclutterIterations{ 24 };
    constexpr std::size_t k_MaxBars{ 256 };
    constexpr float k_BarPlainHeight{ 6.0f };
    constexpr float k_BarGap{ 2.0f };

    auto WithAlpha(ImU32 color, float factor) -> ImU32
    {
        const float alpha = static_cast<float>((color >> IM_COL32_A_SHIFT) & 0xFF) * factor;
        const ImU32 clamped = static_cast<ImU32>(std::clamp(alpha, 0.0f, 255.0f));

        return (color & ~IM_COL32_A_MASK) | (clamped << IM_COL32_A_SHIFT);
    }

    auto Overlaps(const ImVec2& minA, const ImVec2& maxA, const ImVec2& minB, const ImVec2& maxB) -> bool
    {
        return minA.x < maxB.x && maxA.x > minB.x && minA.y < maxB.y && maxA.y > minB.y;
    }
}

namespace Viewer::Hud::System
{
    auto ToImColor(const Viewer::Style::Type::Color& color, float alpha) -> ImU32
    {
        return ImGui::ColorConvertFloat4ToU32(ImVec4(color.R, color.G, color.B, alpha));
    }

    auto DistanceBetween(const Common::Math::Type::Vec3& a, const Common::Math::Type::Vec3& b) -> float
    {
        const float dx = a.X - b.X;
        const float dy = a.Y - b.Y;
        const float dz = a.Z - b.Z;

        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    auto ShortTagName(const std::string& tagName) -> std::string
    {
        const std::size_t cut = tagName.find_last_of("\\/");
        return cut == std::string::npos ? tagName : tagName.substr(cut + 1);
    }

    auto DistanceRow(const Viewer::Hud::Type::LabelContext& context,
        const Common::Math::Type::Vec3& point, ImU32 color) -> Viewer::Hud::Type::LabelRow
    {
        const auto distance = context.Frame.Self ? context.Frame.Self->DistanceTo(point) : std::nullopt;

        return Viewer::Hud::Type::LabelRow{ distance ? std::format("d: {:.1f} wu", *distance) : std::string{ "d: n/a" }, color };
    }

    auto LabelCanvas::Reset() -> void
    {
        m_Entries.clear();
        m_Arrows.clear();
        m_Bars.clear();
        m_KeyIndex.clear();
        m_Eye = m_Camera.GetEye();
    }

    auto LabelCanvas::GetEye() const -> const Vec3&
    {
        return m_Eye;
    }

    auto LabelCanvas::Project(const Vec3& world, ImVec2& outScreen) const -> bool
    {
        Viewer::Camera::Type::Vec2 pixel{};
        if (!m_Camera.ToScreen(world, pixel)) return false;

        outScreen = ImVec2(pixel.X, pixel.Y);
        return true;
    }

    auto LabelCanvas::Add(const Vec3& world, std::vector<LabelRow> rows, ImU32 accent,
        float priority, std::uint32_t key) -> bool
    {
        if (rows.empty()) return false;

        if (key != k_NoLabelKey)
        {
            const auto existing = m_KeyIndex.find(key);
            if (existing != m_KeyIndex.end())
            {
                Entry& entry = m_Entries[existing->second];

                entry.Rows.push_back(LabelRow{ {}, 0, true });
                for (LabelRow& row : rows) entry.Rows.push_back(std::move(row));

                entry.Priority = (std::max)(entry.Priority, priority);
                return true;
            }
        }

        const float distance = DistanceBetween(world, m_Eye);
        if (distance > m_Options.GetScalar(Scalar::LabelMaxDistance)) return false;

        ImVec2 screen{};
        if (!this->Project(world, screen)) return false;

        const auto& viewport = m_Camera.GetViewport();
        if (screen.x < viewport.Position.X - k_ScreenMargin ||
            screen.y < viewport.Position.Y - k_ScreenMargin ||
            screen.x > viewport.Position.X + viewport.Size.X + k_ScreenMargin ||
            screen.y > viewport.Position.Y + viewport.Size.Y + k_ScreenMargin)
        {
            return false;
        }

        Entry entry{};
        entry.Screen = screen;
        entry.Distance = distance;
        entry.Priority = priority;
        entry.Accent = accent;
        entry.Rows = std::move(rows);

        m_Entries.push_back(std::move(entry));
        if (key != k_NoLabelKey) m_KeyIndex[key] = m_Entries.size() - 1;

        return true;
    }

    auto LabelCanvas::AddArrow(const Vec3& from, const Vec3& to, ImU32 color, float thickness, bool dashed) -> bool
    {
        if (DistanceBetween(from, m_Eye) > m_Options.GetScalar(Scalar::LabelMaxDistance)) return false;

        Arrow arrow{};
        if (!this->Project(from, arrow.From) || !this->Project(to, arrow.To)) return false;

        const float dx = arrow.To.x - arrow.From.x;
        const float dy = arrow.To.y - arrow.From.y;
        if (dx * dx + dy * dy < k_ArrowMinimumLength * k_ArrowMinimumLength) return false;

        arrow.Color = color;
        arrow.Thickness = thickness;
        arrow.Dashed = dashed;

        m_Arrows.push_back(arrow);
        return true;
    }

    auto LabelCanvas::AddBar(const Vec3& world, float fraction, ImU32 color,
        std::string text, float priority) -> bool
    {
        const float distance = DistanceBetween(world, m_Eye);
        if (distance > m_Options.GetScalar(Scalar::LabelMaxDistance)) return false;

        ImVec2 screen{};
        if (!this->Project(world, screen)) return false;

        const auto& viewport = m_Camera.GetViewport();
        if (screen.x < viewport.Position.X - k_ScreenMargin ||
            screen.y < viewport.Position.Y - k_ScreenMargin ||
            screen.x > viewport.Position.X + viewport.Size.X + k_ScreenMargin ||
            screen.y > viewport.Position.Y + viewport.Size.Y + k_ScreenMargin)
        {
            return false;
        }

        Bar bar{};
        bar.Screen = screen;
        bar.Distance = distance;
        bar.Priority = priority;
        bar.Fraction = std::clamp(fraction, 0.0f, 1.0f);
        bar.Color = color;
        bar.Text = std::move(text);

        m_Bars.push_back(std::move(bar));
        return true;
    }

    auto LabelCanvas::Flush(ImDrawList& drawList) -> void
    {
        for (const Arrow& arrow : m_Arrows)
        {
            const float dx = arrow.To.x - arrow.From.x;
            const float dy = arrow.To.y - arrow.From.y;
            const float length = std::sqrt(dx * dx + dy * dy);
            const ImVec2 dir(dx / length, dy / length);
            const ImVec2 normal(-dir.y, dir.x);

            const float headLength = (std::min)(k_ArrowHeadLength, length);
            const ImVec2 baseCenter(arrow.To.x - dir.x * headLength, arrow.To.y - dir.y * headLength);

            if (arrow.Dashed)
            {
                const float bodyLength = (std::max)(length - headLength, 0.0f);

                for (float at = 0.0f; at < bodyLength; at += k_DashLength + k_DashGap)
                {
                    const float end = (std::min)(at + k_DashLength, bodyLength);

                    drawList.AddLine(
                        ImVec2(arrow.From.x + dir.x * at, arrow.From.y + dir.y * at),
                        ImVec2(arrow.From.x + dir.x * end, arrow.From.y + dir.y * end),
                        arrow.Color, arrow.Thickness);
                }
            }
            else
            {
                drawList.AddLine(arrow.From, baseCenter, arrow.Color, arrow.Thickness);
            }
            drawList.AddTriangleFilled(arrow.To,
                ImVec2(baseCenter.x + normal.x * k_ArrowHeadHalfWidth, baseCenter.y + normal.y * k_ArrowHeadHalfWidth),
                ImVec2(baseCenter.x - normal.x * k_ArrowHeadHalfWidth, baseCenter.y - normal.y * k_ArrowHeadHalfWidth),
                arrow.Color);
        }

        if (!m_Bars.empty())
        {
            std::sort(m_Bars.begin(), m_Bars.end(), [](const Bar& a, const Bar& b) {
                if (a.Priority != b.Priority) return a.Priority > b.Priority;
                return a.Distance < b.Distance;
            });

            if (m_Bars.size() > k_MaxBars) m_Bars.resize(k_MaxBars);

            const float barMaxDistance = m_Options.GetScalar(Scalar::LabelMaxDistance);
            const float barScale = m_Options.GetScalar(Scalar::LabelTextScale);
            const float barWidth = m_Options.GetScalar(Scalar::HealthBarWidth) * barScale;

            ImFont* barFont = ImGui::GetFont();
            const float barFontSize = ImGui::GetFontSize() * barScale * 0.85f;

            std::vector<std::pair<ImVec2, ImVec2>> barRects{};
            barRects.reserve(m_Bars.size());

            for (const Bar& bar : m_Bars)
            {
                const float height = bar.Text.empty() ? k_BarPlainHeight * barScale : barFontSize + 2.0f;

                ImVec2 minPoint(bar.Screen.x - barWidth * 0.5f, bar.Screen.y - height * 0.5f);
                ImVec2 maxPoint(minPoint.x + barWidth, minPoint.y + height);

                for (int i = 0; i < k_DeclutterIterations; ++i)
                {
                    bool moved{ false };

                    for (const auto& other : barRects)
                    {
                        if (!Overlaps(minPoint, maxPoint, other.first, other.second)) continue;

                        minPoint.y = other.second.y + k_BarGap;
                        maxPoint.y = minPoint.y + height;
                        moved = true;
                        break;
                    }

                    if (!moved) break;
                }

                barRects.emplace_back(minPoint, maxPoint);
            }

            for (std::size_t i = m_Bars.size(); i-- > 0;)
            {
                const Bar& bar = m_Bars[i];
                const auto& rect = barRects[i];

                const float fadeSpan = (std::max)(barMaxDistance * (1.0f - k_FarFadeStart), 1e-3f);
                const float fadeT = std::clamp(
                    (bar.Distance - barMaxDistance * k_FarFadeStart) / fadeSpan, 0.0f, 1.0f);
                const float fade = 1.0f - (1.0f - k_FarFadeMinimum) * fadeT;

                drawList.AddRectFilled(rect.first, rect.second, IM_COL32(12, 14, 18, static_cast<int>(200.0f * fade)), 2.0f);

                const float fillRight = rect.first.x + (rect.second.x - rect.first.x) * bar.Fraction;
                if (bar.Fraction > 0.0f)
                {
                    drawList.AddRectFilled(rect.first, ImVec2(fillRight, rect.second.y), WithAlpha(bar.Color, fade), 2.0f);
                }

                drawList.AddRect(rect.first, rect.second, IM_COL32(255, 255, 255, static_cast<int>(90.0f * fade)), 2.0f);

                if (!bar.Text.empty())
                {
                    const ImVec2 size = barFont->CalcTextSizeA(barFontSize, FLT_MAX, 0.0f, bar.Text.c_str());
                    const ImVec2 position((rect.first.x + rect.second.x - size.x) * 0.5f,
                        (rect.first.y + rect.second.y - size.y) * 0.5f);

                    drawList.AddText(barFont, barFontSize, ImVec2(position.x + 1.0f, position.y + 1.0f),
                        IM_COL32(0, 0, 0, static_cast<int>(220.0f * fade)), bar.Text.c_str());
                    drawList.AddText(barFont, barFontSize, position,
                        IM_COL32(255, 255, 255, static_cast<int>(255.0f * fade)), bar.Text.c_str());
                }
            }
        }

        if (m_Entries.empty()) return;

        std::sort(m_Entries.begin(), m_Entries.end(), [](const Entry& a, const Entry& b) {
            if (a.Priority != b.Priority) return a.Priority > b.Priority;
            return a.Distance < b.Distance;
        });

        const std::size_t maxCount = static_cast<std::size_t>(
            m_Options.GetScalar(Scalar::LabelMaxCount));
        if (m_Entries.size() > maxCount) m_Entries.resize(maxCount);

        const float maxDistance = m_Options.GetScalar(Scalar::LabelMaxDistance);
        const float backgroundAlpha = m_Options.GetScalar(Scalar::LabelBackgroundAlpha);
        const float textScale = m_Options.GetScalar(Scalar::LabelTextScale);

        ImFont* font = ImGui::GetFont();
        const float fontSize = ImGui::GetFontSize() * textScale;
        const float rowHeight = fontSize + k_RowSpacing;

        std::vector<std::pair<ImVec2, ImVec2>> placed{};
        placed.reserve(m_Entries.size());

        for (Entry& entry : m_Entries)
        {
            float width{ 0.0f };
            float height{ 0.0f };

            for (const LabelRow& row : entry.Rows)
            {
                if (row.IsDivider)
                {
                    height += k_DividerHeight;
                    continue;
                }

                const ImVec2 size = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, row.Text.c_str());
                width = (std::max)(width, size.x);
                height += rowHeight;
            }

            height -= k_RowSpacing;

            const float boxWidth = width + k_PaddingX * 2.0f;
            const float boxHeight = height + k_PaddingY * 2.0f + k_AccentHeight;

            ImVec2 boxMin(entry.Screen.x - boxWidth * 0.5f, entry.Screen.y - boxHeight - k_StemLength);
            ImVec2 boxMax(boxMin.x + boxWidth, boxMin.y + boxHeight);

            for (int i = 0; i < k_DeclutterIterations; ++i)
            {
                bool moved{ false };

                for (const auto& other : placed)
                {
                    if (!Overlaps(boxMin, boxMax, other.first, other.second)) continue;

                    boxMax.y = other.first.y - k_DeclutterGap;
                    boxMin.y = boxMax.y - boxHeight;
                    moved = true;
                    break;
                }

                if (!moved) break;
            }

            entry.BoxMin = boxMin;
            entry.BoxMax = boxMax;
            placed.emplace_back(boxMin, boxMax);
        }

        for (auto it = m_Entries.rbegin(); it != m_Entries.rend(); ++it)
        {
            const Entry& entry = *it;

            const float fadeSpan = (std::max)(maxDistance * (1.0f - k_FarFadeStart), 1e-3f);
            const float fadeT = std::clamp(
                (entry.Distance - maxDistance * k_FarFadeStart) / fadeSpan, 0.0f, 1.0f);
            const float fade = 1.0f - (1.0f - k_FarFadeMinimum) * fadeT;

            const ImVec2& boxMin = entry.BoxMin;
            const ImVec2& boxMax = entry.BoxMax;

            drawList.AddRectFilled(boxMin, boxMax,
                IM_COL32(12, 14, 18, static_cast<int>(255.0f * backgroundAlpha * fade)), 4.0f);
            drawList.AddRectFilled(boxMin, ImVec2(boxMax.x, boxMin.y + k_AccentHeight),
                WithAlpha(entry.Accent, fade), 4.0f, ImDrawFlags_RoundCornersTop);

            drawList.AddLine(ImVec2(entry.Screen.x, boxMax.y), entry.Screen, WithAlpha(entry.Accent, fade), 1.0f);
            drawList.AddCircleFilled(entry.Screen, 2.5f, WithAlpha(entry.Accent, fade));

            float cursorY = boxMin.y + k_AccentHeight + k_PaddingY;
            for (const LabelRow& row : entry.Rows)
            {
                if (row.IsDivider)
                {
                    const float lineY = cursorY + (k_DividerHeight - k_RowSpacing) * 0.5f;
                    drawList.AddLine(ImVec2(boxMin.x + k_PaddingX, lineY), ImVec2(boxMax.x - k_PaddingX, lineY),
                        WithAlpha(entry.Accent, 0.5f * fade), 1.0f);

                    cursorY += k_DividerHeight;
                    continue;
                }

                const ImVec2 size = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, row.Text.c_str());
                const ImVec2 position((boxMin.x + boxMax.x - size.x) * 0.5f, cursorY);

                drawList.AddText(font, fontSize, position, WithAlpha(row.Color, fade), row.Text.c_str());
                cursorY += rowHeight;
            }
        }
    }
}