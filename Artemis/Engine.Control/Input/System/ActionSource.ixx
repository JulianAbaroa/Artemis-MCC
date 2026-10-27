export module Platform.Input.System:ActionSource;

import std;

export namespace Platform::Input::System
{
    // Actions of the players an outside client controls (D215). Implemented by the owner (Mod).
    class ActionSource
    {
    public:
        virtual ~ActionSource() = default;

        // Sets the overrides of the players of the client before the engine applies the actions of a tick.
        // note: Game thread (PlayerAction hook), once per tick. May block until the decision arrives (Lockstep, D139/D143):
        // at most k_LockstepTimeout per wait, without a cap during a pause of a live client, at once when the client leaves.
        virtual auto Prepare(std::uint64_t actionTick) -> void = 0;
    };
}