export module DevTools.Viewer.Recorder.State;

import std;

export namespace DevTools::Viewer::Recorder::State
{
    // The CSV file of the frame clock and the text waiting to be written to it (VW.5d).
    // note: Not thread safe. MapService uses it only under its own mutex. The text is kept in memory and written in blocks of k_FlushBytes, so
    // the thread that draws does no disk I/O in most frames. Closing writes what is left.
    class FrameClockStore
    {
    public:
        // Text kept before it is written (about 250 bytes a frame: a block every few seconds at 60 fps).
        static constexpr std::size_t k_FlushBytes{ 64 * 1024 };

        FrameClockStore() = default;
        ~FrameClockStore() { this->Close(); }

        FrameClockStore(const FrameClockStore&) = delete;
        auto operator=(const FrameClockStore&) -> FrameClockStore& = delete;

        // Creates the file (and its folder), emptying it if it exists.
        // return: False if the file cannot be opened.
        auto Open(const std::filesystem::path& path) -> bool;

        auto IsOpen() const -> bool;

        // Adds text, and writes the block when it reaches k_FlushBytes. Does nothing while closed.
        auto Append(std::string_view text) -> void;

        // Writes what is left and closes the file.
        auto Close() -> void;

    private:
        std::ofstream m_File{};
        std::string m_Buffer{};
    };
}