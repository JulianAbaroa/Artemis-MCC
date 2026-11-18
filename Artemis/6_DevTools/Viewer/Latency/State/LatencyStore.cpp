module DevTools.Viewer.Latency.State;

import std;

namespace
{
    using DevTools::Viewer::Latency::Type::k_WindowFrames;
}

namespace DevTools::Viewer::Latency::State
{
    auto LatencyStore::Push(const LatencySample& sample, const LatencyMemory& memory) -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        m_Samples.push_back(sample);
        while (m_Samples.size() > k_WindowFrames) m_Samples.pop_front();

        m_Memory = memory;
    }

    auto LatencyStore::GetMemory() const -> LatencyMemory
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        return m_Memory;
    }

    auto LatencyStore::GetSamples() const -> std::vector<LatencySample>
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        return std::vector<LatencySample>(m_Samples.begin(), m_Samples.end());
    }
}