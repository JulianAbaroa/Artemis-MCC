export module DevTools.Workbench.Oracle.System;

import Common.Log.System;
import Services.Settings.State;
import Engine.Capture.Oracle.Type;
import Engine.Capture.Oracle.State;
import Blam.MapModel.World.System;
import Views.Oracle.Type;
import Views.Raycast.Type;
import Bridge.Codec.Pipeline.Type;
import DevTools.Workbench.Oracle.Type;
import DevTools.Workbench.Oracle.State;
import std;

export namespace DevTools::Workbench::Oracle::System
{
    // The raycast oracle (R3, D272): checks in game that the rays of Artemis give what the engine gives, and measures what a ray costs the
    // engine. Every few ticks it hands the game thread a batch of rays (the rays of the last tick and random ones from the eye of the
    // player), reads what the engine answered, compares it with Artemis and writes a summary to the log and a report file.
    // note: Development tool (D72), off by default, outside the pipeline. OnTick runs on the AI thread as a tick observer and never waits
    // for the game thread: the engine function is called by Engine.Capture on the game thread. Draw runs on the render thread.
    class OracleUIService
    {
    private:
        using Log = Common::Log::System::Log;
        using SettingsStore = Services::Settings::State::SettingsStore;
        using OracleBatch = Engine::Capture::Oracle::Type::OracleBatch;
        using OracleProbe = Engine::Capture::Oracle::Type::OracleProbe;
        using OracleStore = Engine::Capture::Oracle::State::OracleStore;
        using Raycaster = Blam::MapModel::World::System::Raycaster;
        using OracleSample = Views::Oracle::Type::Sample::OracleSample;
        using RaySource = Views::Oracle::Type::Sample::RaySource;
        using RaycastHit = Views::Raycast::Type::RaycastHit;
        using ObservedTick = Bridge::Codec::Pipeline::Type::ObservedTick;
        using PendingBatch = DevTools::Workbench::Oracle::Type::PendingBatch;
        using PendingRay = DevTools::Workbench::Oracle::Type::PendingRay;
        using Gate = DevTools::Workbench::Oracle::Type::Gate;
        using OracleUIStore = DevTools::Workbench::Oracle::State::OracleUIStore;

    public:
        OracleUIService(Log& log, SettingsStore& settingsStore, OracleStore& oracleStore, Raycaster& raycaster,
            OracleUIStore& oracleUIStore) :
            m_Log(log), m_SettingsStore(settingsStore), m_OracleStore(oracleStore), m_Raycaster(raycaster),
            m_OracleUIStore(oracleUIStore) {}
        ~OracleUIService() = default;

        OracleUIService(const OracleUIService&) = delete;
        auto operator=(const OracleUIService&) -> OracleUIService& = delete;

        // Draws the settings, the state and the last summary.
        auto Draw() -> void;

        // Reads the answers of the engine, compares them, hands over the next batch and writes the summary when it is due.
        // note: AI thread, after the tick. Does nothing and costs nothing while the oracle is off.
        auto OnTick(const ObservedTick& observed) -> void;

    private:
        Log& m_Log;
        SettingsStore& m_SettingsStore;
        OracleStore& m_OracleStore;
        Raycaster& m_Raycaster;
        OracleUIStore& m_OracleUIStore;

        // Turns the engine side on with new stats, and the file of the report.
        auto Start() -> void;

        // Turns the engine side off and writes the last report.
        auto Stop() -> void;

        // Compares the answers the engine published since the last tick.
        auto Collect() -> void;

        // Hands the engine the next batch: the rays of the tick, then random ones up to the size of the batch.
        auto Submit(const ObservedTick& observed) -> void;

        // Adds a ray to the batch that goes to the engine and to the batch that waits for the answer.
        auto AddRay(OracleBatch& batch, PendingBatch& pending, RaySource source, const PendingRay& ray, std::uint32_t ignoreHandle) -> void;

        // Logs a message the first time a gate of the oracle is reached after a start, so a run that traces nothing says where it stops.
        auto LogOnce(Gate gate, std::string_view text) -> void;

        // Writes the summary to the log and the report file when the interval has passed, or at once when asked.
        auto Summarize(bool isForced) -> void;

        // Writes the report file again. Does nothing without a storage folder.
        auto WriteReport() -> void;
    };
}