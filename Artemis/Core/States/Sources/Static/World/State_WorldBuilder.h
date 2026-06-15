#pragma once

#include "Core/Types/Sources/Static/World/NavigationGraph.h"
#include "Core/Types/Sources/Static/World/SbspGeometry.h"
#include "Core/Types/Sources/Static/World/CollGeometry.h"
#include "Core/Types/Sources/Static/World/PhmoGeometry.h"
#include "Core/Types/Sources/Static/World/ModeGeometry.h"

#include <unordered_map>
#include <cassert>
#include <atomic>

using CollGeometries = std::unordered_map<std::string, const CollGeometry>;
using ModeGeometries = std::unordered_map<std::string, const ModeGeometry>;
using SbspGeometries = std::vector<SbspGeometry>;
using NavigationGraphs = std::vector<NavigationGraph>;

class State_WorldBuilder
{
public:
    // Coll.
    bool HasCollGeometry(const std::string& tagName) const;
    const CollGeometry* GetCollGeometry(const std::string& tagName) const;
    void AddCollGeometry(const std::string& tagName, CollGeometry geometry);

    // Mode.
    bool HasModeGeometry(const std::string& tagName) const;
    const ModeGeometry* GetModeGeometry(const std::string& tagName) const;
    void AddModeGeometry(const std::string& tagName, ModeGeometry geometry);

    // Sbsp.
    bool HasSbspGeometry() const;
    const SbspGeometry* GetSbspGeometry(int32_t index) const;
    void AddSbspGeometry(SbspGeometry geometry);
    const SbspGeometries& GetAllSbspGeometry() const;
    int32_t GetSbspGeometryCount() const;

    // Navigation graph.
    const std::vector<NavigationGraph>& GetNavigationGraph() const;
    void SetNavigationGraph(std::vector<NavigationGraph> clusters);

    size_t GetNodeCount(const std::string& tagName) const;

    std::string ResolveObjectCollName(const std::string& objectTagName) const;
    void LinkObjectColl(const std::string& objectTagName, const std::string& collTagName);

    std::string ResolveObjectHlmtName(const std::string& objectTagName) const;
    void LinkObjectHlmt(const std::string& objectTagName, const std::string& hlmtTagName);

    bool IsFrozen() const 
    { 
        return m_Frozen.load(std::memory_order_acquire); 
    }

    void Freeze()
    {
        m_Frozen.store(true, std::memory_order_release);
    }

    void Cleanup();

private:
    CollGeometries m_CollGeometries;
    ModeGeometries m_ModeGeometries;
    SbspGeometries m_SbspGeometries;
    NavigationGraphs m_NavigationGraphs;

    std::unordered_map<std::string, std::string> m_ObjectColls;
    std::unordered_map<std::string, std::string> m_ObjectHlmts;

    std::atomic<bool> m_Frozen{ false };
};