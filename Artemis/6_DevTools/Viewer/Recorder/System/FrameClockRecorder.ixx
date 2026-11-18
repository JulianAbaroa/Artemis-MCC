export module DevTools.Viewer.Recorder.System;

import Common.Log.System;
import Services.Settings.State;
import DevTools.Viewer.Latency.Type;
import DevTools.Viewer.Recorder.State;
import std;

export namespace DevTools::Viewer::Recorder::System
{
    // Writes one CSV line per frame the overlay draws over the game, so the timing can be measured from a file and not from the screen (VW.5d).
    // The file is FrameClock\<date_time>.csv in the Artemis folder of LOCALAPPDATA, a new one each time the recording starts.
    // note: Not thread safe: MapService calls it under its own mutex, on the thread that draws.
    class FrameClockRecorder
    {
    private:
        using Log = Common::Log::System::Log;
        using SettingsStore = Services::Settings::State::SettingsStore;

        using FrameClockRow = DevTools::Viewer::Latency::Type::FrameClockRow;

        using FrameClockStore = DevTools::Viewer::Recorder::State::FrameClockStore;

    public:
        FrameClockRecorder(Log& log, SettingsStore& settingsStore) : m_Log(log), m_SettingsStore(settingsStore) {}
        ~FrameClockRecorder() = default;

        FrameClockRecorder(const FrameClockRecorder&) = delete;
        auto operator=(const FrameClockRecorder&) -> FrameClockRecorder& = delete;

        // Keeps the recording in step with the option: closes the file (writing what is left) when it is not wanted.
        // note: The file is opened by the first Record after the option is on, so none is left with only the header. A file that cannot be
        // opened is reported once and not retried until the option goes off and on again.
        auto Update(bool isWanted) -> void;

        // Adds the row, opening a new file first if needed. Does nothing while the option is off.
        auto Record(const FrameClockRow& row) -> void;

        // return: The first line of the file, the names of the columns.
        static auto Header() -> std::string_view;

        // return: The line of the row, ending in a newline. A value that is not known is an empty field.
        static auto FormatRow(const FrameClockRow& row) -> std::string;

    private:
        Log& m_Log;
        SettingsStore& m_SettingsStore;

        FrameClockStore m_Store{};
        bool m_IsWanted{ false };
        bool m_IsFailed{ false };

        // Opens a new file named with the date, time and milliseconds. return: False if it failed (reported once).
        auto Open() -> bool;
    };
}