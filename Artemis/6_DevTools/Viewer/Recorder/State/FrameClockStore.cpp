module DevTools.Viewer.Recorder.State;

import std;

namespace DevTools::Viewer::Recorder::State
{
    auto FrameClockStore::Open(const std::filesystem::path& path) -> bool
    {
        this->Close();

        std::error_code error{};
        std::filesystem::create_directories(path.parent_path(), error);

        m_File.open(path, std::ios::binary | std::ios::trunc);
        if (!m_File)
        {
            m_File.clear();
            return false;
        }

        m_Buffer.reserve(k_FlushBytes + k_FlushBytes / 4);
        return true;
    }

    auto FrameClockStore::IsOpen() const -> bool
    {
        return m_File.is_open();
    }

    auto FrameClockStore::Append(std::string_view text) -> void
    {
        if (!m_File.is_open()) return;

        m_Buffer.append(text);
        if (m_Buffer.size() < k_FlushBytes) return;

        m_File.write(m_Buffer.data(), static_cast<std::streamsize>(m_Buffer.size()));
        m_Buffer.clear();
    }

    auto FrameClockStore::Close() -> void
    {
        if (m_File.is_open())
        {
            m_File.write(m_Buffer.data(), static_cast<std::streamsize>(m_Buffer.size()));
            m_File.close();
        }

        m_File.clear();
        m_Buffer.clear();
    }
}