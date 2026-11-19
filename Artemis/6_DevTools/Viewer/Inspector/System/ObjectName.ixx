export module DevTools.Viewer.Inspector.System:Name;

import Bridge.Codec.Tick.Type;
import std;

export namespace DevTools::Viewer::Inspector::System
{
    // Names an object of the tick for the inspector.
    class ObjectName
    {
    private:
        using Tick = Bridge::Codec::Tick::Type::Tick;

    public:
        ObjectName() = default;
        ~ObjectName() = default;

        // return: The tag of the object, else the function of its entity, else its handle in hex.
        static auto Of(const Tick& tick, std::uint32_t handle) -> std::string;

        // return: "none" for the no-handle value, else the name and the handle in hex.
        static auto Label(const Tick& tick, std::uint32_t handle) -> std::string;
    };
}