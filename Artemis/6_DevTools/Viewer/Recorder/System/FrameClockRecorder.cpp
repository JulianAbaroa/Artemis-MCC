module DevTools.Viewer.Recorder.System;

import Common.Math.Type;
import Common.Time.System;
import Engine.Base.Memory.Type;
import std;

namespace
{
    using Common::Math::Type::PoseSource;
    using Common::Math::Type::Vec3;
    using Common::Time::System::LocalNow;
    using Engine::Base::Memory::Type::FrameClock::ClockSource;
    using Engine::Base::Memory::Type::FrameClock::InterpolationState;

    // Subfolder of the Artemis folder of LOCALAPPDATA that holds the files.
    constexpr std::string_view k_Folder{ "FrameClock" };

    constexpr std::string_view k_Header{
        "qpc_us,mode,T,f,interp,S_newest,S_drawn,waited_us,timed_out,cam_x,cam_y,cam_z,fwd_x,fwd_y,fwd_z,eye_x,eye_y,eye_z,"
        "eye_prev_x,eye_prev_y,eye_prev_z,veh_x,veh_y,veh_z,"
        "interp_pose,eyed_x,eyed_y,eyed_z,vehd_x,vehd_y,vehd_z\n" };

    // Adds ",x,y,z", or three empty fields without a value.
    auto AppendVec3(std::string& line, const std::optional<Vec3>& value) -> void
    {
        if (value) std::format_to(std::back_inserter(line), ",{:.5f},{:.5f},{:.5f}", value->X, value->Y, value->Z);
        else line += ",,,";
    }

    auto AppendTick(std::string& line, const std::optional<std::uint32_t>& tick) -> void
    {
        line += ',';
        if (tick) std::format_to(std::back_inserter(line), "{}", *tick);
    }
}

namespace DevTools::Viewer::Recorder::System
{
    auto FrameClockRecorder::Header() -> std::string_view
    {
        return k_Header;
    }

    auto FrameClockRecorder::FormatRow(const FrameClockRow& row) -> std::string
    {
        std::string line{};
        line.reserve(256);

        const bool hasClock{ row.Clock.Source != ClockSource::None };
        const char* interpolation{ row.Clock.Interpolation == InterpolationState::On ? "yes"
            : row.Clock.Interpolation == InterpolationState::Off ? "no" : "?" };

        std::format_to(std::back_inserter(line), "{},{}", row.QpcUs, row.Mode);
        if (hasClock) std::format_to(std::back_inserter(line), ",{},{:.4f},{}", row.Clock.Tick, row.Clock.Fraction, interpolation);
        else line += ",,,";

        AppendTick(line, row.SceneNewest);
        AppendTick(line, row.SceneDrawn);
        std::format_to(std::back_inserter(line), ",{},{}", row.WaitedUs, row.IsTimedOut ? 1 : 0);

        AppendVec3(line, row.Camera);
        AppendVec3(line, row.Forward);
        AppendVec3(line, row.Eye);
        AppendVec3(line, row.EyePrevious);
        AppendVec3(line, row.Vehicle);

        line += row.Interp == PoseSource::Exact ? ",exact" : row.Interp == PoseSource::Extrapolated ? ",extrap" : ",none";
        AppendVec3(line, row.EyeRender);
        AppendVec3(line, row.VehicleRender);

        line += '\n';
        return line;
    }

    auto FrameClockRecorder::Update(bool isWanted) -> void
    {
        m_IsWanted = isWanted;

        if (!isWanted)
        {
            if (m_Store.IsOpen())
            {
                m_Store.Close();
                m_Log.Message("[FrameClockRecorder] INFO: Recording stopped.");
            }

            m_IsFailed = false;
        }
    }

    auto FrameClockRecorder::Open() -> bool
    {
        const std::string directory{ m_SettingsStore.GetAppDataDirectory() };
        if (directory.empty())
        {
            m_IsFailed = true;
            m_Log.Message("[FrameClockRecorder] ERROR: Storage Folder is not set.");
            return false;
        }

        // note: Milliseconds in the name, so a file opened again within the same second is never the one that was just closed.
        const auto milliseconds{ std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() % 1000 };
        const std::filesystem::path path{ std::filesystem::path(directory) / k_Folder /
            std::format("{:%Y%m%d_%H%M%S}_{:03}.csv", LocalNow(), milliseconds) };
        if (!m_Store.Open(path))
        {
            m_IsFailed = true;
            m_Log.Message("[FrameClockRecorder] ERROR: Failed to open the CSV file.");
            return false;
        }

        m_Store.Append(k_Header);
        m_Log.Message("[FrameClockRecorder] INFO: Recording to {}.", path.string());
        return true;
    }

    auto FrameClockRecorder::Record(const FrameClockRow& row) -> void
    {
        if (!m_IsWanted) return;
        if (!m_Store.IsOpen() && (m_IsFailed || !this->Open())) return;

        m_Store.Append(this->FormatRow(row));
    }
}