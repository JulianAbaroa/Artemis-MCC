export module M0.Consumer;

import Common.Log.System;
import Common.Log.Type;
import Common.State.Type;
import Map.Tag.Type;
import std;

namespace
{
    using Common::Log::System::Log;
    using Common::Log::Type::LogSink;
    using Common::State::Type::Snapshot;
    using Map::Tag::Type::Scnr::Structure::ScnrData;
}

export namespace M0::Consumer
{
    // Keeps the latest scenario header published and logs every publication.
    class ScenarioHolder
    {
    public:
        // param sink: Must outlive the holder.
        explicit ScenarioHolder(LogSink& sink);

        // Publishes a scenario header.
        auto Set(const ScnrData& scenario) -> void;

        // return: The last published header, or null if none.
        auto Get() const -> std::shared_ptr<const ScnrData>;

    private:
        Log m_Log;
        Snapshot<ScnrData> m_Scenario{};
    };
}