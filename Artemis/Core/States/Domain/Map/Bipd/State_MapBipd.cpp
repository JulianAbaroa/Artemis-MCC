#include "pch.h"
#include "Core/States/Domain/Map/Bipd/State_MapBipd.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Interface/System_Debug.h"

bool State_MapBipd::HasBipd(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Bipds.count(tagName) > 0;
}

const BipdObject* State_MapBipd::GetBipd(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Bipds.find(tagName);
    return it != m_Bipds.end() ? &it->second : nullptr;
}

void State_MapBipd::AddBipd(const std::string& tagName, BipdObject data)
{
    //this->LogRawData(tagName, data);

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Bipds.emplace(tagName, std::move(data));
}

void State_MapBipd::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Bipds.clear();
}

void State_MapBipd::LogRawData(const std::string& tagName, const BipdObject& bipd)
{
    const auto& d = bipd.Data;

    g_pSystem->Debug->Log("=== BIPD RAW: %s ===", tagName.c_str());

    // --- Base ---
    g_pSystem->Debug->Log("  [Base]");
    g_pSystem->Debug->Log("    BoundingRadius:              %.3f", d.BoundingRadius);
    g_pSystem->Debug->Log("    Flags_5:                     0x%08X", d.Flags_5);
    g_pSystem->Debug->Log("      TurnsWithoutAnimating:     %d", (d.Flags_5 >> 0) & 1);
    g_pSystem->Debug->Log("      HasPhysRigidBodiesAlive:   %d", (d.Flags_5 >> 1) & 1);
    g_pSystem->Debug->Log("      ImmuneToFallingDamage:     %d", (d.Flags_5 >> 2) & 1);
    g_pSystem->Debug->Log("      HasAnimatedJetpack:        %d", (d.Flags_5 >> 3) & 1);
    g_pSystem->Debug->Log("      StunnedByEMPDamage:        %d", (d.Flags_5 >> 9) & 1);
    g_pSystem->Debug->Log("      DeadPhysicsWhenStunned:    %d", (d.Flags_5 >> 10) & 1);
    g_pSystem->Debug->Log("      AlwaysRagdollWhenDead:     %d", (d.Flags_5 >> 11) & 1);
    g_pSystem->Debug->Log("    Flags_6:                     0x%08X", d.Flags_6);
    g_pSystem->Debug->Log("      CenteredAtOrigin:          %d", (d.Flags_6 >> 0) & 1);
    g_pSystem->Debug->Log("      ShapeSpherical:            %d", (d.Flags_6 >> 1) & 1);
    g_pSystem->Debug->Log("      UsePlayerPhysics:          %d", (d.Flags_6 >> 2) & 1);
    g_pSystem->Debug->Log("      ClimbAnySurface:           %d", (d.Flags_6 >> 3) & 1);
    g_pSystem->Debug->Log("      Flying:                    %d", (d.Flags_6 >> 4) & 1);
    g_pSystem->Debug->Log("      NotPhysical:               %d", (d.Flags_6 >> 5) & 1);

    // --- Physics Shape ---
    g_pSystem->Debug->Log("  [PhysicsShape]");
    g_pSystem->Debug->Log("    HeightStanding:              %.3f", d.HeightStanding);
    g_pSystem->Debug->Log("    HeightCrouching:             %.3f", d.HeightCrouching);
    g_pSystem->Debug->Log("    Radius:                      %.3f", d.Radius);
    g_pSystem->Debug->Log("    Mass:                        %.3f", d.Mass);

    // --- Movement ---
    g_pSystem->Debug->Log("  [Movement]");
    g_pSystem->Debug->Log("    MaximumVelocity:             %.3f", d.MaximumVelocity);
    g_pSystem->Debug->Log("    MaximumSidestepVelocity:     %.3f", d.MaximumSidestepVelocity);
    g_pSystem->Debug->Log("    Acceleration:                %.3f", d.Acceleration);
    g_pSystem->Debug->Log("    Deceleration:                %.3f", d.Deceleration);
    g_pSystem->Debug->Log("    AngularVelocityMaximum:      %.3f", d.AngularVelocityMaximum);
    g_pSystem->Debug->Log("    AngularAccelerationMaximum:  %.3f", d.AngularAccelerationMaximum);
    g_pSystem->Debug->Log("    CrouchVelocityModifier:      %.3f", d.CrouchVelocityModifier);
    g_pSystem->Debug->Log("    JumpVelocity:                %.3f", d.JumpVelocity);
    g_pSystem->Debug->Log("    StationaryTurningThreshold:  %.3f", d.StationaryTurningThreshold);
    g_pSystem->Debug->Log("    MovingTurningSpeed:          %.3f", d.MovingTurningSpeed);

    // --- Slopes ---
    g_pSystem->Debug->Log("  [Slopes]");
    g_pSystem->Debug->Log("    MaximumSlopeAngle:           %.3f", d.MaximumSlopeAngle);
    g_pSystem->Debug->Log("    DownhillFalloffAngle:        %.3f", d.DownhillFalloffAngle);
    g_pSystem->Debug->Log("    DownhillCutoffAngle:         %.3f", d.DownhillCutoffAngle);
    g_pSystem->Debug->Log("    UphillFalloffAngle:          %.3f", d.UphillFalloffAngle);
    g_pSystem->Debug->Log("    UphillCutoffAngle:           %.3f", d.UphillCutoffAngle);
    g_pSystem->Debug->Log("    DownhillVelocityScale:       %.3f", d.DownhillVelocityScale);
    g_pSystem->Debug->Log("    UphillVelocityScale:         %.3f", d.UphillVelocityScale);
    g_pSystem->Debug->Log("    ClimbInflectionAngle:        %.3f", d.ClimbInflectionAngle);
    g_pSystem->Debug->Log("    GravityScale:                %.3f", d.GravityScale);
    g_pSystem->Debug->Log("    AirborneAccelerationScale:   %.3f", d.AirborneAccelerationScale);

    // --- Landing ---
    g_pSystem->Debug->Log("  [Landing]");
    g_pSystem->Debug->Log("    MaximumSoftLandingTime:      %.3f", d.MaximumSoftLandingTime);
    g_pSystem->Debug->Log("    MaximumHardLandingTime:      %.3f", d.MaximumHardLandingTime);
    g_pSystem->Debug->Log("    MinimumSoftLandingVelocity:  %.3f", d.MinimumSoftLandingVelocity);
    g_pSystem->Debug->Log("    MinimumHardLandingVelocity:  %.3f", d.MinimumHardLandingVelocity);
    g_pSystem->Debug->Log("    MaximumHardLandingVelocity:  %.3f", d.MaximumHardLandingVelocity);
    g_pSystem->Debug->Log("    TerminalVelocityFallRatio:   %.3f", d.TerminalVelocityFallRatio);

    // --- Stun ---
    g_pSystem->Debug->Log("  [Stun]");
    g_pSystem->Debug->Log("    StunDuration:                %.3f", d.StunDuration);
    g_pSystem->Debug->Log("    StunMovementPenalty:         %.3f", d.StunMovementPenalty);
    g_pSystem->Debug->Log("    StunTurningPenalty:          %.3f", d.StunTurningPenalty);
    g_pSystem->Debug->Log("    StunJumpingPenalty:          %.3f", d.StunJumpingPenalty);
    g_pSystem->Debug->Log("    MinimumStunTime:             %.3f", d.MinimumStunTime);
    g_pSystem->Debug->Log("    MaximumStunTime:             %.3f", d.MaximumStunTime);

    // --- Combat ---
    g_pSystem->Debug->Log("  [Combat]");
    g_pSystem->Debug->Log("    AutoaimWidth:                %.3f", d.AutoaimWidth);
    g_pSystem->Debug->Log("    AimingVelocityMaximum:       %.3f", d.AimingVelocityMaximum);
    g_pSystem->Debug->Log("    AimingAccelerationMaximum:   %.3f", d.AimingAccelerationMaximum);
    g_pSystem->Debug->Log("    LookingVelocityMaximum:      %.3f", d.LookingVelocityMaximum);
    g_pSystem->Debug->Log("    LookingAccelerationMaximum:  %.3f", d.LookingAccelerationMaximum);
    g_pSystem->Debug->Log("    SoftPingThreshold:           %.3f", d.SoftPingThreshold);
    g_pSystem->Debug->Log("    HardPingThreshold:           %.3f", d.HardPingThreshold);
    g_pSystem->Debug->Log("    HardDeathThreshold:          %.3f", d.HardDeathThreshold);

    // --- Camera Heights ---
    g_pSystem->Debug->Log("  [CameraHeights]");
    g_pSystem->Debug->Log("    StandingCameraHeight:        %.3f", d.StandingCameraHeight);
    g_pSystem->Debug->Log("    CrouchingCameraHeight:       %.3f", d.CrouchingCameraHeight);
    g_pSystem->Debug->Log("    CrouchTransitionTime:        %.3f", d.CrouchTransitionTime);

    // --- Seats ---
    g_pSystem->Debug->Log("  [Seats: %zu]", bipd.Seats.size());
    for (size_t i = 0; i < bipd.Seats.size(); ++i)
    {
        const auto& s = bipd.Seats[i];
        g_pSystem->Debug->Log("    [%zu] AiSeatType:%u EntryRadius:%.3f"
            " EntryMarkerConeAngle:%.3f EntryMarkerFacingAngle:%.3f"
            " MaxRelativeVelocity:%.3f PingScale:%.3f TurnoverTime:%.3f",
            i,
            (uint32_t)s.AiSeatType,
            s.EntryRadius,
            s.EntryMarkerConeAngle,
            s.EntryMarkerFacingAngle,
            s.MaximumRelativeVelocity,
            s.PingScale,
            s.TurnoverTime);
    }

    // --- PillShapes ---
    g_pSystem->Debug->Log("  [PillShapes: %zu]", bipd.PillShapes.size());
    for (size_t i = 0; i < bipd.PillShapes.size(); ++i)
    {
        const auto& ps = bipd.PillShapes[i];
        g_pSystem->Debug->Log("    [%zu] Radius:%.3f Mass:%.3f Friction:%.3f"
            " Restitution:%.3f Bottom:[%.3f,%.3f,%.3f] Top:[%.3f,%.3f,%.3f]",
            i,
            ps.Radius, ps.Mass, ps.Friction, ps.Restitution,
            ps.Bottom.X, ps.Bottom.Y, ps.Bottom.Z,
            ps.Top.X, ps.Top.Y, ps.Top.Z);
    }

    // --- SphereShapes ---
    g_pSystem->Debug->Log("  [SphereShapes: %zu]", bipd.SphereShapes.size());
    for (size_t i = 0; i < bipd.SphereShapes.size(); ++i)
    {
        const auto& ss = bipd.SphereShapes[i];
        g_pSystem->Debug->Log("    [%zu] Radius:%.3f Mass:%.3f Friction:%.3f Restitution:%.3f",
            i, ss.Radius, ss.Mass, ss.Friction, ss.Restitution);
    }

    g_pSystem->Debug->Log("=== END BIPD: %s ===", tagName.c_str());
}