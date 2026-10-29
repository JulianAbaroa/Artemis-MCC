module;

#include "External/imgui/imgui.h"

export module DevTools.Viewer.Hud.System:Canvas;

import Common.Engine.Type;
import Common.Math.Type;
import DevTools.Viewer.Camera.System;
import DevTools.Viewer.Options.State;
import DevTools.Viewer.Style.Type;
import DevTools.Viewer.Hud.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
}

export namespace DevTools::Viewer::Hud::System
{
    // return: The color packed for the ImGui draw lists.
    auto ToImColor(const DevTools::Viewer::Style::Type::Color& color, float alpha = 1.0f) -> ImU32;

    auto DistanceBetween(const Common::Math::Type::Vec3& a, const Common::Math::Type::Vec3& b) -> float;

    // return: The tag name without its folders.
    auto ShortTagName(const std::string& tagName) -> std::string;

    // return: A row with the distance from the local player to the point, or "n/a" if the player is dead.
    auto DistanceRow(const DevTools::Viewer::Hud::Type::LabelContext& context,
        const Common::Math::Type::Vec3& point, ImU32 color) -> DevTools::Viewer::Hud::Type::LabelRow;

    // return: The color of the team of the object, or the neutral color if it has no team or is not in the table.
    auto TeamColorOf(const DevTools::Viewer::Hud::Type::LabelContext& context, std::uint32_t handle) -> ImU32;

    // Collects labels, bars and arrows in world space during a frame and draws them together.
    // The collectors add to it and the HUD flushes it once.
    class LabelCanvas
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;

        using CameraService = DevTools::Viewer::Camera::System::CameraService;
        using OptionsStore = DevTools::Viewer::Options::State::OptionsStore;
        using LabelRow = DevTools::Viewer::Hud::Type::LabelRow;
        using Entry = DevTools::Viewer::Hud::Type::Entry;
        using Bar = DevTools::Viewer::Hud::Type::Bar;
        using Arrow = DevTools::Viewer::Hud::Type::Arrow;

    public:
        LabelCanvas(const CameraService& camera, const OptionsStore& options) :
            m_Camera(camera), m_Options(options) {}
        ~LabelCanvas() = default;

        LabelCanvas(const LabelCanvas&) = delete;
        auto operator=(const LabelCanvas&) -> LabelCanvas& = delete;

        // return: The options the canvas filters and scales with.
        auto GetOptions() const -> const OptionsStore&;

        // Clears everything collected and reads the eye position of the camera.
        auto Reset() -> void;

        // Adds a label anchored at a world position.
        // param group: Color of the semantic group. It fills the right part of the top bar and tints the stem.
        // param team: Color of the team. It fills the left part of the top bar.
        // param priority: Higher priority labels are placed first and survive the label limit.
        // param key: Labels with the same key are merged into one, with a divider between them.
        // return: False if the label was dropped. That happens if it has no rows, is too far or is off screen.
        auto Add(const Vec3& world, std::vector<LabelRow> rows, ImU32 group, ImU32 team,
            float priority = 0.0f, std::uint32_t key = k_NoHandle) -> bool;

        // Adds an arrow between two world positions.
        // return: False if the arrow was dropped. That happens if it is too far or too short on screen.
        auto AddArrow(const Vec3& from, const Vec3& to, ImU32 color,
            float thickness = 2.0f, bool dashed = false) -> bool;

        // Adds a bar filled up to the fraction, with optional text inside.
        // return: False if the bar was dropped. That happens if it is too far or off screen.
        auto AddBar(const Vec3& world, float fraction, ImU32 color,
            std::string text = {}, float priority = 0.0f) -> bool;

        // Draws the arrows, the bars and the labels, nearest and most important on top.
        // note: Overlapping labels and bars are moved so they do not cover each other.
        auto Flush(ImDrawList& drawList) -> void;

        // return: The eye position of the camera, read when the canvas was reset.
        auto GetEye() const -> const Vec3&;

    private:
        const CameraService& m_Camera;
        const OptionsStore& m_Options;

        Vec3 m_Eye{};
        std::vector<Entry> m_Entries{};
        std::vector<Arrow> m_Arrows{};
        std::vector<Bar> m_Bars{};
        std::unordered_map<std::uint32_t, std::size_t> m_KeyIndex{};

        // return: False if the point is behind the camera.
        auto Project(const Vec3& world, ImVec2& outScreen) const -> bool;
    };
}