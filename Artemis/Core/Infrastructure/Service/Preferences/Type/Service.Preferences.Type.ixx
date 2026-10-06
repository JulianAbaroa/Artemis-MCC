export module Service.Preferences.Type;

import std;

export namespace Service::Preferences::Type
{
    using SectionSaver = std::function<void(std::ostream&)>;
    using SectionLoader = std::function<void(std::string_view, std::string_view)>;

    // Key group registered by another module.
    struct Section
    {
        std::string Prefix{};
        SectionSaver Saver{};
        SectionLoader Loader{};
    };
}