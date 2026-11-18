module;

#include <d3d11.h>
#include <wrl/client.h>

export module DevTools.Viewer.Map.System:ViewPass;

import Common.Log.System;
import Common.Math.Type;
import Engine.Render.System;
import Views.View.Type;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Map.Type;
import std;

export namespace DevTools::Viewer::Map::System
{
    // Draws the View of the watched player as lines: the cone of sight, a line to each player it sees, markers for the allies it always
    // knows, the two flags, the radar points and the sounds. Colors depend on the relation to the watched player.
    // note: Everything comes from the View, except the grey markers of the players that the View does not contain: they come from the Tick,
    // to compare by eye with what the game shows, and are used for nothing else.
    class ViewPass
    {
    private:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        using Log = Common::Log::System::Log;

        using GpuPipeline = Engine::Render::System::GpuPipeline;

        using Vec3 = Common::Math::Type::Vec3;
        using Tick = Bridge::Codec::Tick::Type::Tick;
        using View = Views::View::Type::View;

    public:
        explicit ViewPass(Log& log) : m_Log(log) {}
        ~ViewPass() = default;

        ViewPass(const ViewPass&) = delete;
        auto operator=(const ViewPass&) -> ViewPass& = delete;

        // Rebuilds the vertices when the generation or the watched player change.
        // param view: The View of the watched player. Null draws nothing.
        // param tick: Where the players outside the View are placed. Null draws no grey markers.
        // param eyeOffset: How far the overlay draws the biped of the watched player from where the scene shows it (VW.5e). The cone, the lines to
        // the players it sees and the sound arrows start from the eye moved by it, and a frame with another offset is uploaded again. What the View
        // sees (players, flags, radar) stays where the scene shows it.
        auto Upload(ID3D11Device* device, ID3D11DeviceContext* context,
            const View* view, const Tick* tick, std::uint64_t generation, const Vec3& eyeOffset) -> void;

        auto Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void;

        auto Release() -> void;

    private:
        Log& m_Log;

        ComPtr<ID3D11Buffer> m_VertexBuffer{};
        UINT m_Capacity{ 0 };
        UINT m_VertexCount{ 0 };

        Engine::Render::System::UploadGate m_Gate{};
    };
}