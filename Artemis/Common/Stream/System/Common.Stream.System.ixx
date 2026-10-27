export module Common.Stream.System;

import std;

export namespace Common::Stream::System
{
    // Writes the bytes to the stream.
    auto WriteRaw(std::ostream& stream, std::span<const std::byte> bytes) -> void;

    // Reads up to bytes.size() bytes from the stream.
    // return: The bytes read, fewer than asked when the stream ends.
    auto ReadRaw(std::istream& stream, std::span<std::byte> bytes) -> std::size_t;

    // Writes the bytes of a value, as they are in memory.
    // return: Bytes written.
    template <typename T>
        requires std::is_trivially_copyable_v<T>
    auto WriteValue(std::ostream& stream, const T& value) -> std::uint64_t
    {
        Common::Stream::System::WriteRaw(stream, std::as_bytes(std::span<const T, 1>{ &value, 1 }));
        return sizeof(T);
    }

    // Writes the bytes of the rows, as they are in memory.
    // return: Bytes written.
    template <typename T>
        requires std::is_trivially_copyable_v<T>
    auto WriteRows(std::ostream& stream, std::span<const T> rows) -> std::uint64_t
    {
        Common::Stream::System::WriteRaw(stream, std::as_bytes(rows));
        return rows.size_bytes();
    }

    // Reads the bytes of a value.
    // return: Whether all of them were read.
    template <typename T>
        requires std::is_trivially_copyable_v<T>
    auto ReadValue(std::istream& stream, T& value) -> bool
    {
        return Common::Stream::System::ReadRaw(stream, std::as_writable_bytes(std::span<T, 1>{ &value, 1 })) == sizeof(T);
    }
}