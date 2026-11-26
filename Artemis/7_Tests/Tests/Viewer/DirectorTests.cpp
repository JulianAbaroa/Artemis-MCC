#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Log.Type;
import Common.Log.System;
import Common.Math.Type;
import DevTools.Viewer.Roster.Type;
import DevTools.Viewer.Director.Type;
import DevTools.Viewer.Director.State;
import DevTools.Viewer.Director.System;
import Tests.Support.Log;
import std;

// The director of the Viewer (VW.14, VW.16): the pure library (file format v2, migration from v1, playlist, advance, edits), the store and the playback of
// the service with a fake clock, built from their own sources.

namespace
{
    using std::chrono::steady_clock;

    using Common::Engine::Type::k_NoHandle;
    using Common::Log::System::Log;
    using Common::Log::Type::LogLevel;
    using Common::Math::Type::Vec3;
    using DevTools::Viewer::Director::State::DirectorStore;
    using DevTools::Viewer::Director::System::AddRoundChange;
    using DevTools::Viewer::Director::System::AddShot;
    using DevTools::Viewer::Director::System::AddToSequence;
    using DevTools::Viewer::Director::System::Advance;
    using DevTools::Viewer::Director::System::DecidePovRequest;
    using DevTools::Viewer::Director::System::DeleteSequence;
    using DevTools::Viewer::Director::System::DescribeShotProgress;
    using DevTools::Viewer::Director::System::DirectorService;
    using DevTools::Viewer::Director::System::FindSequence;
    using DevTools::Viewer::Director::System::FormatDirectorFile;
    using DevTools::Viewer::Director::System::IsRoundChange;
    using DevTools::Viewer::Director::System::IsShotValid;
    using DevTools::Viewer::Director::System::MoveInSequence;
    using DevTools::Viewer::Director::System::MoveShot;
    using DevTools::Viewer::Director::System::NewSequence;
    using DevTools::Viewer::Director::System::NextPlayableShot;
    using DevTools::Viewer::Director::System::NextShotIdOf;
    using DevTools::Viewer::Director::System::OrbitFromPose;
    using DevTools::Viewer::Director::System::OrbitPose;
    using DevTools::Viewer::Director::System::ParseDirectorFile;
    using DevTools::Viewer::Director::System::ParseSequenceRequest;
    using DevTools::Viewer::Director::System::PickPovAgent;
    using DevTools::Viewer::Director::System::Playlist;
    using DevTools::Viewer::Director::System::RemoveFromSequence;
    using DevTools::Viewer::Director::System::RemoveShot;
    using DevTools::Viewer::Director::System::RenameSequence;
    using DevTools::Viewer::Director::System::SanitizeMapKey;
    using DevTools::Viewer::Director::System::SanitizeName;
    using DevTools::Viewer::Director::System::YawPitchOf;
    using DevTools::Viewer::Director::Type::AdvanceInput;
    using DevTools::Viewer::Director::Type::DirectorFile;
    using DevTools::Viewer::Director::Type::DirectorFrame;
    using DevTools::Viewer::Director::Type::DirectorInput;
    using DevTools::Viewer::Director::Type::RoundKey;
    using DevTools::Viewer::Director::Type::Sequence;
    using DevTools::Viewer::Director::Type::Shot;
    using DevTools::Viewer::Director::Type::ShotEnd;
    using DevTools::Viewer::Director::Type::ShotEndKind;
    using DevTools::Viewer::Director::Type::ShotIdList;
    using DevTools::Viewer::Director::Type::ShotIndexList;
    using DevTools::Viewer::Director::Type::ShotKind;
    using DevTools::Viewer::Director::Type::ShotList;
    using DevTools::Viewer::Roster::Type::RosterEntry;
    using Tests::Support::Log::CollectingSink;

    constexpr float k_Pi{ std::numbers::pi_v<float> };

    auto Seconds(float value) -> ShotEnd
    {
        return ShotEnd{ .Kind = ShotEndKind::Seconds, .Value = value };
    }

    auto Rounds(float value) -> ShotEnd
    {
        return ShotEnd{ .Kind = ShotEndKind::Rounds, .Value = value };
    }

    auto Hold() -> ShotEnd
    {
        return ShotEnd{ .Kind = ShotEndKind::Hold, .Value = 0.0f };
    }

    auto FixedShot(std::string name, ShotEnd end, Vec3 eye, float yaw = 0.0f, float pitch = 0.0f) -> Shot
    {
        Shot shot{};
        shot.Kind = ShotKind::Fixed;
        shot.Name = std::move(name);
        shot.End = end;
        shot.Eye = eye;
        shot.Yaw = yaw;
        shot.Pitch = pitch;
        return shot;
    }

    auto OrbitShot(Vec3 center, float radius, float height, float startDegrees, float degreesPerSecond, ShotEnd end = Seconds(20.0f)) -> Shot
    {
        Shot shot{};
        shot.Kind = ShotKind::Orbit;
        shot.Name = "orbit";
        shot.End = end;
        shot.Center = center;
        shot.Radius = radius;
        shot.Height = height;
        shot.StartDegrees = startDegrees;
        shot.DegreesPerSecond = degreesPerSecond;
        return shot;
    }

    auto PovShot(std::string name, ShotEnd end = Seconds(20.0f)) -> Shot
    {
        Shot shot{};
        shot.Kind = ShotKind::Pov;
        shot.Name = std::move(name);
        shot.End = end;
        return shot;
    }

    // A file with the shots in the order given, with the ids 1 to N, and no sequences.
    auto FileOf(std::vector<Shot> shots) -> DirectorFile
    {
        DirectorFile file{};
        std::uint32_t id{ 1 };

        for (Shot& shot : shots)
        {
            shot.Id = id++;
            file.Shots.push_back(std::move(shot));
        }

        return file;
    }

    auto SequenceOf(std::string name, std::vector<std::uint32_t> ids) -> Sequence
    {
        return Sequence{ .Name = std::move(name), .ShotIds = ShotIdList(ids.begin(), ids.end()) };
    }

    // Lists of ids and positions. The containers of the Type module take braces only through these, so the tests spell them the same way.
    auto Ids(const std::vector<std::uint32_t>& ids) -> ShotIdList
    {
        return ShotIdList(ids.begin(), ids.end());
    }

    auto Indices(const std::vector<std::size_t>& indices) -> ShotIndexList
    {
        return ShotIndexList(indices.begin(), indices.end());
    }

    auto Bot(std::uint32_t handle, std::size_t number, bool isAlive = true) -> RosterEntry
    {
        return RosterEntry{ .Handle = handle, .Name = std::format("Mini-Place {}", number), .BotNumber = number, .IsAlive = isAlive };
    }

    auto Human(std::uint32_t handle) -> RosterEntry
    {
        return RosterEntry{ .Handle = handle, .Name = "human", .IsAlive = true };
    }

    // The clock of the tests: a fixed start and seconds after it.
    auto At(double seconds) -> steady_clock::time_point
    {
        return steady_clock::time_point{ std::chrono::seconds{ 1000 } } + std::chrono::duration_cast<steady_clock::duration>(std::chrono::duration<double>(seconds));
    }

    auto Input(double seconds, const std::vector<RosterEntry>& roster, std::int16_t round = 0) -> DirectorInput
    {
        return DirectorInput{ .Now = At(seconds), .HasRoster = true, .Roster = roster, .Round = round };
    }

    // The frame of a game that has no Tick yet (the match just loaded).
    auto NoTick(double seconds) -> DirectorInput
    {
        return DirectorInput{ .Now = At(seconds) };
    }

    auto Play(DirectorStore& store) -> void
    {
        store.RequestTogglePlay();
    }

    auto ExpectNear(const Vec3& a, const Vec3& b, float tolerance = 1e-3f) -> void
    {
        EXPECT_NEAR(a.X, b.X, tolerance);
        EXPECT_NEAR(a.Y, b.Y, tolerance);
        EXPECT_NEAR(a.Z, b.Z, tolerance);
    }

    // The store, the log and the service of a playback test.
    struct Rig
    {
        CollectingSink Sink{};
        Log Logger{ Sink };
        DirectorStore Store{};
        DirectorService Service{ Store, Logger };
    };

    // A folder of the tests that is removed when the test ends.
    struct TempFolder
    {
        std::filesystem::path Path{};

        TempFolder()
        {
            Path = std::filesystem::temp_directory_path() / std::format("artemis_director_test_{}", steady_clock::now().time_since_epoch().count());
            std::filesystem::create_directories(Path);
        }

        ~TempFolder()
        {
            std::error_code error{};
            std::filesystem::remove_all(Path, error);
        }
    };

    auto ReadAll(const std::filesystem::path& path) -> std::string
    {
        std::ifstream file{ path, std::ios::binary };
        return std::string{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };
    }
}

// --- The file: format v2, migration, invalid lines ---

TEST(DirectorTests, TheV2FileOfAllThreeKindsGoesAroundAndComesBack)
{
    DirectorFile file{ FileOf({
        FixedShot("Corner", Seconds(12.5f), Vec3{ 1.5f, -2.25f, 3.0f }, 0.5f, -0.25f),
        OrbitShot(Vec3{ 10.0f, 20.0f, 30.0f }, 4.5f, 2.0f, 45.0f, -6.0f, Rounds(2.0f)),
        PovShot("Bot view", Hold()) }) };
    file.Shots[1].IsEnabled = false;
    file.Sequences.push_back(SequenceOf("Cuts", { 3, 1, 3, 2 }));
    file.Sequences.push_back(SequenceOf("Empty", {}));
    file.ActiveSequence = "Cuts";

    const std::string text{ FormatDirectorFile(file) };
    EXPECT_TRUE(text.starts_with("version 2\n"));

    const auto parsed{ ParseDirectorFile(text) };

    EXPECT_EQ(parsed.Skipped, 0u);
    EXPECT_EQ(parsed.DroppedIds, 0u);
    EXPECT_FALSE(parsed.IsMigrated);
    ASSERT_EQ(parsed.File.Shots.size(), 3u);

    const Shot& fixed{ parsed.File.Shots[0] };
    EXPECT_EQ(fixed.Id, 1u);
    EXPECT_EQ(fixed.Kind, ShotKind::Fixed);
    EXPECT_EQ(fixed.Name, "Corner");
    EXPECT_TRUE(fixed.IsEnabled);
    EXPECT_EQ(fixed.End.Kind, ShotEndKind::Seconds);
    EXPECT_FLOAT_EQ(fixed.End.Value, 12.5f);
    ExpectNear(fixed.Eye, Vec3{ 1.5f, -2.25f, 3.0f });
    EXPECT_NEAR(fixed.Yaw, 0.5f, 1e-4f);
    EXPECT_NEAR(fixed.Pitch, -0.25f, 1e-4f);

    const Shot& orbit{ parsed.File.Shots[1] };
    EXPECT_EQ(orbit.Id, 2u);
    EXPECT_EQ(orbit.Kind, ShotKind::Orbit);
    EXPECT_FALSE(orbit.IsEnabled);
    EXPECT_EQ(orbit.End.Kind, ShotEndKind::Rounds);
    EXPECT_FLOAT_EQ(orbit.End.Value, 2.0f);
    ExpectNear(orbit.Center, Vec3{ 10.0f, 20.0f, 30.0f });
    EXPECT_FLOAT_EQ(orbit.Radius, 4.5f);
    EXPECT_FLOAT_EQ(orbit.Height, 2.0f);
    EXPECT_FLOAT_EQ(orbit.StartDegrees, 45.0f);
    EXPECT_FLOAT_EQ(orbit.DegreesPerSecond, -6.0f);

    EXPECT_EQ(parsed.File.Shots[2].Kind, ShotKind::Pov);
    EXPECT_EQ(parsed.File.Shots[2].Name, "Bot view");
    EXPECT_EQ(parsed.File.Shots[2].End.Kind, ShotEndKind::Hold);

    // The sequences keep their order and their repeated ids, and the active one comes back.
    ASSERT_EQ(parsed.File.Sequences.size(), 2u);
    EXPECT_EQ(parsed.File.Sequences[0].Name, "Cuts");
    EXPECT_EQ(parsed.File.Sequences[0].ShotIds, Ids({ 3, 1, 3, 2 }));
    EXPECT_TRUE(parsed.File.Sequences[1].ShotIds.empty());
    EXPECT_EQ(parsed.File.ActiveSequence, "Cuts");

    // Writing what was read gives the same text.
    EXPECT_EQ(FormatDirectorFile(parsed.File), text);
}

TEST(DirectorTests, TheAnglesOfTheFileAreInDegreesAndTheEndsHaveTheirOwnWords)
{
    const auto parsed{ ParseDirectorFile("version 2\nshot|4|fixed|a|on|s 10|0 0 0|90 -45\nshot|5|pov|b|off|R 3\nshot|6|pov|c|on|HOLD\n") };
    EXPECT_EQ(parsed.Skipped, 0u);
    ASSERT_EQ(parsed.File.Shots.size(), 3u);
    EXPECT_NEAR(parsed.File.Shots[0].Yaw, k_Pi / 2.0f, 1e-5f);
    EXPECT_NEAR(parsed.File.Shots[0].Pitch, -k_Pi / 4.0f, 1e-5f);
    EXPECT_EQ(parsed.File.Shots[1].End.Kind, ShotEndKind::Rounds);
    EXPECT_FALSE(parsed.File.Shots[1].IsEnabled);
    EXPECT_EQ(parsed.File.Shots[2].End.Kind, ShotEndKind::Hold);

    // The id the next shot of the session gets is above every one of the file.
    EXPECT_EQ(NextShotIdOf(parsed.File), 7u);
    EXPECT_EQ(NextShotIdOf(DirectorFile{}), 1u);
}

TEST(DirectorTests, InvalidLinesAreSkippedAndCounted)
{
    const std::string text{
        "version 2\r\n"
        "# a comment\r\n"
        "\r\n"
        "   \r\n"
        "shot|1|pov|ok|on|s 10\r\n"
        "shot|2|fixed|few fields|on|s 10|1 2 3\r\n"
        "shot|3|fixed|bad number|on|s 10|1 2 x|0 0\r\n"
        "shot|4|fixed|decimal comma|on|s 10|1,5 2 3|0 0\r\n"
        "shot|5|fixed|zero seconds|on|s 0|1 2 3|0 0\r\n"
        "shot|6|fixed|negative seconds|on|s -5|1 2 3|0 0\r\n"
        "shot|7|fixed|word as seconds|on|s ten|1 2 3|0 0\r\n"
        "shot|8|pov|zero rounds|on|r 0\r\n"
        "shot|9|pov|half a round|on|r 1.5\r\n"
        "shot|10|pov|unknown end|on|x 3\r\n"
        "shot|11|pov|hold with a value|on|hold 4\r\n"
        "shot|12|pov|end without a value|on|s\r\n"
        "shot|13|pov|no end|on\r\n"
        "shot|14|pov|bad switch|maybe|s 10\r\n"
        "shot|0|pov|id zero|on|s 10\r\n"
        "shot|x|pov|id word|on|s 10\r\n"
        "shot|1|pov|id again|on|s 10\r\n"
        "shot|15|orbit|three numbers|on|s 10|0 0 0|4 2 0\r\n"
        "shot|16|orbit|negative radius|on|s 10|0 0 0|-4 2 0 6\r\n"
        "shot|17|dolly|unknown kind|on|s 10\r\n"
        "shot|18|pov|too many|on|s 10|1\r\n"
        "line without a word\r\n"
        "shot|19|orbit|ok|off|r 2|0 0 0|4 2 0 6" };

    const auto parsed{ ParseDirectorFile(text) };

    ASSERT_EQ(parsed.File.Shots.size(), 2u);
    EXPECT_EQ(parsed.Skipped, 21u);
    EXPECT_EQ(parsed.File.Shots[0].Name, "ok");
    EXPECT_EQ(parsed.File.Shots[1].Kind, ShotKind::Orbit);
    EXPECT_EQ(parsed.File.Shots[1].Id, 19u);
}

TEST(DirectorTests, SequenceIdsOfNoShotAreDroppedAndCounted)
{
    const std::string text{
        "version 2\n"
        "shot|1|pov|a|on|s 10\n"
        "shot|2|pov|b|on|s 10\n"
        "sequence|Main|2 9 1 9 2\n"
        "sequence|Main|1\n"
        "sequence||1\n"
        "sequence|Bad ids|1 x\n"
        "sequence|Short\n"
        "sequence|Gone|7 8\n"
        "active|Missing\n" };

    const auto parsed{ ParseDirectorFile(text) };

    // The two ids of 9 and the two of Gone are dropped. The repeated name, the empty name, the bad id and the short line are skipped.
    EXPECT_EQ(parsed.DroppedIds, 4u);
    EXPECT_EQ(parsed.Skipped, 4u);
    ASSERT_EQ(parsed.File.Sequences.size(), 2u);
    EXPECT_EQ(parsed.File.Sequences[0].ShotIds, Ids({ 2, 1, 2 }));
    EXPECT_TRUE(parsed.File.Sequences[1].ShotIds.empty());

    // An active sequence that does not exist is none.
    EXPECT_TRUE(parsed.File.ActiveSequence.empty());
}

TEST(DirectorTests, TheFirstVersionIsMigratedAndWrittenAsTheSecond)
{
    const std::string text{
        "# Artemis director shots of this map. One shot per line.\r\n"
        "#   fixed|<name>|<seconds>|<x> <y> <z>|<yaw> <pitch>\r\n"
        "fixed|Corner|12.5|1 2 3|90 0\r\n"
        "this line is not a shot\r\n"
        "orbit|Round|30|0 0 0|4 2 0 6\r\n"
        "pov|Bot|8\r\n" };

    const auto parsed{ ParseDirectorFile(text) };

    EXPECT_TRUE(parsed.IsMigrated);
    EXPECT_EQ(parsed.Skipped, 1u);
    EXPECT_TRUE(parsed.File.Sequences.empty());
    EXPECT_TRUE(parsed.File.ActiveSequence.empty());
    ASSERT_EQ(parsed.File.Shots.size(), 3u);

    // Ids 1 to N in order, all on, the end is the seconds the shot had.
    for (std::size_t i{ 0 }; i < 3; ++i)
    {
        EXPECT_EQ(parsed.File.Shots[i].Id, i + 1);
        EXPECT_TRUE(parsed.File.Shots[i].IsEnabled);
        EXPECT_EQ(parsed.File.Shots[i].End.Kind, ShotEndKind::Seconds);
    }

    EXPECT_FLOAT_EQ(parsed.File.Shots[0].End.Value, 12.5f);
    EXPECT_FLOAT_EQ(parsed.File.Shots[1].End.Value, 30.0f);
    EXPECT_FLOAT_EQ(parsed.File.Shots[2].End.Value, 8.0f);
    EXPECT_EQ(parsed.File.Shots[2].Name, "Bot");

    // The next save is the second version, and reading it is not a migration.
    const auto again{ ParseDirectorFile(FormatDirectorFile(parsed.File)) };
    EXPECT_FALSE(again.IsMigrated);
    EXPECT_EQ(again.File.Shots.size(), 3u);

    // No file, an empty one and only comments are an empty file that is not migrated.
    EXPECT_FALSE(ParseDirectorFile("").IsMigrated);
    EXPECT_FALSE(ParseDirectorFile("# nothing\n\n").IsMigrated);
}

TEST(DirectorTests, TheNamesAreSanitizedAndTheKindIsReadForgivingly)
{
    const auto parsed{ ParseDirectorFile("version 2\nshot|1| POV |  spaced name  | ON | S 10 \n") };
    ASSERT_EQ(parsed.File.Shots.size(), 1u);
    EXPECT_EQ(parsed.File.Shots[0].Name, "spaced name");

    DirectorFile file{ FileOf({ PovShot("a|b\nc") }) };
    file.Sequences.push_back(SequenceOf("x|y", { 1 }));
    file.ActiveSequence = "x|y";
    const auto back{ ParseDirectorFile(FormatDirectorFile(file)) };
    EXPECT_EQ(back.File.Shots[0].Name, "a b c");
    EXPECT_EQ(SanitizeName("  a|b\r\n"), "a b");
}

TEST(DirectorTests, AShotWithAnEndThatCannotWorkIsNotValid)
{
    Shot shot{ PovShot("a") };
    EXPECT_TRUE(IsShotValid(shot));

    shot.End = Seconds(0.0f);
    EXPECT_FALSE(IsShotValid(shot));
    shot.End = Seconds(std::numeric_limits<float>::infinity());
    EXPECT_FALSE(IsShotValid(shot));
    shot.End = Rounds(0.0f);
    EXPECT_FALSE(IsShotValid(shot));
    shot.End = Rounds(1.5f);
    EXPECT_FALSE(IsShotValid(shot));
    shot.End = Rounds(2.0f);
    EXPECT_TRUE(IsShotValid(shot));
    shot.End = Hold();
    EXPECT_TRUE(IsShotValid(shot));
}

// --- The playlist ---

TEST(DirectorTests, ThePlaylistIsTheActiveSequenceWithoutTheShotsThatAreOff)
{
    DirectorFile file{ FileOf({ PovShot("a"), PovShot("b"), PovShot("c"), PovShot("d") }) };
    file.Shots[1].IsEnabled = false;
    file.Sequences.push_back(SequenceOf("Main", { 3, 2, 1, 3, 4 }));

    // No active sequence: the whole library without the shots that are off, in library order.
    EXPECT_EQ(Playlist(file), Indices({ 0, 2, 3 }));

    // An active sequence: its order, its repeated ids, without the shot that is off.
    file.ActiveSequence = "Main";
    EXPECT_EQ(Playlist(file), Indices({ 2, 0, 2, 3 }));

    // A sequence whose shots are all off is empty, it does not fall back to the library.
    file.Sequences.push_back(SequenceOf("Off", { 2 }));
    file.ActiveSequence = "Off";
    EXPECT_TRUE(Playlist(file).empty());

    // An active name that names no sequence is none.
    file.ActiveSequence = "Missing";
    EXPECT_EQ(Playlist(file), Indices({ 0, 2, 3 }));

    // Everything off: empty.
    for (Shot& shot : file.Shots) shot.IsEnabled = false;
    file.ActiveSequence.clear();
    EXPECT_TRUE(Playlist(file).empty());
}

TEST(DirectorTests, ThePlayableShotSkipsPovWithoutBotsAndNeverLoops)
{
    ShotList shots{};
    shots.push_back(PovShot("a"));
    shots.push_back(FixedShot("b", Seconds(5.0f), Vec3{}));
    shots.push_back(PovShot("c"));
    const ShotIndexList playlist{ Indices({ 0, 1, 2 }) };

    EXPECT_EQ(NextPlayableShot(shots, playlist, 0, 1, true), std::optional<std::size_t>{ 0 });
    EXPECT_EQ(NextPlayableShot(shots, playlist, 0, 1, false), std::optional<std::size_t>{ 1 });
    EXPECT_EQ(NextPlayableShot(shots, playlist, 2, 1, false), std::optional<std::size_t>{ 1 });
    EXPECT_EQ(NextPlayableShot(shots, playlist, 2, -1, false), std::optional<std::size_t>{ 1 });
    EXPECT_EQ(NextPlayableShot(shots, playlist, 7, 1, true), std::optional<std::size_t>{ 0 });

    // The answer is a playlist position, not a library one.
    const ShotIndexList reordered{ Indices({ 2, 1, 0 }) };
    EXPECT_EQ(NextPlayableShot(shots, reordered, 0, 1, false), std::optional<std::size_t>{ 1 });

    ShotList onlyPov{};
    onlyPov.push_back(PovShot("a"));
    onlyPov.push_back(PovShot("b"));
    EXPECT_EQ(NextPlayableShot(onlyPov, Indices({ 0, 1 }), 0, 1, false), std::nullopt);
    EXPECT_EQ(NextPlayableShot(shots, ShotIndexList{}, 0, 1, true), std::nullopt);

    // A shot that is not valid is not played either, and a position out of the library is skipped.
    ShotList broken{};
    broken.push_back(FixedShot("zero", Seconds(0.0f), Vec3{}));
    EXPECT_EQ(NextPlayableShot(broken, Indices({ 0, 5 }), 0, 1, true), std::nullopt);
}

// --- Advance: the end of each shot ---

TEST(DirectorTests, ASecondsShotLeavesWhenItsSecondsPassAndIgnoresRounds)
{
    AdvanceInput input{ .Count = 3, .Index = 0, .ElapsedSeconds = 4.9, .End = Seconds(5.0f) };
    EXPECT_EQ(Advance(input), std::nullopt);

    input.ElapsedSeconds = 5.0;
    EXPECT_EQ(Advance(input), std::optional<std::size_t>{ 1 });

    // After the last shot comes the first.
    input.Index = 2;
    EXPECT_EQ(Advance(input), std::optional<std::size_t>{ 0 });

    // An index out of the playlist goes to the first, an empty playlist does nothing.
    input.Index = 9;
    EXPECT_EQ(Advance(input), std::optional<std::size_t>{ 0 });
    input.Count = 0;
    EXPECT_EQ(Advance(input), std::nullopt);

    // Rounds do not cut a shot that ends by seconds.
    const AdvanceInput rounds{ .Count = 2, .Index = 0, .ElapsedSeconds = 6.0, .End = Seconds(20.0f), .RoundChanges = 5 };
    EXPECT_EQ(Advance(rounds), std::nullopt);

    // Seconds that are not above zero count as the default.
    const AdvanceInput broken{ .Count = 2, .Index = 0, .ElapsedSeconds = 5.0, .End = Seconds(0.0f) };
    EXPECT_EQ(Advance(broken), std::nullopt);
}

TEST(DirectorTests, ARoundsShotLeavesAtItsNthRoundChangeAndIgnoresSeconds)
{
    AdvanceInput input{ .Count = 3, .Index = 1, .ElapsedSeconds = 500.0, .End = Rounds(1.0f), .RoundChanges = 0 };

    // Seconds do not cut it.
    EXPECT_EQ(Advance(input), std::nullopt);

    input.RoundChanges = 1;
    EXPECT_EQ(Advance(input), std::optional<std::size_t>{ 2 });

    input.End = Rounds(2.0f);
    EXPECT_EQ(Advance(input), std::nullopt);
    input.RoundChanges = 2;
    EXPECT_EQ(Advance(input), std::optional<std::size_t>{ 2 });
    input.RoundChanges = 3;
    EXPECT_EQ(Advance(input), std::optional<std::size_t>{ 2 });

    // A value that is not usable counts as one round.
    input.End = Rounds(0.0f);
    input.RoundChanges = 1;
    EXPECT_EQ(Advance(input), std::optional<std::size_t>{ 2 });
}

TEST(DirectorTests, ARoundChangeInTheFirstThreeSecondsDoesNotCount)
{
    EXPECT_EQ(AddRoundChange(0, false, 10.0), 0u);
    EXPECT_EQ(AddRoundChange(0, true, 10.0), 1u);
    EXPECT_EQ(AddRoundChange(1, true, 2.9), 1u);
    EXPECT_EQ(AddRoundChange(1, true, 3.0), 2u);
    EXPECT_EQ(AddRoundChange(4, false, 3.0), 4u);
}

TEST(DirectorTests, AHoldShotNeverLeavesByItself)
{
    const AdvanceInput input{ .Count = 3, .Index = 0, .ElapsedSeconds = 1.0e9, .End = Hold(), .RoundChanges = 1000 };
    EXPECT_EQ(Advance(input), std::nullopt);
}

TEST(DirectorTests, TheProgressTextSaysSecondsRoundsOrHold)
{
    EXPECT_EQ(DescribeShotProgress(Seconds(20.0f), 12.34, 0), "12.3/20 s");
    EXPECT_EQ(DescribeShotProgress(Seconds(7.5f), 1.0, 0), "1.0/7.5 s");
    EXPECT_EQ(DescribeShotProgress(Rounds(2.0f), 40.0, 1), "40.0 s, round 1/2");
    EXPECT_EQ(DescribeShotProgress(Hold(), 3.0, 0), "3.0 s, hold");
}

// --- Edits (pure) ---

TEST(DirectorTests, TheLibraryAndTheSequencesAreReorderedByMoving)
{
    ShotList shots{};
    for (const char* name : { "a", "b", "c", "d" })
    {
        shots.push_back(PovShot(name));
    }

    const auto names{ [&shots]
    {
        std::string text{};
        for (const Shot& shot : shots) text += shot.Name;
        return text;
    } };

    EXPECT_TRUE(MoveShot(shots, 0, 2));
    EXPECT_EQ(names(), "bcad");
    EXPECT_TRUE(MoveShot(shots, 3, 0));
    EXPECT_EQ(names(), "dbca");
    EXPECT_TRUE(MoveShot(shots, 1, 3));
    EXPECT_EQ(names(), "dcab");

    // The borders: the same place, and a place out of the list, change nothing.
    EXPECT_FALSE(MoveShot(shots, 2, 2));
    EXPECT_FALSE(MoveShot(shots, 4, 0));
    EXPECT_FALSE(MoveShot(shots, 0, 4));
    EXPECT_EQ(names(), "dcab");

    Sequence sequence{ SequenceOf("s", { 5, 6, 7 }) };
    EXPECT_TRUE(MoveInSequence(sequence, 2, 0));
    EXPECT_EQ(sequence.ShotIds, Ids({ 7, 5, 6 }));
    EXPECT_TRUE(MoveInSequence(sequence, 0, 2));
    EXPECT_EQ(sequence.ShotIds, Ids({ 5, 6, 7 }));
    EXPECT_FALSE(MoveInSequence(sequence, 1, 1));
    EXPECT_FALSE(MoveInSequence(sequence, 3, 0));
    EXPECT_FALSE(MoveInSequence(sequence, 0, 3));
    EXPECT_EQ(sequence.ShotIds, Ids({ 5, 6, 7 }));
}

TEST(DirectorTests, AShotIsAddedInTheMiddleOfASequenceAndTakenOutWithoutLeavingTheLibrary)
{
    Sequence sequence{ SequenceOf("s", { 1, 2 }) };

    EXPECT_TRUE(AddToSequence(sequence, 9, 1));
    EXPECT_EQ(sequence.ShotIds, Ids({ 1, 9, 2 }));
    EXPECT_TRUE(AddToSequence(sequence, 9, 0));
    EXPECT_EQ(sequence.ShotIds, Ids({ 9, 1, 9, 2 }));

    // A slot past the end appends.
    EXPECT_TRUE(AddToSequence(sequence, 4, 99));
    EXPECT_EQ(sequence.ShotIds, Ids({ 9, 1, 9, 2, 4 }));

    EXPECT_TRUE(RemoveFromSequence(sequence, 0));
    EXPECT_EQ(sequence.ShotIds, Ids({ 1, 9, 2, 4 }));
    EXPECT_FALSE(RemoveFromSequence(sequence, 4));
    EXPECT_EQ(sequence.ShotIds.size(), 4u);
}

TEST(DirectorTests, TakingAShotOutOfASequenceRemovesThePositionThatWasClickedAndKeepsTheShotInTheLibrary)
{
    Rig rig{};

    DirectorFile file{ FileOf({ PovShot("a"), PovShot("b"), PovShot("c") }) };
    file.Sequences.push_back(SequenceOf("Main", { 2, 1, 2, 3 }));
    rig.Store.SetFile(file);

    // The edit the "x" of the panel makes: the third position (the second copy of the shot 2), found by the name of the sequence.
    const std::uint64_t before{ rig.Store.GetRevision() };
    rig.Store.Edit([](DirectorFile& edited) {
        Sequence* sequence{ FindSequence(edited, "Main") };
        return sequence && RemoveFromSequence(*sequence, 2);
    });

    const DirectorFile after{ rig.Store.CopyFile() };
    EXPECT_NE(rig.Store.GetRevision(), before);
    EXPECT_EQ(after.Sequences[0].ShotIds, Ids({ 2, 1, 3 }));
    EXPECT_EQ(after.Shots.size(), 3u);

    // A position that does not exist is not an edit: nothing is saved.
    const std::uint64_t revision{ rig.Store.GetRevision() };
    rig.Store.Edit([](DirectorFile& edited) { return RemoveFromSequence(*FindSequence(edited, "Main"), 3); });
    EXPECT_EQ(rig.Store.GetRevision(), revision);
}

TEST(DirectorTests, DeletingAShotTakesItOutOfEverySequence)
{
    DirectorFile file{ FileOf({ PovShot("a"), PovShot("b"), PovShot("c") }) };
    file.Sequences.push_back(SequenceOf("one", { 2, 1, 2, 3 }));
    file.Sequences.push_back(SequenceOf("two", { 2 }));

    EXPECT_TRUE(RemoveShot(file, 2));
    ASSERT_EQ(file.Shots.size(), 2u);
    EXPECT_EQ(file.Shots[0].Name, "a");
    EXPECT_EQ(file.Sequences[0].ShotIds, Ids({ 1, 3 }));
    EXPECT_TRUE(file.Sequences[1].ShotIds.empty());

    EXPECT_FALSE(RemoveShot(file, 2));

    // A new shot with the active sequence goes to the library and to the end of that sequence.
    file.ActiveSequence = "one";
    Shot added{ PovShot("d") };
    added.Id = 10;
    AddShot(file, added);
    EXPECT_EQ(file.Shots.back().Id, 10u);
    EXPECT_EQ(file.Sequences[0].ShotIds, Ids({ 1, 3, 10 }));
    EXPECT_TRUE(file.Sequences[1].ShotIds.empty());

    // Without an active sequence only the library gets it.
    file.ActiveSequence.clear();
    added.Id = 11;
    AddShot(file, added);
    EXPECT_EQ(file.Shots.size(), 4u);
    EXPECT_EQ(file.Sequences[0].ShotIds.size(), 3u);
}

TEST(DirectorTests, SequenceNamesAreSanitizedAndUnique)
{
    DirectorFile file{};

    EXPECT_EQ(NewSequence(file, "Cuts"), "Cuts");
    EXPECT_EQ(NewSequence(file, "Cuts"), "Cuts 2");
    EXPECT_EQ(NewSequence(file, "  Cuts  "), "Cuts 3");
    EXPECT_EQ(NewSequence(file, "a|b"), "a b");
    EXPECT_EQ(NewSequence(file, ""), "Sequence");
    EXPECT_EQ(NewSequence(file, "|"), "Sequence 2");
    EXPECT_EQ(file.Sequences.size(), 6u);

    // Renaming: the active sequence follows, a taken name gets a number, an empty one is refused, the same name is itself.
    file.ActiveSequence = "Cuts 2";
    EXPECT_EQ(RenameSequence(file, "Cuts 2", "Final"), std::optional<std::string>{ "Final" });
    EXPECT_EQ(file.ActiveSequence, "Final");
    EXPECT_EQ(RenameSequence(file, "Final", "Cuts"), std::optional<std::string>{ "Cuts 2" });
    EXPECT_EQ(file.ActiveSequence, "Cuts 2");
    EXPECT_EQ(RenameSequence(file, "Cuts 2", "Cuts 2"), std::optional<std::string>{ "Cuts 2" });
    EXPECT_EQ(RenameSequence(file, "Cuts 2", "   "), std::nullopt);
    EXPECT_EQ(RenameSequence(file, "Nothing", "x"), std::nullopt);
    EXPECT_EQ(file.ActiveSequence, "Cuts 2");

    // Deleting the active sequence leaves none active, and the shots are not touched.
    file.Shots.push_back(Shot{ .Id = 1 });
    EXPECT_TRUE(DeleteSequence(file, "Cuts 2"));
    EXPECT_TRUE(file.ActiveSequence.empty());
    EXPECT_FALSE(DeleteSequence(file, "Cuts 2"));
    EXPECT_EQ(file.Shots.size(), 1u);
    EXPECT_EQ(FindSequence(file, "Cuts 2"), nullptr);
    EXPECT_EQ(FindSequence(file, ""), nullptr);
    EXPECT_NE(FindSequence(file, "Cuts"), nullptr);
}

// --- The key of the bench ---

TEST(DirectorTests, TheDirectorKeyOfTheBenchIsTheNameOfASequence)
{
    EXPECT_EQ(ParseSequenceRequest("Cuts A"), std::optional<std::string>{ "Cuts A" });
    EXPECT_EQ(ParseSequenceRequest("  Cuts A  "), std::optional<std::string>{ "Cuts A" });

    // The name is sanitized as the names of the file are, so it finds the sequence the file holds.
    EXPECT_EQ(ParseSequenceRequest("a|b"), std::optional<std::string>{ "a b" });

    // Without a value it is an error.
    EXPECT_EQ(ParseSequenceRequest(""), std::nullopt);
    EXPECT_EQ(ParseSequenceRequest("   "), std::nullopt);
    EXPECT_EQ(ParseSequenceRequest("|"), std::nullopt);
}

// --- Orbit, POV agent and the rest of the pure shots ---

TEST(DirectorTests, AnOrbitStartsWhereItsPoseWasMade)
{
    const Vec3 eye{ 3.0f, 4.0f, 10.0f };
    const Vec3 hit{ 0.0f, 0.0f, 0.0f };
    const Vec3 forward{ -3.0f, -4.0f, -10.0f };

    const Shot orbit{ OrbitFromPose(eye, forward, hit) };

    EXPECT_EQ(orbit.Kind, ShotKind::Orbit);
    ExpectNear(orbit.Center, hit);
    EXPECT_NEAR(orbit.Radius, 5.0f, 1e-4f);
    EXPECT_NEAR(orbit.Height, 10.0f, 1e-4f);
    EXPECT_NEAR(orbit.StartDegrees, std::atan2(4.0f, 3.0f) * 180.0f / k_Pi, 1e-3f);

    const auto pose{ OrbitPose(orbit, 0.0) };
    ExpectNear(pose.Eye, eye);

    // It looks at the center.
    const float length{ std::sqrt(9.0f + 16.0f + 100.0f) };
    ExpectNear(pose.Forward, Vec3{ -3.0f / length, -4.0f / length, -10.0f / length });
}

TEST(DirectorTests, AnOrbitTurnsByRealTimeAndTheSignOfTheSpeed)
{
    Shot orbit{ OrbitShot(Vec3{ 0.0f, 0.0f, 0.0f }, 5.0f, 2.0f, 0.0f, 90.0f) };

    ExpectNear(OrbitPose(orbit, 0.0).Eye, Vec3{ 5.0f, 0.0f, 2.0f });
    ExpectNear(OrbitPose(orbit, 1.0).Eye, Vec3{ 0.0f, 5.0f, 2.0f });
    ExpectNear(OrbitPose(orbit, 2.0).Eye, Vec3{ -5.0f, 0.0f, 2.0f });

    orbit.DegreesPerSecond = -90.0f;
    ExpectNear(OrbitPose(orbit, 1.0).Eye, Vec3{ 0.0f, -5.0f, 2.0f });

    // A long run keeps the angle small, and a time that is not a number is zero.
    orbit.DegreesPerSecond = 6.0f;
    ExpectNear(OrbitPose(orbit, 3600.0).Eye, Vec3{ 5.0f, 0.0f, 2.0f });
    ExpectNear(OrbitPose(orbit, std::numeric_limits<double>::quiet_NaN()).Eye, Vec3{ 5.0f, 0.0f, 2.0f });

    // Always looking at the center.
    const auto pose{ OrbitPose(orbit, 7.0) };
    const Vec3 toCenter{ -pose.Eye.X, -pose.Eye.Y, -pose.Eye.Z };
    const float length{ std::sqrt(toCenter.X * toCenter.X + toCenter.Y * toCenter.Y + toCenter.Z * toCenter.Z) };
    ExpectNear(pose.Forward, Vec3{ toCenter.X / length, toCenter.Y / length, toCenter.Z / length });
}

TEST(DirectorTests, WithoutAHitTheCenterIsOnThePlaneBelowTheEye)
{
    const float inverseRoot{ 1.0f / std::sqrt(2.0f) };

    // Looking down at 45 degrees: the plane 5 below the eye is reached 5 ahead.
    const Shot down{ OrbitFromPose(Vec3{ 0.0f, 0.0f, 10.0f }, Vec3{ inverseRoot, 0.0f, -inverseRoot }, std::nullopt) };
    ExpectNear(down.Center, Vec3{ 5.0f, 0.0f, 5.0f });
    EXPECT_NEAR(down.Radius, 5.0f, 1e-3f);
    EXPECT_NEAR(down.Height, 5.0f, 1e-3f);
    EXPECT_NEAR(down.StartDegrees, 180.0f, 1e-3f);

    // Not looking down: 10 ahead.
    const Shot level{ OrbitFromPose(Vec3{ 0.0f, 0.0f, 10.0f }, Vec3{ 1.0f, 0.0f, 0.0f }, std::nullopt) };
    ExpectNear(level.Center, Vec3{ 10.0f, 0.0f, 10.0f });
    EXPECT_NEAR(level.Height, 0.0f, 1e-3f);

    const Shot up{ OrbitFromPose(Vec3{ 0.0f, 0.0f, 10.0f }, Vec3{ 0.0f, 1.0f, 0.5f }, std::nullopt) };
    EXPECT_NEAR(up.Radius, 10.0f * std::cos(std::atan2(0.5f, 1.0f)), 1e-2f);

    // Almost level and down: the plane is very far, so the center stops at the maximum distance.
    const Shot far{ OrbitFromPose(Vec3{ 0.0f, 0.0f, 10.0f }, Vec3{ 1.0f, 0.0f, -0.001f }, std::nullopt) };
    EXPECT_NEAR(far.Radius, 100.0f, 0.1f);

    // A hit that is not a number is no hit.
    const float nan{ std::numeric_limits<float>::quiet_NaN() };
    const Shot bad{ OrbitFromPose(Vec3{ 0.0f, 0.0f, 10.0f }, Vec3{ 1.0f, 0.0f, 0.0f }, Vec3{ nan, 0.0f, 0.0f }) };
    ExpectNear(bad.Center, Vec3{ 10.0f, 0.0f, 10.0f });
}

TEST(DirectorTests, YawAndPitchAreThoseOfTheFreeCamera)
{
    const auto east{ YawPitchOf(Vec3{ 1.0f, 0.0f, 0.0f }) };
    EXPECT_NEAR(east.Yaw, 0.0f, 1e-6f);
    EXPECT_NEAR(east.Pitch, 0.0f, 1e-6f);

    const auto north{ YawPitchOf(Vec3{ 0.0f, 2.0f, 0.0f }) };
    EXPECT_NEAR(north.Yaw, k_Pi / 2.0f, 1e-5f);

    const auto down{ YawPitchOf(Vec3{ 1.0f, 0.0f, -1.0f }) };
    EXPECT_NEAR(down.Pitch, -k_Pi / 4.0f, 1e-5f);

    const auto none{ YawPitchOf(Vec3{ 0.0f, 0.0f, 0.0f }) };
    EXPECT_EQ(none.Yaw, 0.0f);
    EXPECT_EQ(none.Pitch, 0.0f);
}

TEST(DirectorTests, ARoundChangesWhenTheMatchOrTheRoundDoes)
{
    EXPECT_FALSE(IsRoundChange(std::nullopt, RoundKey{ 1, 0 }));
    EXPECT_FALSE(IsRoundChange(RoundKey{ 1, 0 }, RoundKey{ 1, 0 }));
    EXPECT_TRUE(IsRoundChange(RoundKey{ 1, 0 }, RoundKey{ 1, 1 }));
    EXPECT_TRUE(IsRoundChange(RoundKey{ 1, 3 }, RoundKey{ 2, 3 }));
}

TEST(DirectorTests, ThePovAgentCyclesBetweenTheLiveBotsAndSkipsHumans)
{
    const std::vector<RosterEntry> roster{ Human(1), Bot(10, 0), Human(2), Bot(11, 1), Bot(12, 2, false), Bot(13, 3) };

    // From nothing it starts with the first bot, then goes on in the order of the roster, skipping the humans and the dead bot, and wraps.
    EXPECT_EQ(PickPovAgent(roster, k_NoHandle), std::optional<std::uint32_t>{ 10 });
    EXPECT_EQ(PickPovAgent(roster, 10), std::optional<std::uint32_t>{ 11 });
    EXPECT_EQ(PickPovAgent(roster, 11), std::optional<std::uint32_t>{ 13 });
    EXPECT_EQ(PickPovAgent(roster, 13), std::optional<std::uint32_t>{ 10 });

    // A last agent that left the roster starts again at the beginning.
    EXPECT_EQ(PickPovAgent(roster, 99), std::optional<std::uint32_t>{ 10 });

    // The only live bot is chosen again.
    const std::vector<RosterEntry> alone{ Human(1), Bot(10, 0) };
    EXPECT_EQ(PickPovAgent(alone, 10), std::optional<std::uint32_t>{ 10 });
}

TEST(DirectorTests, WithoutLiveBotsThereIsNoPovAgent)
{
    EXPECT_EQ(PickPovAgent({}, k_NoHandle), std::nullopt);

    const std::vector<RosterEntry> humans{ Human(1), Human(2) };
    EXPECT_EQ(PickPovAgent(humans, k_NoHandle), std::nullopt);

    const std::vector<RosterEntry> dead{ Human(1), Bot(10, 0, false) };
    EXPECT_EQ(PickPovAgent(dead, k_NoHandle), std::nullopt);
}

TEST(DirectorTests, TheMapKeyIsAFileName)
{
    EXPECT_EQ(SanitizeMapKey("Map M.1"), "Map_M.1");
    EXPECT_EQ(SanitizeMapKey("a/b\\c:d*?"), "a_b_c_d__");
    EXPECT_EQ(SanitizeMapKey(""), "default");
    EXPECT_EQ(SanitizeMapKey("..\\x"), "___x");
    EXPECT_EQ(SanitizeMapKey(std::string(200, 'a')).size(), 64u);
}

TEST(DirectorTests, WhilePlayingThePovRequestIsTheDirectorsAlone)
{
    // A POV shot publishes its player, whatever the option and the selection say.
    EXPECT_EQ(DecidePovRequest(true, 7, false, k_NoHandle), std::optional<std::uint32_t>{ 7 });
    EXPECT_EQ(DecidePovRequest(true, 7, true, 9), std::optional<std::uint32_t>{ 7 });

    // Any other shot, or a wait, withdraws it even with Watch POV on and a player selected.
    EXPECT_EQ(DecidePovRequest(true, k_NoHandle, true, 9), std::nullopt);
    EXPECT_EQ(DecidePovRequest(true, k_NoHandle, false, 9), std::nullopt);

    // Not playing: Watch POV with the observed player, as before.
    EXPECT_EQ(DecidePovRequest(false, k_NoHandle, true, 9), std::optional<std::uint32_t>{ 9 });
    EXPECT_EQ(DecidePovRequest(false, k_NoHandle, true, k_NoHandle), std::nullopt);
    EXPECT_EQ(DecidePovRequest(false, k_NoHandle, false, 9), std::nullopt);
}

// --- The store ---

TEST(DirectorTests, TheEditsOfTheFileAreCountedAndTheIdsAreNeverReused)
{
    DirectorStore store{};

    // A load changes the serial and not the revision, so it is not saved back.
    store.SetFile(FileOf({ PovShot("a"), PovShot("b") }));
    EXPECT_EQ(store.GetRevision(), 0u);
    EXPECT_EQ(store.GetLoadSerial(), 1u);

    // The ids of a new shot are above the ones the file has.
    EXPECT_EQ(store.AllocateShotId(), 3u);
    EXPECT_EQ(store.AllocateShotId(), 4u);

    // An edit that did nothing does not change the revision, one that did changes it once.
    EXPECT_FALSE(store.Edit([](DirectorFile&) { return false; }));
    EXPECT_EQ(store.GetRevision(), 0u);

    EXPECT_TRUE(store.Edit([](DirectorFile& file) { return RemoveShot(file, 2); }));
    EXPECT_EQ(store.GetRevision(), 1u);
    EXPECT_EQ(store.CopyFile().Shots.size(), 1u);

    // The id of a deleted shot is not given again in the session, not even the highest one.
    EXPECT_TRUE(store.Edit([](DirectorFile& file) { return RemoveShot(file, 1); }));
    EXPECT_EQ(store.AllocateShotId(), 5u);

    // A file that is loaded later with higher ids raises the next one.
    store.SetFile(FileOf({ PovShot("a"), PovShot("b"), PovShot("c"), PovShot("d"), PovShot("e"), PovShot("f"), PovShot("g") }));
    EXPECT_EQ(store.AllocateShotId(), 8u);
}

TEST(DirectorTests, TheRequestsAreTakenOnceAndTheBenchOnesAreKept)
{
    DirectorStore store{};

    // Play from the start plays, and its sequence is taken once. An empty one means the active sequence of the file.
    EXPECT_FALSE(store.IsPlaying());
    store.RequestPlayFromStart("Cuts");
    EXPECT_TRUE(store.IsPlaying());
    EXPECT_EQ(store.ConsumePlayFromStart(), std::optional<std::string>{ "Cuts" });
    EXPECT_EQ(store.ConsumePlayFromStart(), std::nullopt);

    store.RequestPlayFromStart("");
    EXPECT_EQ(store.ConsumePlayFromStart(), std::optional<std::string>{ "" });

    // Dropping the requests of the panel and the keys does not drop the one of the bench.
    store.RequestPlayFromStart("x");
    store.RequestStep(2);
    store.SetLoadRequest("Bench sequence", 0);
    store.ClearRequests();
    EXPECT_EQ(store.ConsumePlayFromStart(), std::nullopt);
    EXPECT_EQ(store.ConsumeStep(), 0);
    EXPECT_EQ(store.TakeLoadRequest(1), std::optional<std::string>{ "Bench sequence" });
    EXPECT_EQ(store.TakeLoadRequest(2), std::nullopt);

    // A request of the next match replaces the one that was not taken, and ClearRequests does not clear it, ClearLoadRequest does.
    store.SetLoadRequest("one", 0);
    store.SetLoadRequest("two", 0);
    EXPECT_EQ(store.TakeLoadRequest(1), std::optional<std::string>{ "two" });
    store.SetLoadRequest("three", 0);
    store.ClearLoadRequest();
    EXPECT_EQ(store.TakeLoadRequest(1), std::nullopt);
}

TEST(DirectorTests, ARequestOfTheBenchBelongsToTheFirstEngineStartAboveTheOneItWasMadeAt)
{
    DirectorStore store{};

    // Made when the engine had counted 3 starts: the start 3 (the one that was already there) and the ones before never take it.
    store.SetLoadRequest("Bench", 3);
    EXPECT_EQ(store.TakeLoadRequest(0), std::nullopt);
    EXPECT_EQ(store.TakeLoadRequest(2), std::nullopt);
    EXPECT_EQ(store.TakeLoadRequest(3), std::nullopt);

    // A refused take keeps it for the start that follows.
    EXPECT_EQ(store.TakeLoadRequest(4), std::optional<std::string>{ "Bench" });
    EXPECT_EQ(store.TakeLoadRequest(5), std::nullopt);
}

// --- The playback ---

TEST(DirectorTests, ThePlaybackGoesFromShotToShotByRealSeconds)
{
    Rig rig{};
    const std::vector<RosterEntry> roster{ Human(1) };

    rig.Store.SetFile(FileOf({ FixedShot("a", Seconds(5.0f), Vec3{ 1.0f, 0.0f, 0.0f }), FixedShot("b", Seconds(5.0f), Vec3{ 2.0f, 0.0f, 0.0f }) }));

    // Stopped: an empty frame.
    EXPECT_FALSE(rig.Service.Update(Input(0.0, roster)).IsPlaying);

    Play(rig.Store);
    DirectorFrame frame{ rig.Service.Update(Input(0.0, roster)) };
    EXPECT_TRUE(frame.IsPlaying);
    EXPECT_TRUE(frame.HasShot);
    EXPECT_EQ(frame.Kind, ShotKind::Fixed);
    EXPECT_FLOAT_EQ(frame.Eye.X, 1.0f);
    EXPECT_EQ(frame.PovPlayer, k_NoHandle);
    EXPECT_EQ(rig.Store.GetStatus().Index, 0u);
    EXPECT_EQ(rig.Store.GetStatus().ShotName, "a");
    EXPECT_EQ(rig.Store.GetStatus().End.Kind, ShotEndKind::Seconds);

    EXPECT_FLOAT_EQ(rig.Service.Update(Input(4.9, roster)).Eye.X, 1.0f);
    EXPECT_FLOAT_EQ(rig.Service.Update(Input(5.1, roster)).Eye.X, 2.0f);
    EXPECT_EQ(rig.Store.GetStatus().Index, 1u);
    EXPECT_NEAR(rig.Store.GetStatus().ElapsedSeconds, 0.0f, 1e-3f);

    // Back to the first one.
    EXPECT_FLOAT_EQ(rig.Service.Update(Input(10.2, roster)).Eye.X, 1.0f);

    // The toggle stops, and then it has no pose and no POV player.
    Play(rig.Store);
    frame = rig.Service.Update(Input(11.0, roster));
    EXPECT_FALSE(frame.IsPlaying);
    EXPECT_FALSE(rig.Store.IsPlaying());
    EXPECT_EQ(rig.Store.GetPovPlayer(), k_NoHandle);
}

TEST(DirectorTests, AnOrbitIsPlayedByRealTime)
{
    Rig rig{};
    const std::vector<RosterEntry> roster{ Human(1) };

    rig.Store.SetFile(FileOf({ OrbitShot(Vec3{ 0.0f, 0.0f, 0.0f }, 5.0f, 2.0f, 0.0f, 90.0f) }));
    Play(rig.Store);

    DirectorFrame frame{ rig.Service.Update(Input(0.0, roster)) };
    ExpectNear(frame.Eye, Vec3{ 5.0f, 0.0f, 2.0f });

    frame = rig.Service.Update(Input(1.0, roster));
    ExpectNear(frame.Eye, Vec3{ 0.0f, 5.0f, 2.0f });
    EXPECT_NEAR(frame.Yaw, std::atan2(-5.0f, 0.0f), 1e-4f);
}

TEST(DirectorTests, ASecondsShotIgnoresTheRoundsAndARoundsShotCountsThem)
{
    Rig rig{};
    const std::vector<RosterEntry> roster{ Human(1) };

    rig.Store.SetFile(FileOf({ FixedShot("seconds", Seconds(20.0f), Vec3{ 1.0f, 0.0f, 0.0f }), FixedShot("one round", Rounds(1.0f), Vec3{ 2.0f, 0.0f, 0.0f }),
        FixedShot("two rounds", Rounds(2.0f), Vec3{ 3.0f, 0.0f, 0.0f }) }));
    Play(rig.Store);

    rig.Service.Update(Input(0.0, roster, 0));

    // Round after round: the shot of 20 seconds is not cut by any.
    rig.Service.Update(Input(5.0, roster, 1));
    rig.Service.Update(Input(6.0, roster, 2));
    rig.Service.Update(Input(7.0, roster, 3));
    EXPECT_EQ(rig.Store.GetStatus().Index, 0u);

    // Its seconds cut it.
    rig.Service.Update(Input(20.5, roster, 3));
    EXPECT_EQ(rig.Store.GetStatus().Index, 1u);

    // A round change in the first 3 seconds of the next shot does not count: no cut.
    rig.Service.Update(Input(22.0, roster, 4));
    EXPECT_EQ(rig.Store.GetStatus().Index, 1u);
    EXPECT_EQ(rig.Store.GetStatus().RoundChanges, 0u);

    // The next one does: the shot of one round ends, and a long time does not cut the next one.
    rig.Service.Update(Input(25.0, roster, 5));
    EXPECT_EQ(rig.Store.GetStatus().Index, 2u);
    rig.Service.Update(Input(900.0, roster, 5));
    EXPECT_EQ(rig.Store.GetStatus().Index, 2u);

    // It takes two round changes, then it goes back to the first shot.
    rig.Service.Update(Input(901.0, roster, 6));
    EXPECT_EQ(rig.Store.GetStatus().Index, 2u);
    EXPECT_EQ(rig.Store.GetStatus().RoundChanges, 1u);
    rig.Service.Update(Input(905.0, roster, 7));
    EXPECT_EQ(rig.Store.GetStatus().Index, 0u);

    // A new match is a new round change too.
    rig.Service.Update(Input(906.0, roster, 7));
    rig.Service.Update(Input(926.0, roster, 7));
    EXPECT_EQ(rig.Store.GetStatus().Index, 1u);
    rig.Store.SetMatchId(5);
    rig.Service.Update(Input(930.0, roster, 7));
    EXPECT_EQ(rig.Store.GetStatus().Index, 2u);
}

TEST(DirectorTests, AHoldShotStaysUntilTheStepKeys)
{
    Rig rig{};
    const std::vector<RosterEntry> roster{ Human(1) };

    rig.Store.SetFile(FileOf({ FixedShot("hold", Hold(), Vec3{ 1.0f, 0.0f, 0.0f }), FixedShot("next", Seconds(5.0f), Vec3{ 2.0f, 0.0f, 0.0f }) }));
    Play(rig.Store);

    rig.Service.Update(Input(0.0, roster, 0));
    rig.Service.Update(Input(100.0, roster, 1));
    rig.Service.Update(Input(100000.0, roster, 2));
    EXPECT_EQ(rig.Store.GetStatus().Index, 0u);
    EXPECT_EQ(rig.Store.GetStatus().End.Kind, ShotEndKind::Hold);

    rig.Store.RequestStep(1);
    EXPECT_FLOAT_EQ(rig.Service.Update(Input(100001.0, roster, 2)).Eye.X, 2.0f);
    EXPECT_EQ(rig.Store.GetStatus().Index, 1u);

    // The other shot ends by its seconds and the hold comes again.
    EXPECT_FLOAT_EQ(rig.Service.Update(Input(100006.5, roster, 2)).Eye.X, 1.0f);
}

TEST(DirectorTests, ThePlaylistIsTheActiveSequenceAndTheShotsThatAreOffAreSkipped)
{
    Rig rig{};
    const std::vector<RosterEntry> roster{ Human(1) };

    DirectorFile file{ FileOf({ FixedShot("a", Seconds(5.0f), Vec3{ 1.0f, 0.0f, 0.0f }), FixedShot("b", Seconds(5.0f), Vec3{ 2.0f, 0.0f, 0.0f }),
        FixedShot("c", Seconds(5.0f), Vec3{ 3.0f, 0.0f, 0.0f }) }) };
    file.Shots[1].IsEnabled = false;
    file.Sequences.push_back(SequenceOf("Main", { 3, 2, 1, 3 }));
    file.ActiveSequence = "Main";
    rig.Store.SetFile(file);
    Play(rig.Store);

    // c, a, c (b is off), then back to c.
    EXPECT_FLOAT_EQ(rig.Service.Update(Input(0.0, roster)).Eye.X, 3.0f);
    EXPECT_EQ(rig.Store.GetStatus().Count, 3u);
    EXPECT_EQ(rig.Store.GetStatus().SequenceName, "Main");
    EXPECT_FLOAT_EQ(rig.Service.Update(Input(5.1, roster)).Eye.X, 1.0f);
    EXPECT_FLOAT_EQ(rig.Service.Update(Input(10.2, roster)).Eye.X, 3.0f);
    EXPECT_EQ(rig.Store.GetStatus().Index, 2u);
    EXPECT_FLOAT_EQ(rig.Service.Update(Input(15.3, roster)).Eye.X, 3.0f);
    EXPECT_EQ(rig.Store.GetStatus().Index, 0u);

    // Without an active sequence the library plays, without the shot that is off.
    rig.Store.Edit([](DirectorFile& edited) {
        edited.ActiveSequence.clear();
        return true;
    });

    rig.Service.Update(Input(15.4, roster));
    EXPECT_EQ(rig.Store.GetStatus().Count, 2u);
    EXPECT_TRUE(rig.Store.GetStatus().SequenceName.empty());
}

TEST(DirectorTests, AnEditOfThePlayingShotTakesEffectAndOneThatTakesItOutMovesOn)
{
    Rig rig{};
    const std::vector<RosterEntry> roster{ Human(1) };

    rig.Store.SetFile(FileOf({ FixedShot("a", Seconds(5.0f), Vec3{ 1.0f, 0.0f, 0.0f }), FixedShot("b", Seconds(20.0f), Vec3{ 2.0f, 0.0f, 0.0f }),
        FixedShot("c", Seconds(5.0f), Vec3{ 3.0f, 0.0f, 0.0f }) }));
    Play(rig.Store);

    rig.Service.Update(Input(0.0, roster));
    rig.Service.Update(Input(5.1, roster));
    EXPECT_EQ(rig.Store.GetStatus().ShotName, "b");

    // The seconds of the playing shot are edited: it ends at the new ones.
    rig.Store.Edit([](DirectorFile& edited) {
        edited.Shots[1].End = Seconds(6.0f);
        return true;
    });
    EXPECT_FLOAT_EQ(rig.Service.Update(Input(10.0, roster)).Eye.X, 2.0f);
    EXPECT_FLOAT_EQ(rig.Service.Update(Input(11.2, roster)).Eye.X, 3.0f);

    // The shot that plays moves in the library (the playlist is rebuilt): it goes on, it does not start again.
    rig.Store.Edit([](DirectorFile& edited) { return MoveShot(edited.Shots, 2, 0); });
    EXPECT_FLOAT_EQ(rig.Service.Update(Input(12.0, roster)).Eye.X, 3.0f);
    EXPECT_EQ(rig.Store.GetStatus().Index, 0u);
    EXPECT_NEAR(rig.Store.GetStatus().ElapsedSeconds, 0.8f, 1e-3f);

    // The shot that plays is switched off: the one that takes its place goes on.
    rig.Store.Edit([](DirectorFile& edited) {
        edited.Shots[0].IsEnabled = false;
        return true;
    });
    EXPECT_FLOAT_EQ(rig.Service.Update(Input(13.0, roster)).Eye.X, 1.0f);

    // A shot deleted from the library under the playing one too.
    rig.Store.Edit([](DirectorFile& edited) { return RemoveShot(edited, 1); });
    EXPECT_NE(rig.Service.Update(Input(14.0, roster)).Eye.X, 1.0f);
}

TEST(DirectorTests, APovShotShowsTheNextBotEachTimeAndIsSkippedWithoutBots)
{
    Rig rig{};

    rig.Store.SetFile(FileOf({ PovShot("a", Seconds(5.0f)), FixedShot("b", Seconds(5.0f), Vec3{ 2.0f, 0.0f, 0.0f }) }));
    Play(rig.Store);

    // No bots: the POV shot is skipped and the fixed one plays.
    const std::vector<RosterEntry> humans{ Human(1) };
    DirectorFrame frame{ rig.Service.Update(Input(0.0, humans)) };
    EXPECT_EQ(frame.Kind, ShotKind::Fixed);
    EXPECT_EQ(rig.Store.GetPovPlayer(), k_NoHandle);

    // With two bots the POV shot comes again after the fixed one and each time it shows the next bot.
    const std::vector<RosterEntry> bots{ Human(1), Bot(10, 0), Bot(11, 1) };
    frame = rig.Service.Update(Input(5.1, bots));
    EXPECT_EQ(frame.Kind, ShotKind::Pov);
    EXPECT_EQ(frame.PovPlayer, 10u);
    EXPECT_EQ(rig.Store.GetPovPlayer(), 10u);

    rig.Service.Update(Input(10.2, bots));
    frame = rig.Service.Update(Input(15.4, bots));
    EXPECT_EQ(frame.Kind, ShotKind::Pov);
    EXPECT_EQ(frame.PovPlayer, 11u);

    // The agent leaves the table: its shot ends at once.
    const std::vector<RosterEntry> left{ Human(1), Bot(10, 0) };
    frame = rig.Service.Update(Input(16.0, left));
    EXPECT_EQ(frame.Kind, ShotKind::Fixed);
    EXPECT_EQ(rig.Store.GetPovPlayer(), k_NoHandle);

    // A bot that is still there is shown again after the fixed shot. Without a Tick nothing is known, so nothing ends.
    frame = rig.Service.Update(Input(21.5, left));
    EXPECT_EQ(frame.Kind, ShotKind::Pov);
    EXPECT_EQ(frame.PovPlayer, 10u);

    frame = rig.Service.Update(NoTick(22.0));
    EXPECT_EQ(frame.Kind, ShotKind::Pov);
    EXPECT_EQ(frame.PovPlayer, 10u);
}

TEST(DirectorTests, ALoneListOfPovShotsWithoutBotsWaitsAndRetriesOncePerSecond)
{
    Rig rig{};

    rig.Store.SetFile(FileOf({ PovShot("a") }));
    Play(rig.Store);

    const std::vector<RosterEntry> humans{ Human(1) };
    DirectorFrame frame{ rig.Service.Update(Input(0.0, humans)) };

    // It plays and waits: no shot to show, the camera stays where it is.
    EXPECT_TRUE(frame.IsPlaying);
    EXPECT_FALSE(frame.HasShot);
    EXPECT_TRUE(rig.Store.GetStatus().IsHolding);

    frame = rig.Service.Update(Input(0.5, humans));
    EXPECT_FALSE(frame.HasShot);

    // A bot appears: the next try (after a second) finds it.
    const std::vector<RosterEntry> bots{ Human(1), Bot(10, 0) };
    frame = rig.Service.Update(Input(0.7, bots));
    EXPECT_FALSE(frame.HasShot);
    frame = rig.Service.Update(Input(1.1, bots));
    EXPECT_TRUE(frame.HasShot);
    EXPECT_EQ(frame.PovPlayer, 10u);
    EXPECT_FALSE(rig.Store.GetStatus().IsHolding);
}

TEST(DirectorTests, AnEmptyPlaylistDoesNotPlayFromThePanel)
{
    Rig rig{};
    const std::vector<RosterEntry> roster{ Human(1) };

    Play(rig.Store);
    EXPECT_FALSE(rig.Service.Update(Input(0.0, roster)).IsPlaying);
    EXPECT_FALSE(rig.Store.IsPlaying());

    // Every shot off is an empty playlist too.
    DirectorFile file{ FileOf({ PovShot("a") }) };
    file.Shots[0].IsEnabled = false;
    rig.Store.SetFile(file);
    Play(rig.Store);
    EXPECT_FALSE(rig.Service.Update(Input(1.0, roster)).IsPlaying);
}

TEST(DirectorTests, TheStepsGoToTheNextAndPreviousShotAndAreDroppedWhenStopped)
{
    Rig rig{};
    const std::vector<RosterEntry> roster{ Human(1) };

    std::vector<Shot> shots{};
    for (int i{ 0 }; i < 3; ++i) shots.push_back(FixedShot(std::format("s{}", i), Seconds(20.0f), Vec3{ static_cast<float>(i), 0.0f, 0.0f }));
    rig.Store.SetFile(FileOf(shots));

    // A step asked while stopped is not kept for later.
    rig.Store.RequestStep(1);
    rig.Service.Update(Input(0.0, roster));
    Play(rig.Store);
    rig.Service.Update(Input(1.0, roster));
    EXPECT_EQ(rig.Store.GetStatus().Index, 0u);

    rig.Store.RequestStep(1);
    rig.Service.Update(Input(2.0, roster));
    EXPECT_EQ(rig.Store.GetStatus().Index, 1u);

    rig.Store.RequestStep(-1);
    rig.Store.RequestStep(-1);
    rig.Service.Update(Input(3.0, roster));
    EXPECT_EQ(rig.Store.GetStatus().Index, 2u);

    // The shot started again at the step.
    EXPECT_NEAR(rig.Store.GetStatus().ElapsedSeconds, 0.0f, 1e-3f);
}

TEST(DirectorTests, AnotherMapStartsItsFileFromTheFirstShotAndStopStopsEverything)
{
    Rig rig{};
    const std::vector<RosterEntry> roster{ Human(1) };

    const DirectorFile file{ FileOf({ FixedShot("a", Seconds(20.0f), Vec3{ 1.0f, 0.0f, 0.0f }), FixedShot("b", Seconds(20.0f), Vec3{ 2.0f, 0.0f, 0.0f }) }) };
    rig.Store.SetFile(file);
    Play(rig.Store);

    rig.Service.Update(Input(0.0, roster));
    rig.Store.RequestStep(1);
    rig.Service.Update(Input(1.0, roster));
    EXPECT_EQ(rig.Store.GetStatus().Index, 1u);

    rig.Store.SetFile(file);
    EXPECT_FLOAT_EQ(rig.Service.Update(Input(2.0, roster)).Eye.X, 1.0f);

    rig.Store.RequestStep(1);
    rig.Store.RequestAddFixed();
    rig.Service.Stop();
    EXPECT_FALSE(rig.Store.IsPlaying());
    EXPECT_FALSE(rig.Store.ConsumeAddFixed());
    EXPECT_EQ(rig.Store.ConsumeStep(), 0);
    EXPECT_FALSE(rig.Service.Update(Input(5.0, roster)).IsPlaying);
}

// --- Playing from the load of a match: auto-play and the sequence of the bench ---

TEST(DirectorTests, ANewMatchPlaysFromTheLoadOnlyWithAutoPlayOrARequest)
{
    Rig rig{};

    // No match is no start, whatever the option says.
    EXPECT_FALSE(rig.Service.Observe(0, true));

    // The first key seen is a start. Without the option and without a request it does not play.
    EXPECT_FALSE(rig.Service.Observe(1, false));
    EXPECT_FALSE(rig.Store.IsPlaying());

    // The same key again is the same match.
    EXPECT_FALSE(rig.Service.Observe(1, true));

    // A new key with the option: it plays and asks the caller to make the Viewer active.
    EXPECT_TRUE(rig.Service.Observe(2, true));
    EXPECT_TRUE(rig.Store.IsPlaying());
    EXPECT_FALSE(rig.Service.Observe(2, true));

    // The first key seen with the option is a start too (the match was there when the DLL came).
    Rig first{};
    EXPECT_TRUE(first.Service.Observe(7, true));
}

TEST(DirectorTests, ThePlayFromTheLoadStartsInTheFirstFrameEvenWithoutATick)
{
    Rig rig{};
    rig.Store.SetFile(FileOf({ FixedShot("first", Seconds(5.0f), Vec3{ 1.0f, 0.0f, 0.0f }), FixedShot("second", Seconds(5.0f), Vec3{ 2.0f, 0.0f, 0.0f }) }));

    // The match loads: the first frame has no Tick yet, and the camera is already the director's.
    ASSERT_TRUE(rig.Service.Observe(1, true));
    const DirectorFrame frame{ rig.Service.Update(NoTick(0.0)) };
    EXPECT_TRUE(frame.IsPlaying);
    EXPECT_TRUE(frame.HasShot);
    EXPECT_FLOAT_EQ(frame.Eye.X, 1.0f);
    EXPECT_EQ(rig.Store.GetStatus().Index, 0u);

    // Rounds and the roster come with the Tick afterwards: the shot goes on and it is not restarted.
    const std::vector<RosterEntry> roster{ Human(1) };
    EXPECT_FLOAT_EQ(rig.Service.Update(Input(2.0, roster)).Eye.X, 1.0f);
    EXPECT_NEAR(rig.Store.GetStatus().ElapsedSeconds, 2.0f, 1e-3f);
    EXPECT_FLOAT_EQ(rig.Service.Update(Input(5.1, roster)).Eye.X, 2.0f);
}

TEST(DirectorTests, ApovShotThatComesFirstWaitsForTheTickAndThenStartsAtOnce)
{
    Rig rig{};
    rig.Store.SetFile(FileOf({ PovShot("pov", Seconds(5.0f)), FixedShot("fixed", Seconds(5.0f), Vec3{ 2.0f, 0.0f, 0.0f }) }));

    ASSERT_TRUE(rig.Service.Observe(1, true));

    // Without a Tick the roster is unknown: the first shot of the playlist is not skipped, the camera waits for it.
    EXPECT_FALSE(rig.Service.Update(NoTick(0.0)).HasShot);
    EXPECT_FALSE(rig.Service.Update(NoTick(0.1)).HasShot);
    EXPECT_TRUE(rig.Store.GetStatus().IsHolding);

    // The first frame with the roster starts it, with no wait of a second.
    const std::vector<RosterEntry> bots{ Human(1), Bot(10, 0) };
    const DirectorFrame frame{ rig.Service.Update(Input(0.2, bots)) };
    EXPECT_TRUE(frame.HasShot);
    EXPECT_EQ(frame.Kind, ShotKind::Pov);
    EXPECT_EQ(frame.PovPlayer, 10u);
}

TEST(DirectorTests, TheLoadWaitsForShotsAndAMatchStartRestartsAPlayingDirector)
{
    Rig rig{};
    const std::vector<RosterEntry> roster{ Human(1) };

    // Nothing to play yet (the file of the map is not loaded): a start for a new match waits instead of giving up.
    ASSERT_TRUE(rig.Service.Observe(1, true));
    EXPECT_TRUE(rig.Service.Update(NoTick(0.0)).IsPlaying);
    EXPECT_FALSE(rig.Service.Update(NoTick(0.1)).HasShot);
    EXPECT_TRUE(rig.Store.IsPlaying());

    // The file arrives (another map was loaded): it starts from its first shot.
    rig.Store.SetFile(FileOf({ FixedShot("a", Seconds(20.0f), Vec3{ 1.0f, 0.0f, 0.0f }), FixedShot("b", Seconds(20.0f), Vec3{ 2.0f, 0.0f, 0.0f }) }));
    EXPECT_FLOAT_EQ(rig.Service.Update(Input(0.2, roster)).Eye.X, 1.0f);

    // Playing in the second shot, a new match starts from the first.
    rig.Store.RequestStep(1);
    rig.Service.Update(Input(1.0, roster));
    EXPECT_EQ(rig.Store.GetStatus().Index, 1u);

    ASSERT_TRUE(rig.Service.Observe(2, true));
    EXPECT_FLOAT_EQ(rig.Service.Update(NoTick(2.0)).Eye.X, 1.0f);
    EXPECT_EQ(rig.Store.GetStatus().Index, 0u);
}

TEST(DirectorTests, TheSequenceOfTheBenchPlaysInPlaceOfTheActiveOneWithoutChangingTheFile)
{
    Rig rig{};
    const std::vector<RosterEntry> roster{ Human(1) };

    DirectorFile file{ FileOf({ FixedShot("a", Seconds(5.0f), Vec3{ 1.0f, 0.0f, 0.0f }), FixedShot("b", Seconds(5.0f), Vec3{ 2.0f, 0.0f, 0.0f }) }) };
    file.Sequences.push_back(SequenceOf("Main", { 1 }));
    file.Sequences.push_back(SequenceOf("Bench", { 2, 1 }));
    file.ActiveSequence = "Main";
    rig.Store.SetFile(file);

    // The bench asks with the next match, and the director takes it at the first start it sees, option off or on.
    rig.Store.SetLoadRequest("Bench", 0);
    ASSERT_TRUE(rig.Service.Observe(1, false));
    EXPECT_FALSE(rig.Store.TakeLoadRequest(2).has_value());

    EXPECT_FLOAT_EQ(rig.Service.Update(NoTick(0.0)).Eye.X, 2.0f);
    EXPECT_EQ(rig.Store.GetStatus().SequenceName, "Bench");
    EXPECT_EQ(rig.Store.GetStatus().Count, 2u);
    EXPECT_FLOAT_EQ(rig.Service.Update(Input(5.1, roster)).Eye.X, 1.0f);

    // The file of the user is the same, and it is not saved because of this.
    EXPECT_EQ(rig.Store.CopyFile().ActiveSequence, "Main");
    EXPECT_EQ(rig.Store.GetRevision(), 0u);
    EXPECT_EQ(rig.Sink.CountOf(LogLevel::Warning), 0u);

    // Stopping ends the request: the next play uses the active sequence.
    Play(rig.Store);
    rig.Service.Update(Input(6.0, roster));
    Play(rig.Store);
    rig.Service.Update(Input(7.0, roster));
    EXPECT_EQ(rig.Store.GetStatus().SequenceName, "Main");
}

TEST(DirectorTests, ASequenceOfTheBenchThatDoesNotExistIsAWarningAndTheActiveOnePlays)
{
    Rig rig{};
    const std::vector<RosterEntry> roster{ Human(1) };

    DirectorFile file{ FileOf({ FixedShot("a", Seconds(5.0f), Vec3{ 1.0f, 0.0f, 0.0f }), FixedShot("b", Seconds(5.0f), Vec3{ 2.0f, 0.0f, 0.0f }) }) };
    file.Sequences.push_back(SequenceOf("Main", { 2 }));
    file.ActiveSequence = "Main";
    rig.Store.SetFile(file);

    rig.Store.SetLoadRequest("Nope", 0);
    ASSERT_TRUE(rig.Service.Observe(1, false));

    EXPECT_FLOAT_EQ(rig.Service.Update(NoTick(0.0)).Eye.X, 2.0f);
    rig.Service.Update(Input(1.0, roster));
    rig.Service.Update(Input(2.0, roster));

    // One WARNING, said once.
    ASSERT_EQ(rig.Sink.CountOf(LogLevel::Warning), 1u);
    const auto warning{ std::ranges::find(rig.Sink.m_Messages, LogLevel::Warning, &Common::Log::Type::LogMessage::Level) };
    EXPECT_EQ(warning->Tag, "[DirectorService]");
    EXPECT_NE(warning->Message.find("Nope"), std::string::npos);

    // The one that plays is the active sequence.
    EXPECT_EQ(rig.Store.GetStatus().SequenceName, "Main");
}

TEST(DirectorTests, AStartThatIsNotNewerDoesNotTakeTheRequestOfTheBenchAndAClearedOneIsNeverTaken)
{
    Rig rig{};

    DirectorFile file{ FileOf({ FixedShot("a", Seconds(5.0f), Vec3{ 1.0f, 0.0f, 0.0f }) }) };
    file.Sequences.push_back(SequenceOf("Bench", { 1 }));
    rig.Store.SetFile(file);

    // The first key seen (5) is a start, but the bench asked after 5: it is not for that match, so nothing plays.
    rig.Store.SetLoadRequest("Bench", 5);
    EXPECT_FALSE(rig.Service.Observe(5, false));
    EXPECT_FALSE(rig.Store.IsPlaying());

    // The match that follows takes it.
    EXPECT_TRUE(rig.Service.Observe(6, false));
    EXPECT_TRUE(rig.Store.IsPlaying());

    // A match that failed to start (the bench cleared it) leaves nothing for the next one.
    rig.Store.SetPlaying(false);
    rig.Store.ConsumePlayFromStart();
    rig.Store.SetLoadRequest("Bench", 6);
    rig.Store.ClearLoadRequest();
    EXPECT_FALSE(rig.Service.Observe(7, false));
    EXPECT_FALSE(rig.Store.IsPlaying());
}

TEST(DirectorTests, ANewMatchThatDoesNotAskForTheSequenceOfTheBenchForgetsItAndTheWarningIsSaidOncePerMatch)
{
    Rig rig{};
    const std::vector<RosterEntry> roster{ Human(1) };

    DirectorFile file{ FileOf({ FixedShot("a", Seconds(50.0f), Vec3{ 1.0f, 0.0f, 0.0f }), FixedShot("b", Seconds(50.0f), Vec3{ 2.0f, 0.0f, 0.0f }) }) };
    file.Sequences.push_back(SequenceOf("Main", { 1 }));
    file.Sequences.push_back(SequenceOf("Bench", { 2 }));
    file.ActiveSequence = "Main";
    rig.Store.SetFile(file);

    // Match 1 asks for a sequence that does not exist: one WARNING.
    rig.Store.SetMatchId(1);
    rig.Store.SetLoadRequest("Nope", 0);
    ASSERT_TRUE(rig.Service.Observe(1, false));
    rig.Service.Update(NoTick(0.0));
    rig.Service.Update(Input(1.0, roster));
    ASSERT_EQ(rig.Sink.CountOf(LogLevel::Warning), 1u);

    // Match 2 asks for it again: said again, once for this match.
    rig.Store.SetMatchId(2);
    rig.Store.SetLoadRequest("Nope", 1);
    ASSERT_TRUE(rig.Service.Observe(2, false));
    rig.Service.Update(Input(2.0, roster));
    rig.Service.Update(Input(3.0, roster));
    EXPECT_EQ(rig.Sink.CountOf(LogLevel::Warning), 2u);

    // Match 3 asks for the Bench one and plays it.
    rig.Store.SetMatchId(3);
    rig.Store.SetLoadRequest("Bench", 2);
    ASSERT_TRUE(rig.Service.Observe(3, false));
    rig.Service.Update(Input(4.0, roster));
    EXPECT_EQ(rig.Store.GetStatus().SequenceName, "Bench");

    // Match 4 asks for nothing and the director does not stop: the sequence of the bench does not stay, the active one takes its place.
    rig.Store.SetMatchId(4);
    EXPECT_FALSE(rig.Service.Observe(4, false));
    rig.Service.Update(Input(5.0, roster));
    EXPECT_EQ(rig.Store.GetStatus().SequenceName, "Main");
}

// --- The file of the map ---

TEST(DirectorTests, TheFileOfTheMapIsLoadedWhenTheMapChangesAndSavedAfterEachEdit)
{
    TempFolder folder{};
    const std::string directory{ folder.Path.string() };
    const std::filesystem::path path{ folder.Path / "Director" / "Map_M.1.txt" };

    Rig rig{};
    rig.Store.SetMapName("Map M.1");
    rig.Service.Persist(directory);

    EXPECT_EQ(rig.Store.GetFileInfo().MapKey, "Map_M.1");
    EXPECT_EQ(rig.Store.GetFileInfo().Message, "no file yet");
    EXPECT_EQ(std::filesystem::path{ rig.Store.GetFileInfo().Path }, path);

    // The ids are asked before the edit: the store holds its lock while it runs.
    Shot a{ FixedShot("a", Seconds(7.0f), Vec3{ 1.0f, 2.0f, 3.0f }, 0.25f, -0.5f) };
    a.Id = rig.Store.AllocateShotId();
    Shot b{ PovShot("b", Rounds(2.0f)) };
    b.Id = rig.Store.AllocateShotId();

    rig.Store.Edit([&a, &b](DirectorFile& file) {
        AddShot(file, a);
        AddShot(file, b);
        NewSequence(file, "Main");
        file.ActiveSequence = "Main";
        return true;
    });
    rig.Service.Persist(directory);
    EXPECT_TRUE(std::filesystem::exists(path));
    EXPECT_EQ(rig.Store.GetFileInfo().Message, "saved 2 shot(s), 1 sequence(s)");

    // Another map has its own file.
    rig.Store.SetMapName("Other");
    rig.Service.Persist(directory);
    EXPECT_TRUE(rig.Store.CopyFile().Shots.empty());

    // The first one comes back, with its sequence, and a new session reads the same.
    rig.Store.SetMapName("Map M.1");
    rig.Service.Persist(directory);
    const DirectorFile back{ rig.Store.CopyFile() };
    ASSERT_EQ(back.Shots.size(), 2u);
    EXPECT_EQ(back.Shots[0].Name, "a");
    EXPECT_EQ(back.Shots[1].End.Kind, ShotEndKind::Rounds);
    EXPECT_EQ(back.ActiveSequence, "Main");
    EXPECT_EQ(rig.Store.GetFileInfo().Message, "loaded 2 shot(s), 1 sequence(s)");

    // A new shot of the session never gets an id of the file or one that was given.
    EXPECT_EQ(rig.Store.AllocateShotId(), 3u);
}

TEST(DirectorTests, AFileOfTheFirstVersionWithOneShotIsMigratedAndTheMessageSaysSoOnLoadAndOnTheFirstSave)
{
    // One line, with and without a final line break, as the user's file has.
    for (const std::string_view text : { std::string_view{ "fixed|Corner|20|1 2 3|90 0" }, std::string_view{ "fixed|Corner|20|1 2 3|90 0\r\n" } })
    {
        const auto parsed{ ParseDirectorFile(text) };
        EXPECT_TRUE(parsed.IsMigrated);
        EXPECT_EQ(parsed.Skipped, 0u);
        ASSERT_EQ(parsed.File.Shots.size(), 1u);
        EXPECT_EQ(parsed.File.Shots[0].Id, 1u);
        EXPECT_TRUE(parsed.File.Shots[0].IsEnabled);
        EXPECT_FLOAT_EQ(parsed.File.Shots[0].End.Value, 20.0f);
    }

    TempFolder folder{};
    const std::string directory{ folder.Path.string() };
    const std::filesystem::path path{ folder.Path / "Director" / "Map_M.1.txt" };

    std::filesystem::create_directories(path.parent_path());
    {
        std::ofstream file{ path, std::ios::binary | std::ios::trunc };
        file << "fixed|Corner|20|1 2 3|90 0\r\n";
    }

    Rig rig{};
    rig.Store.SetMapName("Map M.1");
    rig.Service.Persist(directory);

    // The panel shows this message (the Director section draws DirectorFileInfo.Message).
    EXPECT_EQ(rig.Store.GetFileInfo().Message, "loaded 1 shot(s), 0 sequence(s), migrated from the first version (saved as the new one at the next edit)");

    // The first save replaces it, so it keeps saying that the file was migrated.
    rig.Store.Edit([](DirectorFile& file) {
        file.Shots[0].IsEnabled = false;
        return true;
    });
    rig.Service.Persist(directory);
    EXPECT_NE(rig.Store.GetFileInfo().Message.find("saved 1 shot(s), 0 sequence(s), migrated"), std::string::npos);

    // The next one is a normal save.
    rig.Store.Edit([](DirectorFile& file) {
        file.Shots[0].IsEnabled = true;
        return true;
    });
    rig.Service.Persist(directory);
    EXPECT_EQ(rig.Store.GetFileInfo().Message, "saved 1 shot(s), 0 sequence(s)");
}

TEST(DirectorTests, AFileOfTheFirstVersionIsMigratedOnLoadAndWrittenAsTheSecondAtTheNextEditKeepingTheOldText)
{
    TempFolder folder{};
    const std::string directory{ folder.Path.string() };
    const std::filesystem::path path{ folder.Path / "Director" / "Map_M.1.txt" };
    const std::string firstVersion{ "# shots\r\nfixed|Corner|12|1 2 3|90 0\r\nnot a shot\r\npov|Bot|8\r\n" };

    std::filesystem::create_directories(path.parent_path());
    {
        std::ofstream file{ path, std::ios::binary | std::ios::trunc };
        file << firstVersion;
    }

    Rig rig{};
    rig.Store.SetMapName("Map M.1");
    rig.Service.Persist(directory);

    const DirectorFile loaded{ rig.Store.CopyFile() };
    ASSERT_EQ(loaded.Shots.size(), 2u);
    EXPECT_EQ(loaded.Shots[0].Id, 1u);
    EXPECT_EQ(loaded.Shots[1].Id, 2u);
    EXPECT_EQ(loaded.Shots[0].End.Kind, ShotEndKind::Seconds);
    EXPECT_FLOAT_EQ(loaded.Shots[0].End.Value, 12.0f);
    EXPECT_NE(rig.Store.GetFileInfo().Message.find("1 invalid line(s) skipped"), std::string::npos);
    EXPECT_NE(rig.Store.GetFileInfo().Message.find("migrated"), std::string::npos);

    // Loading does not save, so the file stays as it was until an edit.
    rig.Service.Persist(directory);
    EXPECT_EQ(ReadAll(path), firstVersion);

    rig.Store.Edit([](DirectorFile& file) {
        file.Shots[0].IsEnabled = false;
        return true;
    });
    rig.Service.Persist(directory);

    const std::string saved{ ReadAll(path) };
    EXPECT_TRUE(saved.starts_with("version 2\n"));
    EXPECT_EQ(ParseDirectorFile(saved).File.Shots.size(), 2u);
    EXPECT_FALSE(ParseDirectorFile(saved).File.Shots[0].IsEnabled);

    std::filesystem::path backup{ path };
    backup += ".v1.bak";
    EXPECT_EQ(ReadAll(backup), firstVersion);

    // The next edits do not touch the backup.
    rig.Store.Edit([](DirectorFile& file) {
        file.Shots[1].IsEnabled = false;
        return true;
    });
    rig.Service.Persist(directory);
    EXPECT_EQ(ReadAll(backup), firstVersion);
}

TEST(DirectorTests, WithoutTheArtemisFolderTheFileLivesInMemoryAndSaysSo)
{
    Rig rig{};

    rig.Service.Persist("");
    rig.Store.Edit([](DirectorFile& file) {
        Shot shot{ PovShot("a") };
        shot.Id = 1;
        AddShot(file, shot);
        return true;
    });
    rig.Service.Persist("");

    EXPECT_EQ(rig.Store.CopyFile().Shots.size(), 1u);
    EXPECT_TRUE(rig.Store.GetFileInfo().Path.empty());
    EXPECT_NE(rig.Store.GetFileInfo().Message.find("not saved"), std::string::npos);
}