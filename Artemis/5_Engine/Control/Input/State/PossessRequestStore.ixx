export module Engine.Control.Input.State:PossessRequest;

import std;

export namespace Engine::Control::Input::State
{
    // The bot the Viewer asks the game to let the user possess, or none (VW.9c.2 B). One thread writes it (the render thread of the Viewer) and the game
    // thread reads it.
    // note: One atomic word holds the request: the active bit, the clock of the write in milliseconds (31 bits, compared modulo) and the handle, so a
    // read always finds a complete request and no lock or allocation is needed on either side. It is not the PovRequestStore of Engine.Base: this
    // project does not import Pov, and what is asked here is another thing.
    class PossessRequestStore
    {
    public:
        PossessRequestStore() = default;
        ~PossessRequestStore() = default;

        PossessRequestStore(const PossessRequestStore&) = delete;
        auto operator=(const PossessRequestStore&) -> PossessRequestStore& = delete;

        // Replaces the request, marks it active and stamps it with the clock.
        // note: The handle is not validated here, the game thread checks it against its bots.
        auto Publish(std::uint32_t playerHandle, std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now()) -> void;

        // Marks the request inactive.
        auto Clear() -> void;

        // param maxAge: A request published longer ago than this is taken as none.
        // return: The handle asked for, or nothing when the request is inactive or older than maxAge.
        auto Read(std::chrono::milliseconds maxAge,
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now()) const -> std::optional<std::uint32_t>;

    private:
        // note: Bit 63 active, bits 32 to 62 the clock in milliseconds, bits 0 to 31 the handle.
        std::atomic<std::uint64_t> m_Word{ 0 };
    };
}