export module Engine.Base.Memory.Type:Store;

import std;

export namespace Engine::Base::Memory::Type::Store
{
    // Times a reader of a double buffered store (CameraOverrideStore, PovRequestStore) retries when the writer laps it, before it gives up.
    inline constexpr int k_SlotReadAttempts{ 4 };
}