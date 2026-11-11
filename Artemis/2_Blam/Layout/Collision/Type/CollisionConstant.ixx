export module Blam.Layout.Collision.Type:Constant;

import std;

export namespace Blam::Layout::Collision::Type::Constant
{
    // Bytes of the collision_result that collision_test_vector fills (A20 sec. 1.4).
    constexpr std::size_t k_ResultSize{ 0x70 };
}