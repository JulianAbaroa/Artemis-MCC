module M0.Consumer;

import Common.Log.Type;
import Common.State.Type;
import Map.Tag.Type;
import std;

namespace
{
    using Common::Log::Type::LogSink;
    using Map::Tag::Type::Scnr::Structure::ScnrData;
}

namespace M0::Consumer
{
    ScenarioHolder::ScenarioHolder(LogSink& sink) : m_Log(sink, "[ScenarioHolder]") {}

    auto ScenarioHolder::Set(const ScnrData& scenario) -> void
    {
        this->m_Scenario.Publish(scenario);
        this->m_Log.Message("Published scenario with {} bsps.", scenario.StructureBsps.EntryCount);
    }

    auto ScenarioHolder::Get() const -> std::shared_ptr<const ScnrData>
    {
        return this->m_Scenario.Acquire();
    }
}