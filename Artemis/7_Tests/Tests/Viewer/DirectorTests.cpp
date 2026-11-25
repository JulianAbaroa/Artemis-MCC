#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Math.Type;
import DevTools.Viewer.Roster.Type;
import DevTools.Viewer.Director.Type;
import DevTools.Viewer.Director.State;
import DevTools.Viewer.Director.System;
import std;

// The director of the Viewer (VW.14): the pure shots (file format, orbit, advance, POV agent) and the playback of the service with a fake clock, built
// from their own sources.

namespace
{
    using std::chrono::steady_clock;

    using Common::Engine::Type::k_NoHandle;
    using Common::Math::Type::Vec3;
    using DevTools::Viewer::Director::State::DirectorStore;
    using DevTools::Viewer::Director::System::Advance;
    using DevTools::Viewer::Director::System::DirectorService;
    using DevTools::Viewer::Director::System::DecidePovRequest;
    using DevTools::Viewer::Director::System::FormatShots;
    using DevTools::Viewer::Director::System::IsRoundChange;
    using DevTools::Viewer::Director::System::NextPlayableShot;
    using DevTools::Viewer::Director::System::OrbitFromPose;
    using DevTools::Viewer::Director::System::OrbitPose;
    using DevTools::Viewer::Director::System::ParseShots;
    using DevTools::Viewer::Director::System::PickPovAgent;
    using DevTools::Viewer::Director::System::SanitizeMapKey;
    using DevTools::Viewer::Director::System::YawPitchOf;
    using DevTools::Viewer::Director::Type::AdvanceInput;
    using DevTools::Viewer::Director::Type::DirectorFrame;
    using DevTools::Viewer::Director::Type::DirectorInput;
    using DevTools::Viewer::Director::Type::RoundKey;
    using DevTools::Viewer::Director::Type::Shot;
    using DevTools::Viewer::Director::Type::ShotKind;
    using DevTools::Viewer::Director::Type::ShotList;
    using DevTools::Viewer::Roster::Type::RosterEntry;

    constexpr float k_Pi{ std::numbers::pi_v<float> };

    auto FixedShot(std::string name, float seconds, Vec3 eye, float yaw = 0.0f, float pitch = 0.0f) -> Shot
    {
        Shot shot{};
        shot.Kind = ShotKind::Fixed;
        shot.Name = std::move(name);
        shot.Seconds = seconds;
        shot.Eye = eye;
        shot.Yaw = yaw;
        shot.Pitch = pitch;
        return shot;
    }

    auto OrbitShot(Vec3 center, float radius, float height, float startDegrees, float degreesPerSecond, float seconds = 20.0f) -> Shot
    {
        Shot shot{};
        shot.Kind = ShotKind::Orbit;
        shot.Name = "orbit";
        shot.Seconds = seconds;
        shot.Center = center;
        shot.Radius = radius;
        shot.Height = height;
        shot.StartDegrees = startDegrees;
        shot.DegreesPerSecond = degreesPerSecond;
        return shot;
    }

    auto PovShot(std::string name, float seconds = 20.0f) -> Shot
    {
        Shot shot{};
        shot.Kind = ShotKind::Pov;
        shot.Name = std::move(name);
        shot.Seconds = seconds;
        return shot;
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

    auto Input(double seconds, const std::vector<RosterEntry>& roster, std::int16_t round = 0, bool isCutOnRound = true) -> DirectorInput
    {
        return DirectorInput{ .Now = At(seconds), .HasRoster = true, .Roster = roster, .Round = round, .IsCutOnRound = isCutOnRound };
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
}

TEST(DirectorTests, TheFileOfAllThreeKindsGoesAroundAndComesBack)
{
    ShotList shots{};
    shots.push_back(FixedShot("Corner", 12.5f, Vec3{ 1.5f, -2.25f, 3.0f }, 0.5f, -0.25f));
    shots.push_back(OrbitShot(Vec3{ 10.0f, 20.0f, 30.0f }, 4.5f, 2.0f, 45.0f, -6.0f, 30.0f));
    shots.push_back(PovShot("Bot view", 8.0f));

    const std::string text{ FormatShots(shots) };
    const auto parsed{ ParseShots(text) };

    EXPECT_EQ(parsed.Skipped, 0u);
    ASSERT_EQ(parsed.Shots.size(), 3u);

    const Shot& fixed{ parsed.Shots[0] };
    EXPECT_EQ(fixed.Kind, ShotKind::Fixed);
    EXPECT_EQ(fixed.Name, "Corner");
    EXPECT_FLOAT_EQ(fixed.Seconds, 12.5f);
    ExpectNear(fixed.Eye, Vec3{ 1.5f, -2.25f, 3.0f });
    EXPECT_NEAR(fixed.Yaw, 0.5f, 1e-4f);
    EXPECT_NEAR(fixed.Pitch, -0.25f, 1e-4f);

    const Shot& orbit{ parsed.Shots[1] };
    EXPECT_EQ(orbit.Kind, ShotKind::Orbit);
    EXPECT_FLOAT_EQ(orbit.Seconds, 30.0f);
    ExpectNear(orbit.Center, Vec3{ 10.0f, 20.0f, 30.0f });
    EXPECT_FLOAT_EQ(orbit.Radius, 4.5f);
    EXPECT_FLOAT_EQ(orbit.Height, 2.0f);
    EXPECT_FLOAT_EQ(orbit.StartDegrees, 45.0f);
    EXPECT_FLOAT_EQ(orbit.DegreesPerSecond, -6.0f);

    EXPECT_EQ(parsed.Shots[2].Kind, ShotKind::Pov);
    EXPECT_EQ(parsed.Shots[2].Name, "Bot view");
    EXPECT_FLOAT_EQ(parsed.Shots[2].Seconds, 8.0f);

    // Writing what was read gives the same text.
    EXPECT_EQ(FormatShots(parsed.Shots), text);
}

TEST(DirectorTests, TheAnglesOfTheFileAreInDegrees)
{
    const auto parsed{ ParseShots("fixed|a|10|0 0 0|90 -45\n") };
    ASSERT_EQ(parsed.Shots.size(), 1u);
    EXPECT_NEAR(parsed.Shots[0].Yaw, k_Pi / 2.0f, 1e-5f);
    EXPECT_NEAR(parsed.Shots[0].Pitch, -k_Pi / 4.0f, 1e-5f);
}

TEST(DirectorTests, InvalidLinesAreSkippedAndCounted)
{
    const std::string text{
        "# a comment\r\n"
        "\r\n"
        "   \r\n"
        "pov|ok|10\r\n"
        "fixed|few fields|10|1 2 3\r\n"
        "fixed|bad number|10|1 2 x|0 0\r\n"
        "fixed|decimal comma|10|1,5 2 3|0 0\r\n"
        "fixed|no seconds|0|1 2 3|0 0\r\n"
        "fixed|negative seconds|-5|1 2 3|0 0\r\n"
        "fixed|bad seconds|ten|1 2 3|0 0\r\n"
        "orbit|three numbers|10|0 0 0|4 2 0\r\n"
        "orbit|negative radius|10|0 0 0|-4 2 0 6\r\n"
        "dolly|unknown kind|10\r\n"
        "pov|only two\r\n"
        "pov|too many|10|1\r\n"
        "orbit|ok|10|0 0 0|4 2 0 6" };

    const auto parsed{ ParseShots(text) };

    EXPECT_EQ(parsed.Shots.size(), 2u);
    EXPECT_EQ(parsed.Skipped, 11u);
    EXPECT_EQ(parsed.Shots[0].Name, "ok");
    EXPECT_EQ(parsed.Shots[1].Kind, ShotKind::Orbit);
}

TEST(DirectorTests, TheKindAndTheNamesAreReadForgivingly)
{
    const auto parsed{ ParseShots("POV |  spaced name  | 10 \n") };
    ASSERT_EQ(parsed.Shots.size(), 1u);
    EXPECT_EQ(parsed.Shots[0].Name, "spaced name");

    ShotList shots{};
    shots.push_back(PovShot("a|b\nc"));
    EXPECT_EQ(ParseShots(FormatShots(shots)).Shots[0].Name, "a b c");
}

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

TEST(DirectorTests, AShotLeavesWhenItsSecondsPass)
{
    AdvanceInput input{ .Count = 3, .Index = 0, .ElapsedSeconds = 4.9, .Seconds = 5.0f };
    EXPECT_EQ(Advance(input), std::nullopt);

    input.ElapsedSeconds = 5.0;
    EXPECT_EQ(Advance(input), std::optional<std::size_t>{ 1 });

    // After the last shot comes the first.
    input.Index = 2;
    EXPECT_EQ(Advance(input), std::optional<std::size_t>{ 0 });

    // An index out of the list goes to the first, an empty list does nothing.
    input.Index = 9;
    EXPECT_EQ(Advance(input), std::optional<std::size_t>{ 0 });
    input.Count = 0;
    EXPECT_EQ(Advance(input), std::nullopt);

    // Seconds that are not above zero count as the default.
    const AdvanceInput broken{ .Count = 2, .Index = 0, .ElapsedSeconds = 5.0, .Seconds = 0.0f };
    EXPECT_EQ(Advance(broken), std::nullopt);
}

TEST(DirectorTests, ANewRoundCutsTheShotUnlessItJustStartedOrCuttingIsOff)
{
    AdvanceInput input{ .Count = 3, .Index = 1, .ElapsedSeconds = 10.0, .Seconds = 20.0f, .IsRoundChanged = true, .IsCutOnRound = true };
    EXPECT_EQ(Advance(input), std::optional<std::size_t>{ 2 });

    // No double cut: the shot started less than 3 seconds ago.
    input.ElapsedSeconds = 2.9;
    EXPECT_EQ(Advance(input), std::nullopt);
    input.ElapsedSeconds = 3.0;
    EXPECT_EQ(Advance(input), std::optional<std::size_t>{ 2 });

    input.IsCutOnRound = false;
    input.ElapsedSeconds = 10.0;
    EXPECT_EQ(Advance(input), std::nullopt);

    input.IsCutOnRound = true;
    input.IsRoundChanged = false;
    EXPECT_EQ(Advance(input), std::nullopt);
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

TEST(DirectorTests, ThePlayableShotSkipsPovWithoutBotsAndNeverLoops)
{
    ShotList shots{};
    shots.push_back(PovShot("a"));
    shots.push_back(FixedShot("b", 5.0f, Vec3{}));
    shots.push_back(PovShot("c"));

    EXPECT_EQ(NextPlayableShot(shots, 0, 1, true), std::optional<std::size_t>{ 0 });
    EXPECT_EQ(NextPlayableShot(shots, 0, 1, false), std::optional<std::size_t>{ 1 });
    EXPECT_EQ(NextPlayableShot(shots, 2, 1, false), std::optional<std::size_t>{ 1 });
    EXPECT_EQ(NextPlayableShot(shots, 2, -1, false), std::optional<std::size_t>{ 1 });
    EXPECT_EQ(NextPlayableShot(shots, 7, 1, true), std::optional<std::size_t>{ 0 });

    ShotList onlyPov{};
    onlyPov.push_back(PovShot("a"));
    onlyPov.push_back(PovShot("b"));
    EXPECT_EQ(NextPlayableShot(onlyPov, 0, 1, false), std::nullopt);
    EXPECT_EQ(NextPlayableShot(ShotList{}, 0, 1, true), std::nullopt);

    // A shot with seconds that are not valid is not played either.
    ShotList broken{};
    broken.push_back(FixedShot("zero", 0.0f, Vec3{}));
    EXPECT_EQ(NextPlayableShot(broken, 0, 1, true), std::nullopt);
}

TEST(DirectorTests, TheMapKeyIsAFileName)
{
    EXPECT_EQ(SanitizeMapKey("Map M.1"), "Map_M.1");
    EXPECT_EQ(SanitizeMapKey("a/b\\c:d*?"), "a_b_c_d__");
    EXPECT_EQ(SanitizeMapKey(""), "default");
    EXPECT_EQ(SanitizeMapKey("..\\x"), "___x");
    EXPECT_EQ(SanitizeMapKey(std::string(200, 'a')).size(), 64u);
}

TEST(DirectorTests, ThePlaybackGoesFromShotToShotByRealSeconds)
{
    DirectorStore store{};
    DirectorService service{ store };
    const std::vector<RosterEntry> roster{ Human(1) };

    ShotList shots{};
    shots.push_back(FixedShot("a", 5.0f, Vec3{ 1.0f, 0.0f, 0.0f }));
    shots.push_back(FixedShot("b", 5.0f, Vec3{ 2.0f, 0.0f, 0.0f }));
    store.SetShots(std::move(shots));

    // Stopped: an empty frame.
    EXPECT_FALSE(service.Update(Input(0.0, roster)).IsPlaying);

    Play(store);
    DirectorFrame frame{ service.Update(Input(0.0, roster)) };
    EXPECT_TRUE(frame.IsPlaying);
    EXPECT_TRUE(frame.HasShot);
    EXPECT_EQ(frame.Kind, ShotKind::Fixed);
    EXPECT_FLOAT_EQ(frame.Eye.X, 1.0f);
    EXPECT_EQ(frame.PovPlayer, k_NoHandle);
    EXPECT_EQ(store.GetStatus().Index, 0u);

    EXPECT_FLOAT_EQ(service.Update(Input(4.9, roster)).Eye.X, 1.0f);
    EXPECT_FLOAT_EQ(service.Update(Input(5.1, roster)).Eye.X, 2.0f);
    EXPECT_EQ(store.GetStatus().Index, 1u);
    EXPECT_NEAR(store.GetStatus().ElapsedSeconds, 0.0f, 1e-3f);

    // Back to the first one.
    EXPECT_FLOAT_EQ(service.Update(Input(10.2, roster)).Eye.X, 1.0f);

    // The toggle stops, and then it has no pose and no POV player.
    Play(store);
    frame = service.Update(Input(11.0, roster));
    EXPECT_FALSE(frame.IsPlaying);
    EXPECT_FALSE(store.IsPlaying());
    EXPECT_EQ(store.GetPovPlayer(), k_NoHandle);
}

TEST(DirectorTests, AnOrbitIsPlayedByRealTime)
{
    DirectorStore store{};
    DirectorService service{ store };
    const std::vector<RosterEntry> roster{ Human(1) };

    ShotList shots{};
    shots.push_back(OrbitShot(Vec3{ 0.0f, 0.0f, 0.0f }, 5.0f, 2.0f, 0.0f, 90.0f));
    store.SetShots(std::move(shots));
    Play(store);

    DirectorFrame frame{ service.Update(Input(0.0, roster)) };
    ExpectNear(frame.Eye, Vec3{ 5.0f, 0.0f, 2.0f });

    frame = service.Update(Input(1.0, roster));
    ExpectNear(frame.Eye, Vec3{ 0.0f, 5.0f, 2.0f });
    EXPECT_NEAR(frame.Yaw, std::atan2(-5.0f, 0.0f), 1e-4f);
}

TEST(DirectorTests, ANewRoundCutsButNeverTwiceInARow)
{
    DirectorStore store{};
    DirectorService service{ store };
    const std::vector<RosterEntry> roster{ Human(1) };

    ShotList shots{};
    shots.push_back(FixedShot("a", 20.0f, Vec3{ 1.0f, 0.0f, 0.0f }));
    shots.push_back(FixedShot("b", 20.0f, Vec3{ 2.0f, 0.0f, 0.0f }));
    store.SetShots(std::move(shots));
    Play(store);

    service.Update(Input(0.0, roster, 0));
    service.Update(Input(1.0, roster, 0));
    EXPECT_EQ(store.GetStatus().Index, 0u);

    // Round 1 cuts to the second shot.
    service.Update(Input(5.0, roster, 1));
    EXPECT_EQ(store.GetStatus().Index, 1u);

    // Round 2 right after the cut (less than 3 seconds): no second cut.
    service.Update(Input(6.0, roster, 2));
    EXPECT_EQ(store.GetStatus().Index, 1u);

    // A later round cuts again, back to the first.
    service.Update(Input(9.0, roster, 3));
    EXPECT_EQ(store.GetStatus().Index, 0u);

    // A new match is a new round too.
    store.SetMatchId(5);
    service.Update(Input(13.0, roster, 3));
    EXPECT_EQ(store.GetStatus().Index, 1u);

    // With cutting off the shot goes on until its seconds pass.
    service.Update(Input(14.0, roster, 4, false));
    service.Update(Input(18.0, roster, 5, false));
    EXPECT_EQ(store.GetStatus().Index, 1u);
}

TEST(DirectorTests, APovShotShowsTheNextBotEachTimeAndIsSkippedWithoutBots)
{
    DirectorStore store{};
    DirectorService service{ store };

    ShotList shots{};
    shots.push_back(PovShot("a", 5.0f));
    shots.push_back(FixedShot("b", 5.0f, Vec3{ 2.0f, 0.0f, 0.0f }));
    store.SetShots(std::move(shots));
    Play(store);

    // No bots: the POV shot is skipped and the fixed one plays.
    const std::vector<RosterEntry> humans{ Human(1) };
    DirectorFrame frame{ service.Update(Input(0.0, humans)) };
    EXPECT_EQ(frame.Kind, ShotKind::Fixed);
    EXPECT_EQ(store.GetPovPlayer(), k_NoHandle);

    // With two bots the POV shot comes again after the fixed one and each time it shows the next bot.
    const std::vector<RosterEntry> bots{ Human(1), Bot(10, 0), Bot(11, 1) };
    frame = service.Update(Input(5.1, bots));
    EXPECT_EQ(frame.Kind, ShotKind::Pov);
    EXPECT_EQ(frame.PovPlayer, 10u);
    EXPECT_EQ(store.GetPovPlayer(), 10u);

    service.Update(Input(10.2, bots));
    frame = service.Update(Input(15.4, bots));
    EXPECT_EQ(frame.Kind, ShotKind::Pov);
    EXPECT_EQ(frame.PovPlayer, 11u);

    // The agent leaves the table: its shot ends at once.
    const std::vector<RosterEntry> left{ Human(1), Bot(10, 0) };
    frame = service.Update(Input(16.0, left));
    EXPECT_EQ(frame.Kind, ShotKind::Fixed);
    EXPECT_EQ(store.GetPovPlayer(), k_NoHandle);

    // A bot that is still there is shown again after the fixed shot. Without a Tick nothing is known, so nothing ends.
    frame = service.Update(Input(21.5, left));
    EXPECT_EQ(frame.Kind, ShotKind::Pov);
    EXPECT_EQ(frame.PovPlayer, 10u);

    const DirectorInput noTick{ .Now = At(22.0) };
    frame = service.Update(noTick);
    EXPECT_EQ(frame.Kind, ShotKind::Pov);
    EXPECT_EQ(frame.PovPlayer, 10u);
}

TEST(DirectorTests, ALoneListOfPovShotsWithoutBotsWaitsAndRetriesOncePerSecond)
{
    DirectorStore store{};
    DirectorService service{ store };

    ShotList shots{};
    shots.push_back(PovShot("a"));
    store.SetShots(std::move(shots));
    Play(store);

    const std::vector<RosterEntry> humans{ Human(1) };
    DirectorFrame frame{ service.Update(Input(0.0, humans)) };

    // It plays and waits: no shot to show, the camera stays where it is.
    EXPECT_TRUE(frame.IsPlaying);
    EXPECT_FALSE(frame.HasShot);
    EXPECT_TRUE(store.GetStatus().IsHolding);

    frame = service.Update(Input(0.5, humans));
    EXPECT_FALSE(frame.HasShot);

    // A bot appears: the next try (after a second) finds it.
    const std::vector<RosterEntry> bots{ Human(1), Bot(10, 0) };
    frame = service.Update(Input(0.7, bots));
    EXPECT_FALSE(frame.HasShot);
    frame = service.Update(Input(1.1, bots));
    EXPECT_TRUE(frame.HasShot);
    EXPECT_EQ(frame.PovPlayer, 10u);
    EXPECT_FALSE(store.GetStatus().IsHolding);
}

TEST(DirectorTests, AnEmptyListDoesNotPlay)
{
    DirectorStore store{};
    DirectorService service{ store };
    const std::vector<RosterEntry> roster{ Human(1) };

    Play(store);
    EXPECT_FALSE(service.Update(Input(0.0, roster)).IsPlaying);
    EXPECT_FALSE(store.IsPlaying());
}

TEST(DirectorTests, TheStepsGoToTheNextAndPreviousShotAndAreDroppedWhenStopped)
{
    DirectorStore store{};
    DirectorService service{ store };
    const std::vector<RosterEntry> roster{ Human(1) };

    ShotList shots{};
    for (int i{ 0 }; i < 3; ++i) shots.push_back(FixedShot(std::format("s{}", i), 20.0f, Vec3{ static_cast<float>(i), 0.0f, 0.0f }));
    store.SetShots(std::move(shots));

    // A step asked while stopped is not kept for later.
    store.RequestStep(1);
    service.Update(Input(0.0, roster));
    Play(store);
    service.Update(Input(1.0, roster));
    EXPECT_EQ(store.GetStatus().Index, 0u);

    store.RequestStep(1);
    service.Update(Input(2.0, roster));
    EXPECT_EQ(store.GetStatus().Index, 1u);

    store.RequestStep(-1);
    store.RequestStep(-1);
    service.Update(Input(3.0, roster));
    EXPECT_EQ(store.GetStatus().Index, 2u);

    // The shot started again at the step.
    EXPECT_NEAR(store.GetStatus().ElapsedSeconds, 0.0f, 1e-3f);
}

TEST(DirectorTests, AnotherMapStartsItsListFromTheFirstShotAndStopStopsEverything)
{
    DirectorStore store{};
    DirectorService service{ store };
    const std::vector<RosterEntry> roster{ Human(1) };

    ShotList shots{};
    shots.push_back(FixedShot("a", 20.0f, Vec3{ 1.0f, 0.0f, 0.0f }));
    shots.push_back(FixedShot("b", 20.0f, Vec3{ 2.0f, 0.0f, 0.0f }));
    store.SetShots(shots);
    Play(store);

    service.Update(Input(0.0, roster));
    store.RequestStep(1);
    service.Update(Input(1.0, roster));
    EXPECT_EQ(store.GetStatus().Index, 1u);

    store.SetShots(shots);
    EXPECT_FLOAT_EQ(service.Update(Input(2.0, roster)).Eye.X, 1.0f);

    // A list that shrank under the playing shot goes back to the first one.
    store.RequestStep(1);
    service.Update(Input(3.0, roster));
    store.RemoveShot(1);
    EXPECT_FLOAT_EQ(service.Update(Input(4.0, roster)).Eye.X, 1.0f);

    store.RequestStep(1);
    store.RequestAddFixed();
    service.Stop();
    EXPECT_FALSE(store.IsPlaying());
    EXPECT_FALSE(store.ConsumeAddFixed());
    EXPECT_EQ(store.ConsumeStep(), 0);
    EXPECT_FALSE(service.Update(Input(5.0, roster)).IsPlaying);
}

TEST(DirectorTests, TheEditsOfTheListAreCheckedAndCounted)
{
    DirectorStore store{};
    store.AddShot(FixedShot("a", 5.0f, Vec3{}));
    store.AddShot(OrbitShot(Vec3{}, 1.0f, 1.0f, 0.0f, 6.0f));
    store.AddShot(PovShot("c"));
    EXPECT_EQ(store.GetRevision(), 3u);

    store.SetShotSeconds(0, 0.0f);
    store.SetShotSeconds(0, -3.0f);
    store.SetShotSeconds(0, std::numeric_limits<float>::infinity());
    EXPECT_FLOAT_EQ(store.GetShot(0)->Seconds, 5.0f);
    EXPECT_EQ(store.GetRevision(), 3u);

    store.SetShotSeconds(0, 9.0f);
    store.SetShotName(0, "renamed");
    store.SetShotDegreesPerSecond(0, 30.0f);
    store.SetShotDegreesPerSecond(1, -12.0f);
    EXPECT_FLOAT_EQ(store.GetShot(0)->Seconds, 9.0f);
    EXPECT_EQ(store.GetShot(0)->Name, "renamed");
    EXPECT_FLOAT_EQ(store.GetShot(1)->DegreesPerSecond, -12.0f);
    EXPECT_EQ(store.GetRevision(), 6u);

    store.MoveShot(0, 1);
    EXPECT_EQ(store.GetShot(1)->Name, "renamed");
    store.MoveShot(0, -1);
    store.MoveShot(9, 1);
    EXPECT_EQ(store.GetShot(1)->Name, "renamed");

    store.RemoveShot(0);
    store.RemoveShot(9);
    EXPECT_EQ(store.GetShotCount(), 2u);
    EXPECT_FALSE(store.GetShot(2).has_value());

    // A load changes the serial and not the revision, so it is not saved back.
    const std::uint64_t revision{ store.GetRevision() };
    store.SetShots(ShotList{});
    EXPECT_EQ(store.GetRevision(), revision);
    EXPECT_EQ(store.GetLoadSerial(), 1u);
}

TEST(DirectorTests, TheFileOfTheMapIsLoadedWhenTheMapChangesAndSavedAfterEachEdit)
{
    TempFolder folder{};
    const std::string directory{ folder.Path.string() };

    DirectorStore store{};
    DirectorService service{ store };

    store.SetMapName("Map M.1");
    service.Persist(directory);

    EXPECT_EQ(store.GetFileInfo().MapKey, "Map_M.1");
    EXPECT_EQ(store.GetFileInfo().Message, "no file yet");
    EXPECT_EQ(std::filesystem::path{ store.GetFileInfo().Path }, folder.Path / "Director" / "Map_M.1.txt");

    store.AddShot(FixedShot("a", 7.0f, Vec3{ 1.0f, 2.0f, 3.0f }, 0.25f, -0.5f));
    store.AddShot(PovShot("b", 9.0f));
    service.Persist(directory);
    EXPECT_TRUE(std::filesystem::exists(folder.Path / "Director" / "Map_M.1.txt"));
    EXPECT_EQ(store.GetFileInfo().Message, "saved 2 shot(s)");

    // Another map has its own list.
    store.SetMapName("Other");
    service.Persist(directory);
    EXPECT_EQ(store.GetShotCount(), 0u);
    store.AddShot(OrbitShot(Vec3{}, 3.0f, 1.0f, 0.0f, 6.0f));
    service.Persist(directory);

    // The first one comes back, also in a new store (a new session), with its text edited by hand next to it.
    store.SetMapName("Map M.1");
    service.Persist(directory);
    ASSERT_EQ(store.GetShotCount(), 2u);
    EXPECT_EQ(store.GetShot(0)->Name, "a");
    EXPECT_EQ(store.GetShot(1)->Kind, ShotKind::Pov);
    EXPECT_EQ(store.GetFileInfo().Message, "loaded 2 shot(s)");

    {
        std::ofstream file{ folder.Path / "Director" / "Map_M.1.txt", std::ios::binary | std::ios::trunc };
        file << "pov|only|4\r\nnot a shot\r\n";
    }

    DirectorStore second{};
    DirectorService secondService{ second };
    second.SetMapName("Map M.1");
    secondService.Persist(directory);
    EXPECT_EQ(second.GetShotCount(), 1u);
    EXPECT_EQ(second.GetFileInfo().Message, "loaded 1 shot(s), 1 invalid line(s) skipped");

    // Loading does not save, so the hand-edited file stays as it is until an edit.
    secondService.Persist(directory);
    std::ifstream file{ folder.Path / "Director" / "Map_M.1.txt", std::ios::binary };
    const std::string text{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };
    EXPECT_EQ(text, "pov|only|4\r\nnot a shot\r\n");
}

TEST(DirectorTests, WithoutTheArtemisFolderTheListLivesInMemoryAndSaysSo)
{
    DirectorStore store{};
    DirectorService service{ store };

    service.Persist("");
    store.AddShot(PovShot("a"));
    service.Persist("");

    EXPECT_EQ(store.GetShotCount(), 1u);
    EXPECT_TRUE(store.GetFileInfo().Path.empty());
    EXPECT_NE(store.GetFileInfo().Message.find("not saved"), std::string::npos);
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