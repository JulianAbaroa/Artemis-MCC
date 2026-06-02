#pragma once

#include <string>

enum class ProjectileType : uint8_t
{
	Ballistic,
	Guided,
	BruteGrenade,
	FireBomb,
	ConicalSpread,
};

struct ProjectileConicalSpread
{
	int16_t YawCount;
	int16_t PitchCount;
	float DistributionExponent;
	float Spread;
};

struct ProjectileData
{
	std::string TagName;
	ProjectileType Type;

	// Kinematics.
	float InitialVelocity;
	float FinalVelocity;
	float AirGravityScale;
	float WaterGravityScale;
	float AccelerationRangeMin;
	float AccelerationRangeMax;
	float AirDamageRangeMin;
	float AirDamageRangeMax;
	float WaterDamageRangeMin;
	float WaterDamageRangeMax;

	// Guidance.
	float GuidedAngularVelocityLower;
	float GuidedAngularVelocityUpper;
	float GuidedAngularVelocityAtRest;
	float TargetedLeadingFraction;
	float OuterRangeErrorRadius;
	float AutoaimLeadingMaxLeadTime;
	float AiNormalVelocityScale;
	float AiHeroicVelocityScale;
	float AiLegendaryVelocityScale;
	float AiNormalGuidedAngularVelocityScale;

	// Detonation.
	float CollisionRadius;
	float ArmingTime;
	float TimerMin;
	float TimerMax;
	float MinimumVelocity;
	float MaximumRange;
	float SuperDetonationTime;
	float BoardingDetonationTime;
	float MaxLatchTimeToArm;
	uint16_t DetonationTimerStarts;

	// Danger.
	float DangerRadius;
	float DangerStimuliRadius;
	float FlybyDamageMaxDistance;
	float MaterialEffectRadius;

	// Noise
	uint16_t ImpactNoise;
	uint16_t DetonationNoise;

	// Flags
	bool MustUseBallisticAiming;
	bool IsMinorTrackingThreat;
	bool DangerousWhenInactive;

	// Derived from MaterialResponse
	bool CanBounce;

	// Conical spread (only valid when Type == ConicalSpread)
	bool HasConicalSpread;
	ProjectileConicalSpread ConicalSpreadData;
};