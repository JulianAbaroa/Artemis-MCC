#include "pch.h"

#include "System_WorldBuilder.h"

#include "Core/Types/Sources/MapReader/MapMagics.h"
#include "Core/States/Sources/MapReader/State_MapReader.h"
#include "Core/States/Sources/MapReader/Tags/State_MapBipd.h"
#include "Core/States/Sources/MapReader/Tags/State_MapBloc.h"
#include "Core/States/Sources/MapReader/Tags/State_MapColl.h"
#include "Core/States/Sources/MapReader/Tags/State_MapCtrl.h"
#include "Core/States/Sources/MapReader/Tags/State_MapEqip.h"
#include "Core/States/Sources/MapReader/Tags/State_MapHlmt.h"
#include "Core/States/Sources/MapReader/Tags/State_MapMach.h"
#include "Core/States/Sources/MapReader/Tags/State_MapMode.h"
#include "Core/States/Sources/MapReader/Tags/State_MapPhmo.h"
#include "Core/States/Sources/MapReader/Tags/State_MapSbsp.h"
#include "Core/States/Sources/MapReader/Tags/State_MapScen.h"
#include "Core/States/Sources/MapReader/Tags/State_MapVehi.h"
#include "Core/States/Sources/MapReader/Tags/State_MapWeap.h"
#include "Core/States/Sources/Static/World/State_WorldBuilder.h"

#include "Sbsp/System_SbspBuilder.h"
#include "Sbsp/System_SbspSeamLinker.h"
#include "Coll/System_CollBuilder.h"
#include "Phmo/System_PhmoBuilder.h"
#include "Mode/System_ModeBuilder.h"
#include "Core/Systems/Sources/MapReader/System_MapReader.h"
#include "Core/Systems/Sources/MapReader/Geometry/System_GeometryReader.h"
#include "Core/Systems/Other/Logs/System_Logs.h"

void System_WorldBuilder::BuildForMap()
{
    auto& sta_reader = m_Deps.State_MapReader;

    SbspGeometries geometries;
    SbspObjects sbspObjects;

    int32_t sbsp{}, coll{}, mode{};

    const int32_t tagCount = static_cast<int32_t>(
        sta_reader.GetTagsSize());

    for (int32_t i = 0; i < tagCount; ++i)
    {
        const Map_TagTableEntry& entry = sta_reader.GetTag(i);
        if (entry.TagGroupIndex < 0) continue;

        const std::string tagName = sta_reader.GetTagName(i);
        if (tagName.empty()) continue;

        const uint32_t magic = sta_reader.GetGroupMagic(
            entry.TagGroupIndex);

        if (magic == MapMagics::k_CollMagic)
        {
            if (!this->BuildColl(tagName)) continue;
            ++coll;
        }
        else if (magic == MapMagics::k_ModeMagic)
        {
            if (!this->BuildMode(tagName)) continue;
            ++mode;
        }
        else if (magic == MapMagics::k_SbspMagic)
        {
            if (!this->BuildSbsp(tagName, geometries, sbspObjects)) continue;
            ++sbsp;
        }
    }

    if (sbsp > 1)
    {
        auto& seamLinker = m_Deps.System_SbspSeamLinker;
        seamLinker.LinkSeams(geometries, sbspObjects);
    }

    auto& geometryReader = m_Deps.System_GeometryReader;
    bool readed = geometryReader.ReadRenderGeometry(geometries);
    if (!readed)
    {
        m_Deps.System_Logs.Log("[WorldBuilder] ERROR:"
            " Failed to read render geometry.");
    }

    for (auto& geometry : geometries)
    {
        m_Deps.State_WorldBuilder.AddSbspGeometry(geometry);
    }

    this->BuildNavigationGraph(geometries);

    this->LinkObjectColls();

    m_Deps.State_WorldBuilder.Freeze();

    m_Deps.System_Logs.Log("[WorldBuilder] INFO: World built."
        " Coll: %d | Mode: %d | Sbsp: %d", coll, mode, sbsp);
}

bool System_WorldBuilder::BuildColl(const std::string& tagName)
{
    const CollObject* coll = m_Deps.State_MapColl.Get(tagName);
    if (!coll)
    {
        m_Deps.System_Logs.Log("[WorldBuilder] WARNING:"
            " Coll tag found in table but not loaded: %s", 
            tagName.c_str());
        return false;
    }

    CollGeometry geometry = m_Deps.System_CollBuilder.Build(*coll);
    m_Deps.State_WorldBuilder.AddCollGeometry(tagName, std::move(geometry));
    return true;
}

bool System_WorldBuilder::BuildMode(const std::string& tagName)
{
    const ModeObject* mode = m_Deps.State_MapMode.Get(tagName);
    if (!mode)
    {
        m_Deps.System_Logs.Log("[WorldBuilder] WARNING:"
            " Mode tag found in table but not loaded: %s", 
            tagName.c_str());
        return false;
    }

    ModeGeometry geometry = m_Deps.System_ModeBuilder.Build(*mode);
    m_Deps.State_WorldBuilder.AddModeGeometry(tagName, std::move(geometry));
    return true;
}

bool System_WorldBuilder::BuildSbsp(const std::string& tagName,
    SbspGeometries& geometries, SbspObjects& sbspObjects)
{
    const SbspObject* sbsp = m_Deps.State_MapSbsp.Get(tagName);
    if (!sbsp)
    {
        m_Deps.System_Logs.Log("[WorldBuilder] WARNING:"
            " SBSP tag found in table but not loaded: %s", 
            tagName.c_str());
        return false;
    }

    int32_t sbspIndex = static_cast<int32_t>(geometries.size());
    SbspGeometry geometry = m_Deps.System_SbspBuilder
        .Build(*sbsp, sbspIndex);

    m_Deps.System_Logs.Log("[WorldBuilder] INFO:"
        " Built %d clusters, %d portals, %d markers, %d seams.",
        geometry.Clusters.size(), geometry.Portals.size(),
        geometry.Markers.size(), sbsp->StructureSeams.size());

    geometries.push_back(std::move(geometry));
    sbspObjects.push_back(sbsp);

    return true;
}

void System_WorldBuilder::BuildNavigationGraph(
    const SbspGeometries& geometries) const
{
    std::vector<NavigationGraph> clusters;

    for (int32_t i = 0; 
        i < static_cast<int32_t>(geometries.size()); 
        ++i)
    {
        const SbspGeometry& geometry = geometries[i];

        for (const SbspCluster& cluster : geometry.Clusters)
        {
            NavigationGraph navigationCluster{};

            navigationCluster.ClusterIndex = cluster.ClusterIndex;
            navigationCluster.SbspIndex = i;

            navigationCluster.Center = {
                cluster.Center.X, 
                cluster.Center.Y, 
                cluster.Center.Z
            };

            navigationCluster.BoundsMin = {
                cluster.BoundsMin.X, 
                cluster.BoundsMin.Y, 
                cluster.BoundsMin.Z
            };

            navigationCluster.BoundsMax = {
                cluster.BoundsMax.X, 
                cluster.BoundsMax.Y, 
                cluster.BoundsMax.Z
            };

            for (int32_t portalIdx : cluster.PortalIndices)
            {
                if (portalIdx < 0 || portalIdx >= 
                    static_cast<int32_t>(geometry.Portals.size()))
                {
                    continue;
                }

                const SbspPortal& portal = 
                    geometry.Portals[portalIdx];

                int32_t neighborCluster = -1;

                if (portal.BackCluster == cluster.ClusterIndex)
                {
                    neighborCluster = portal.FrontCluster;
                }
                else if (portal.FrontCluster == cluster.ClusterIndex)
                {
                    neighborCluster = portal.BackCluster;
                }

                if (neighborCluster < 0) continue;

                NavigationLink link{};
                link.ClusterIndex = neighborCluster;
                link.SbspIndex = i;

                link.Centroid = {
                    portal.Centroid.X, 
                    portal.Centroid.Y, 
                    portal.Centroid.Z
                };

                link.PassRadius = portal.BoundingRadius;

                navigationCluster.Links.push_back(link);
            }

            for (const SbspCrossLink& seam : cluster.CrossLinks)
            {
                NavigationLink link{};

                link.ClusterIndex = seam.RemoteClusterIndex;
                link.SbspIndex = seam.RemoteSbspIndex;

                link.Centroid = {
                    seam.ConnectionPoint.X,
                    seam.ConnectionPoint.Y,
                    seam.ConnectionPoint.Z
                };

                link.PassRadius = 0.0f;

                navigationCluster.Links.push_back(link);
            }

            clusters.push_back(std::move(navigationCluster));
        }
    }

    const int32_t total = static_cast<int32_t>(clusters.size());
    m_Deps.State_WorldBuilder.SetNavigationGraph(std::move(clusters));

    m_Deps.System_Logs.Log("[WorldBuilder] INFO:"
        " Navigation graph built. Total clusters: %d", total);
}

void System_WorldBuilder::Cleanup()
{
    m_Deps.State_WorldBuilder.Cleanup();

    m_Deps.System_Logs.Log("[WorldBuilder] INFO:"
        " Cleanup completed.");
}

void System_WorldBuilder::LinkObjectColls()
{
    this->LinkObjectFamily(m_Deps.State_MapBipd);
    this->LinkObjectFamily(m_Deps.State_MapVehi);
    this->LinkObjectFamily(m_Deps.State_MapWeap);
    this->LinkObjectFamily(m_Deps.State_MapEqip);
    this->LinkObjectFamily(m_Deps.State_MapScen);
    this->LinkObjectFamily(m_Deps.State_MapMach);
    this->LinkObjectFamily(m_Deps.State_MapCtrl);
    this->LinkObjectFamily(m_Deps.State_MapBloc);
}

template <typename TObject>
void System_WorldBuilder::LinkObjectFamily(const MapTag<TObject>& state)
{
    auto& mapReader = m_Deps.System_MapReader;

    for (const auto& [objectTagName, object] : state.All())
    {
        const std::string hlmtName =
            mapReader.ResolveTagRefName(object.Data.Model);
        if (hlmtName.empty()) continue;

        const HlmtObject* hlmt = m_Deps.State_MapHlmt.Get(hlmtName);
        if (!hlmt) continue;

        m_Deps.State_WorldBuilder.LinkObjectHlmt(objectTagName, hlmtName);

        const std::string collName =
            mapReader.ResolveTagRefName(hlmt->Data.CollisionModel);
        if (collName.empty()) continue;

        m_Deps.State_WorldBuilder.LinkObjectColl(objectTagName, collName);
    }
}