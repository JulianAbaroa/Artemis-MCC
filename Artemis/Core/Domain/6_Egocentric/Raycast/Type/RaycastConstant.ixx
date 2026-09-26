export module Egocentric.Raycast.Type:Constant;

export namespace Egocentric::Raycast::Type::Constant
{
	constexpr float k_FallbackAimRange{ 50.0f };
	constexpr float k_MaxRayRange{ 150.0f };
	constexpr float k_DynamicRejectMargin{ 1.0f };
	constexpr float k_PerceptionRange{ 20.0f };
	constexpr int k_PerceptionRayCount{ 14 };
}