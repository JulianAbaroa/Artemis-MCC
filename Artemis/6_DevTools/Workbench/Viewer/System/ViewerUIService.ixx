export module DevTools.Workbench.Viewer.System;

import Services.Observed.State;
import Bridge.Codec.Tick.Type;
import Bridge.Codec.Tick.State;
import DevTools.Viewer.Options.Type;
import DevTools.Viewer.Options.State;
import DevTools.Viewer.Camera.State;
import DevTools.Viewer.Inspector.System;
import std;

namespace
{
    using DevTools::Viewer::Options::Type::k_LayerCount;
}

export namespace DevTools::Workbench::Viewer::System
{
    // Draws the Viewer tab: the state line, the preset, the camera, the layers with their level and the development options.
    class ViewerUIService
    {
    private:
        using ObservedPlayerStore = Services::Observed::State::ObservedPlayerStore;
        using TickStore = Bridge::Codec::Tick::State::TickStore;

        using Flag = DevTools::Viewer::Options::Type::Flag;
        using Group = DevTools::Viewer::Options::Type::Group;
        using Layer = DevTools::Viewer::Options::Type::Layer;
        using LayerInfo = DevTools::Viewer::Options::Type::LayerInfo;
        using ScalarInfo = DevTools::Viewer::Options::Type::ScalarInfo;
        using OptionsStore = DevTools::Viewer::Options::State::OptionsStore;
        using CameraStore = DevTools::Viewer::Camera::State::CameraStore;
        using InspectorService = DevTools::Viewer::Inspector::System::InspectorService;

    public:
        // Builds the tooltip of each layer from the flag table.
        ViewerUIService(OptionsStore& optionsStore, CameraStore& cameraStore, const ObservedPlayerStore& observedStore, TickStore& tickStore,
            InspectorService& inspectorService);
        ~ViewerUIService() = default;

        ViewerUIService(const ViewerUIService&) = delete;
        auto operator=(const ViewerUIService&) -> ViewerUIService& = delete;

        // Draws the content of the Viewer tab.
        auto Draw() -> void;

    private:
        OptionsStore& m_OptionsStore;
        CameraStore& m_CameraStore;
        const ObservedPlayerStore& m_ObservedStore;
        TickStore& m_TickStore;
        InspectorService& m_InspectorService;

        // What each level of each layer shows, in the order of Layer.
        std::array<std::string, k_LayerCount> m_LayerTooltips{};

        // Draws whether the Viewer is active, its mode and the player it follows.
        auto DrawState() -> void;

        // Draws the preset combo. Only Debug can be picked for now.
        auto DrawPreset() -> void;

        // Draws the Viewer switches (active, mode, zoom), the game HUD and first person switches and the camera scalars.
        // note: Active, mode and zoom are runtime states and are not saved with the preferences.
        auto DrawCamera() -> void;

        // Draws the labels switch and their scalars, then one row per layer.
        auto DrawLayers() -> void;

        // Draws the name of a layer with its tooltip, the selector of its level and, folded below, its own scalars.
        auto DrawLayerRow(const LayerInfo& layer) -> void;

        // Draws the three buttons Off, Minimal and Full of a layer, or a combo when the row is too narrow for them.
        auto DrawLevelSelector(Layer layer) -> void;

        // Draws the inspector of the selected object.
        auto DrawInspector() -> void;

        // Draws the development options, folded by default.
        auto DrawDevelopment() -> void;

        // Draws the checkbox of a flag that belongs to no layer.
        auto DrawFlag(Flag flag) -> void;

        // Draws the slider of every scalar of the group that belongs to the layer (Layer::Count for none).
        auto DrawScalars(Group group, Layer owner) -> void;

        // Draws the slider of a scalar. A logarithmic one also shows its log2 and goes back to its default with a double click.
        auto DrawScalar(const ScalarInfo& info) -> void;
    };
}