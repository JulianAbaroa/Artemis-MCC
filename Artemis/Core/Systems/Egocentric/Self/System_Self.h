#pragma once

#include <unordered_map>
#include <cstdint>
#include <string>
#include <array>

struct LivePlayer;

using PlayerTable = std::unordered_map<uint32_t, LivePlayer>;

class State_PlayerTable;
class State_Self;
class System_Logs;

struct Sys_Self_Deps
{
    State_PlayerTable& State_PlayerTable;
    State_Self& State_Self;
    System_Logs& System_Logs;
};

class System_Self
{
public:
    System_Self(Sys_Self_Deps deps) : m_Deps(deps) {}
    ~System_Self() = default;

    void Update();

    void Cleanup();

private:
    Sys_Self_Deps m_Deps;

    // TODO: replace name lookup with in-game local-player resolution.
    uint32_t ResolveSelfHandle(const PlayerTable& players) const;

    void BuildFrame(const std::array<float, 3>& forward,
        std::array<float, 3>& outForward,
        std::array<float, 3>& outRight,
        std::array<float, 3>& outUp) const;

    // --- Helpers ---
    std::array<float, 3> Cross(const std::array<float, 3>& a,
        const std::array<float, 3>& b) const;
    std::array<float, 3> Normalize(const std::array<float, 3>& v) const;
};