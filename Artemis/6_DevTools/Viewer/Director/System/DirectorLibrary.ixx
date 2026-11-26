export module DevTools.Viewer.Director.System:Library;

import DevTools.Viewer.Director.Type;
import std;

export namespace DevTools::Viewer::Director::System
{
    // return: The name as the file can hold it: '|' and line breaks become a space and the ends are trimmed. Used for shots and sequences.
    auto SanitizeName(std::string_view name) -> std::string;

    // return: The sequence the bench asks for with the key "director <sequence>": the value sanitized, or nothing when it is empty.
    auto ParseSequenceRequest(std::string_view value) -> std::optional<std::string>;

    // return: The file of the text. A line is a comment when it starts with '#', empty lines are ignored and a line that does not parse is skipped and
    // counted. Numbers use '.', yaw and pitch are in degrees. Ids of sequences that name no shot are dropped and counted, an active sequence that does
    // not exist is cleared. Without the line "version 2" the text is the first version (fixed|name|seconds|..., orbit|..., pov|...): the shots get the ids
    // 1 to N in order, are on, end by seconds and there are no sequences (IsMigrated).
    //   version 2
    //   shot|<id>|fixed|<name>|<on|off>|<end>|<x> <y> <z>|<yaw> <pitch>
    //   shot|<id>|orbit|<name>|<on|off>|<end>|<cx> <cy> <cz>|<radius> <height> <start degrees> <degrees per second>
    //   shot|<id>|pov|<name>|<on|off>|<end>
    //   sequence|<name>|<id> <id> ...
    //   active|<name>
    // The end is "s <seconds>", "r <rounds>" or "hold". A shot without a unique id above zero is invalid.
    auto ParseDirectorFile(std::string_view text) -> Director::Type::ParsedDirector;

    // return: The text of the file in the current format, with the version line first and a comment header. ParseDirectorFile reads it back.
    auto FormatDirectorFile(const Director::Type::DirectorFile& file) -> std::string;

    // return: The id a new shot gets in a file that was just read: one more than the highest one, one for an empty library.
    auto NextShotIdOf(const Director::Type::DirectorFile& file) -> std::uint32_t;

    // return: The library position of the shot with the id.
    auto FindShotIndex(const Director::Type::DirectorFile& file, std::uint32_t id) -> std::optional<std::size_t>;

    // return: The sequence with the name, or null. An empty name finds none.
    auto FindSequence(const Director::Type::DirectorFile& file, std::string_view name) -> const Director::Type::Sequence*;
    auto FindSequence(Director::Type::DirectorFile& file, std::string_view name) -> Director::Type::Sequence*;

    // return: The library positions of the shots to play: the active sequence in its order without the shots that are off, or, with no active sequence,
    // the whole library without the shots that are off. An active sequence whose shots are all off gives an empty playlist.
    auto Playlist(const Director::Type::DirectorFile& file) -> Director::Type::ShotIndexList;

    // Moves the shot at from to the place to (the position it has after the move). return: False, and nothing changes, when a position is out of range or
    // they are the same.
    auto MoveShot(Director::Type::ShotList& shots, std::size_t from, std::size_t to) -> bool;
    auto MoveInSequence(Director::Type::Sequence& sequence, std::size_t from, std::size_t to) -> bool;

    // Inserts the id before the slot at (at the end for a slot past it). return: Always true.
    auto AddToSequence(Director::Type::Sequence& sequence, std::uint32_t id, std::size_t at) -> bool;

    // return: Whether the slot existed. The shot stays in the library.
    auto RemoveFromSequence(Director::Type::Sequence& sequence, std::size_t index) -> bool;

    // Adds the shot to the library and, when a sequence is active, at the end of it. The id must be set by the caller.
    auto AddShot(Director::Type::DirectorFile& file, Director::Type::Shot shot) -> void;

    // Removes the shot from the library and every slot of every sequence that holds it. return: Whether the library had it.
    auto RemoveShot(Director::Type::DirectorFile& file, std::uint32_t id) -> bool;

    // Adds an empty sequence. return: Its name, sanitized and made unique ("Sequence" when empty, "Name 2" when taken). It is not made the active one.
    auto NewSequence(Director::Type::DirectorFile& file, std::string_view name) -> std::string;

    // return: The new name (sanitized, unique among the other sequences), or nothing when the sequence does not exist or the name is empty. The active
    // sequence follows the rename.
    auto RenameSequence(Director::Type::DirectorFile& file, std::string_view name, std::string_view newName) -> std::optional<std::string>;

    // return: Whether it existed. Deleting the active sequence leaves none active. The shots stay in the library.
    auto DeleteSequence(Director::Type::DirectorFile& file, std::string_view name) -> bool;
}