export module Engine.Base.Host.Hook:SessionJoinCheck;

import Common.Log.System;
import Engine.Base.Memory.System;
import Engine.Base.Host.State;
import std;

export namespace Engine::Base::Host::Hook
{
    // Patches a network session check so the Artemis users can join the match: the conditional jump after
    // "cmp (session state - 6), 3" becomes unconditional (77 -> EB), skipping that call for session states 6 to 9.
    // note: Same change as AlphaRing. What the skipped call checks is not measured yet (A9_SplitScreen.txt sec. 6).
    // note: Applied only when more than one local user is requested at install time.
    class SessionJoinCheckPatch
    {
    private:
        using Log = Common::Log::System::Log;

        using LocalUserStore = Engine::Base::Host::State::LocalUserStore;

        using AOBService = Engine::Base::Memory::System::AOBService;

    public:
        SessionJoinCheckPatch(Log& log, AOBService& aobService, LocalUserStore& localUserStore) :
            m_Log(log), m_AOBService(aobService), m_LocalUserStore(localUserStore) {}
        ~SessionJoinCheckPatch() = default;

        // Finds the jump by signature and patches it. Does nothing if already patched or if only one local user is requested.
        // return: False if the jump is not found, has unexpected bytes, or cannot be written.
        auto Install() -> bool;

        // Restores the original jump. Does nothing if not patched.
        auto Uninstall() -> void;

    private:
        Log& m_Log;
        AOBService& m_AOBService;
        LocalUserStore& m_LocalUserStore;

        void* m_PatchAddress{ nullptr };
        std::vector<std::uint8_t> m_OriginalBytes{};
        bool m_IsPatched{ false };
    };
}