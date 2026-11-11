export module Views.Oracle.Type:Constant;

import std;

export namespace Views::Oracle::Type::Constant
{
    // Largest difference between the distance of the engine and the distance of Artemis that still counts as the same hit, in meters.
    inline constexpr float k_DistanceTolerance{ 0.05f };

    // Range of the random rays, in meters.
    inline constexpr float k_RandomRayRange{ 100.0f };

    // How many of the worst differences the report keeps.
    inline constexpr std::size_t k_WorstCount{ 20 };

    // The cost of a ray is counted in buckets of k_HistogramBucketMicroseconds up to k_HistogramBucketCount of them. The last one takes the rest.
    inline constexpr std::size_t k_HistogramBucketCount{ 2000 };
    inline constexpr float k_HistogramBucketMicroseconds{ 0.25f };
}