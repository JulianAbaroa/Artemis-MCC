#include "pch.h"
#include "Core/Systems/Domain/Map/Sbsp/SbspSeamLinker.h"
#include <cmath>

// Algorithm:
//
// For every ordered pair (iA, iB) where iA != iB:
//   For every StructureSeam in sbspObjects[iA]:
//     For every SeamCluster in that seam:
//       centroidA = SeamCluster.Centroid  (world space)
//       clusterIdxA = SeamCluster.ClusterIndex
//
//       For every StructureSeam in sbspObjects[iB]:
//         For every SeamCluster in that seam:
//           centroidB = SeamCluster.Centroid
//           clusterIdxB = SeamCluster.ClusterIndex
//
//           if dist(centroidA, centroidB) < threshold:
//             → geometries[iA].Clusters[clusterIdxA] gets a CrossLink
//               pointing to (iB, clusterIdxB, centroidA)
//
// We iterate all ordered pairs so both directions are recorded.
// Deduplication prevents adding the same cross-link twice if multiple
// seam clusters happen to match the same remote cluster.
void SbspSeamLinker::LinkSeams(
    std::vector<SbspGeometry>& geometries,
    const std::vector<const SbspObject*>& sbspObjects,
    float threshold)
{
    const int32_t sbspCount = static_cast<int32_t>(geometries.size());

    if (sbspCount < 2) return; // Nothing to link.

    const float thresholdSq = threshold * threshold;

    for (int32_t iA = 0; iA < sbspCount; ++iA)
    {
        const SbspObject* objA = sbspObjects[iA];
        if (!objA) continue;

        SbspGeometry& geoA = geometries[iA];
        const int32_t clusterCountA = static_cast<int32_t>(geoA.Clusters.size());

        for (int32_t iB = 0; iB < sbspCount; ++iB)
        {
            if (iB == iA) continue;

            const SbspObject* objB = sbspObjects[iB];
            if (!objB) continue;

            const SbspGeometry& geoB = geometries[iB];
            const int32_t clusterCountB = static_cast<int32_t>(geoB.Clusters.size());

            // Walk every seam in BSP A.
            for (const auto& seamA : objA->StructureSeams)
            {
                for (const auto& scA : seamA.SeamClusters)
                {
                    const int32_t clusterIdxA = scA.ClusterIndex;

                    if (clusterIdxA < 0 || clusterIdxA >= clusterCountA)
                        continue;

                    const SbspVec3 centroidA = {
                        scA.Centroid.X,
                        scA.Centroid.Y,
                        scA.Centroid.Z
                    };

                    // Walk every seam in BSP B looking for a centroid match.
                    for (const auto& seamB : objB->StructureSeams)
                    {
                        for (const auto& scB : seamB.SeamClusters)
                        {
                            const int32_t clusterIdxB = scB.ClusterIndex;

                            if (clusterIdxB < 0 || clusterIdxB >= clusterCountB)
                                continue;

                            const SbspVec3 centroidB = {
                                scB.Centroid.X,
                                scB.Centroid.Y,
                                scB.Centroid.Z
                            };

                            if (DistanceSquared(centroidA, centroidB) > thresholdSq)
                                continue;

                            // Match found, check for duplicate before adding.
                            SbspCluster& clusterA = geoA.Clusters[clusterIdxA];

                            bool alreadyLinked = false;
                            for (const auto& existing : clusterA.CrossLinks)
                            {
                                if (existing.RemoteSbspIndex == iB &&
                                    existing.RemoteClusterIndex == clusterIdxB)
                                {
                                    alreadyLinked = true;
                                    break;
                                }
                            }

                            if (!alreadyLinked)
                            {
                                SbspCrossLink link;
                                link.RemoteSbspIndex = iB;
                                link.RemoteClusterIndex = clusterIdxB;
                                link.ConnectionPoint = centroidA;

                                clusterA.CrossLinks.push_back(link);
                            }
                        }
                    }
                }
            }
        }
    }
}

// --- Helpers ---

float SbspSeamLinker::DistanceSquared(const SbspVec3& a, const SbspVec3& b)
{
    const float dx = a.X - b.X;
    const float dy = a.Y - b.Y;
    const float dz = a.Z - b.Z;
    return dx * dx + dy * dy + dz * dz;
}