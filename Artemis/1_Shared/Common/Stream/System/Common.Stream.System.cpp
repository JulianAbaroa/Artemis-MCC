module Common.Stream.System;

import std;

namespace Common::Stream::System
{
    auto WriteRaw(std::ostream& stream, std::span<const std::byte> bytes) -> void
    {
        if (bytes.empty()) return;
        stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }

    auto ReadRaw(std::istream& stream, std::span<std::byte> bytes) -> std::size_t
    {
        if (bytes.empty()) return 0;
        stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        return static_cast<std::size_t>(stream.gcount());
    }
}