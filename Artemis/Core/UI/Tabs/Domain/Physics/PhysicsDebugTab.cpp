#include "pch.h"
#include "Core/UI/Tabs/Domain/Physics/PhysicsDebugTab.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Map/Phmo/MapPhmoState.h"
#include "Core/States/Domain/Tables/ObjectTableState.h"
#include "Core/States/Domain/Tables/PlayerTableState.h"
#include "External/imgui/imgui.h"
#include <algorithm>
#include <format>

void PhysicsDebugTab::Draw()
{
    ImGui::TextDisabled("Physics verification - compare data against in-game state.");
    ImGui::Separator();

    float leftWidth = ImGui::GetContentRegionAvail().x * 0.35f;

    ImGui::BeginChild("##PhysDbgLeft", ImVec2(leftWidth, 0), true);
    DrawPlayerSection();
    ImGui::Separator();
    DrawObjectList();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##PhysDbgRight", ImVec2(0, 0), true);
    if (m_SelectedHandle != 0xFFFFFFFF)
        DrawObjectDetail(m_SelectedHandle);
    else
        ImGui::TextDisabled("Select an object on the left.");
    ImGui::EndChild();
}

void PhysicsDebugTab::Cleanup()
{
    m_SelectedHandle = 0xFFFFFFFF;
}

// Player section, verify player orientation.
void PhysicsDebugTab::DrawPlayerSection()
{
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[ PLAYER ORIENTATION VERIFIER ]");
    ImGui::TextDisabled("Face North in-game, confirm Forward = (0, 1, 0).");
    ImGui::TextDisabled("Face East  in-game, confirm Forward = (1, 0, 0).");
    ImGui::TextDisabled("Up should always be (0, 0, 1).");
    ImGui::Spacing();

    // Find local player biped.
    const auto& allObjects = g_pState->Domain->ObjectTable->GetObjectTable();
    const auto& allPlayers = g_pState->Domain->PlayerTable->GetAllPlayers();

    // Use first local player's biped.
    uint32_t bipedHandle = 0xFFFFFFFF;
    for (const auto& [ph, player] : allPlayers)
    {
        bipedHandle = player.AliveBipedHandle;
        break;
    }

    if (bipedHandle == 0xFFFFFFFF)
    {
        ImGui::TextDisabled("No local player found.");
        return;
    }

    const LiveObject* obj = g_pState->Domain->ObjectTable->GetObject(bipedHandle);
    if (!obj)
    {
        ImGui::TextDisabled("Player biped not in ObjectTable.");
        return;
    }

    // Position.
    ImGui::Text("Position : (%.3f, %.3f, %.3f)",
        obj->Position[0], obj->Position[1], obj->Position[2]);

    // Forward.
    float fx = obj->Forward[0], fy = obj->Forward[1], fz = obj->Forward[2];
    float flen = sqrtf(fx * fx + fy * fy + fz * fz);
    ImGui::Text("Forward  : (%.3f, %.3f, %.3f)  len=%.3f",
        fx, fy, fz, flen);

    // Cardinal direction.
    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f),
        "Facing   : %s", ForwardToCardinal(fx, fy));

    // Up.
    ImGui::Text("Up       : (%.3f, %.3f, %.3f)",
        obj->Up[0], obj->Up[1], obj->Up[2]);

    // Sanity checks.
    bool fwdOk = (flen > 0.98f && flen < 1.02f);
    bool upOk = (fabsf(obj->Up[2] - 1.0f) < 0.05f);

    ImGui::Spacing();
    ImGui::TextColored(fwdOk ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
        fwdOk ? "Forward vector: NORMALIZED [OK]" : "Forward vector: NOT UNIT LENGTH [WARN]");
    ImGui::TextColored(upOk ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
        upOk ? "Up vector: Z~1.0 [OK]" : "Up vector: Z not ~1.0 [check]");
}

// Object list, sorted by distance to player.
void PhysicsDebugTab::DrawObjectList()
{
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[ OBJECTS ]");
    ImGui::TextDisabled("Select to inspect physics data.");
    ImGui::Spacing();

    const auto& allObjects = g_pState->Domain->ObjectTable->GetObjectTable();

    // Get player position for distance sort.
    float playerPos[3] = {};
    const auto& allPlayers = g_pState->Domain->PlayerTable->GetAllPlayers();
    for (const auto& [ph, player] : allPlayers)
    {
        const LiveObject* bip = 
            g_pState->Domain->ObjectTable->GetObject(player.AliveBipedHandle);

        if (bip) 
        {
            playerPos[0] = bip->Position[0];
            playerPos[1] = bip->Position[1];
            playerPos[2] = bip->Position[2];
        }

        break;
    }

    // Build sorted list.
    struct Entry { uint32_t handle; float dist; std::string shortName; };
    std::vector<Entry> entries;
    entries.reserve(allObjects.size());

    for (const auto& [handle, obj] : allObjects)
    {
        float d = Distance3D(obj.Position, playerPos);
        if (d > 100.0f) continue;
        entries.push_back({ handle, d, GetShortName(obj.TagName) });
    }

    std::sort(entries.begin(), entries.end(),
        [](const Entry& a, const Entry& b) { return a.dist < b.dist; });

    ImGui::TextDisabled("Showing %zu objects within 100m.", entries.size());
    ImGui::Separator();

    //for (const auto& e : entries)
    //{
    //    const LivePhmoObject* lop =
    //        g_pState->Domain->Phmo->GetPhmos(e.handle);

    //    bool hasPhys = lop && lop->HasPhysics;
    //    bool isSelected = (m_SelectedHandle == e.handle);

    //    // Color code: green=has physics, yellow=no physics, gray=selected
    //    ImVec4 col = hasPhys
    //        ? ImVec4(0.7f, 1.0f, 0.7f, 1.0f)
    //        : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);

    //    ImGui::PushStyleColor(ImGuiCol_Text, col);
    //    std::string label = std::format("[{:.1f}m] {}##{:08X}",
    //        e.dist, e.shortName, e.handle);
    //    if (ImGui::Selectable(label.c_str(), isSelected))
    //        m_SelectedHandle = e.handle;
    //    ImGui::PopStyleColor();
    //}
}

// Object detail, main verification panel.
void PhysicsDebugTab::DrawObjectDetail(uint32_t handle)
{
    const LiveObject* obj = g_pState->Domain->ObjectTable->GetObject(handle);
    if (!obj)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Object not in ObjectTable.");
        return;
    }

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f),
        "Handle: 0x%08X", handle);
    ImGui::TextWrapped("Tag: %s", obj->TagName.c_str());
    ImGui::TextDisabled("Class: %s", obj->Class.c_str());
    ImGui::Separator();

    DrawLiveObjectSection(handle);
    ImGui::Spacing();
    DrawOrientationVerifier(handle);
    ImGui::Spacing();
    DrawPhysicsModelSection(handle);
    ImGui::Spacing();
    DrawLivePhysicsSection(handle);
}

// LiveObject raw data.
void PhysicsDebugTab::DrawLiveObjectSection(uint32_t handle)
{
    const LiveObject* obj = g_pState->Domain->ObjectTable->GetObject(handle);
    if (!obj) return;

    if (!ImGui::CollapsingHeader("LiveObject (raw)", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    ImGui::Text("Position : (%.4f, %.4f, %.4f)",
        obj->Position[0], obj->Position[1], obj->Position[2]);

    float flen = sqrtf(
        obj->Forward[0] * obj->Forward[0] +
        obj->Forward[1] * obj->Forward[1] +
        obj->Forward[2] * obj->Forward[2]);

    ImGui::Text("Forward  : (%.4f, %.4f, %.4f)  |len=%.4f|",
        obj->Forward[0], obj->Forward[1], obj->Forward[2], flen);

    ImGui::Text("Up       : (%.4f, %.4f, %.4f)",
        obj->Up[0], obj->Up[1], obj->Up[2]);

    ImGui::Text("LinVel   : (%.3f, %.3f, %.3f)",
        obj->LinearVelocity[0], obj->LinearVelocity[1], obj->LinearVelocity[2]);
}

// Orientation verifier, compare facing with cardinal.
void PhysicsDebugTab::DrawOrientationVerifier(uint32_t handle)
{
    const LiveObject* obj = g_pState->Domain->ObjectTable->GetObject(handle);
    if (!obj) return;

    if (!ImGui::CollapsingHeader("Orientation Verifier", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    ImGui::TextDisabled("If object faces a known direction in-game, verify here.");

    float fx = obj->Forward[0], fy = obj->Forward[1];
    float angle = atan2f(fy, fx) * (180.0f / 3.14159265f);
    if (angle < 0) angle += 360.0f;

    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f),
        "Facing : %s  (%.1f deg from East)",
        ForwardToCardinal(fx, fy), angle);

    // Z tilt.
    float fz = obj->Forward[2];
    float tilt = asinf(std::clamp(fz, -1.0f, 1.0f)) * (180.0f / 3.14159265f);
    ImGui::Text("Vertical tilt : %.1f deg  (0=horizontal, +90=straight up)",
        tilt);

    // Sanity.
    float flen = sqrtf(fx * fx + fy * fy + obj->Forward[2] * obj->Forward[2]);
    bool ok = (flen > 0.98f && flen < 1.02f);
    ImGui::TextColored(ok ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
        ok ? "Forward normalized [OK]" : "Forward NOT normalized [ERROR]");
}

// Static phmo data.
void PhysicsDebugTab::DrawPhysicsModelSection(uint32_t handle)
{
    const LiveObject* obj = g_pState->Domain->ObjectTable->GetObject(handle);
    if (!obj) return;

    //const PhmoObject* pm =
    //    g_pState->Domain->MapPhmo->GetPhmo(obj->TagName);

    //if (!ImGui::CollapsingHeader("PhysicsModelData (static, from .map)"))
    //    return;

    //if (!pm)
    //{
    //    ImGui::TextDisabled("No PhysicsModelData for this tag.");
    //    return;
    //}

    //ImGui::TextDisabled("%zu RBs | %zu Spheres | %zu Pills | %zu Boxes | "
    //    "%zu Triangles | %zu MultiSpheres | %zu Polyhedra",
    //    pm->RigidBodies.size(), pm->Spheres.size(), pm->Pills.size(),
    //    pm->Boxes.size(), pm->Triangles.size(), pm->MultiSpheres.size(),
    //    pm->Polyhedra.size());

    //if (ImGui::BeginTable("RBTable", 6,
    //    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
    //    ImGuiTableFlags_ScrollY, ImVec2(0, 150)))
    //{
    //    ImGui::TableSetupColumn("RB#");
    //    ImGui::TableSetupColumn("Node");
    //    ImGui::TableSetupColumn("ShapeType");
    //    ImGui::TableSetupColumn("ShapeIdx");
    //    ImGui::TableSetupColumn("SphOffset");
    //    ImGui::TableSetupColumn("SphRadius");
    //    ImGui::TableHeadersRow();

    //    static const char* shapeNames[] = {
    //        "Sphere","Pill","Box","Triangle","Polyhedron",
    //        "MultiSph","Phantom","Unk7","Unk8","Unk9",
    //        "UnkA","UnkB","UnkC","UnkD","List","MOPP"
    //    };

        //for (size_t i = 0; i < pm->RigidBodies.size(); ++i)
        //{
        //    const auto& rb = pm->RigidBodies[i];

        //    ImGui::TableNextRow();
        //    ImGui::TableSetColumnIndex(0); ImGui::Text("%zu", i);
        //    ImGui::TableSetColumnIndex(1); ImGui::Text("%d", rb.NodeIndex);
        //    ImGui::TableSetColumnIndex(2);

        //    int st = (int)rb.ShapeType;
        //    ImGui::Text("%s(%d)", (st >= 0 && st <= 15) ? shapeNames[st] : "?", st);
        //    ImGui::TableSetColumnIndex(3); ImGui::Text("%d", rb.ShapeIndex);
        //    ImGui::TableSetColumnIndex(4);

        //    ImGui::Text("(%.3f,%.3f,%.3f)", rb.BoundingSphereOffset[0],
        //        rb.BoundingSphereOffset[1], rb.BoundingSphereOffset[2]);

        //    ImGui::TableSetColumnIndex(5);
        //    ImGui::Text("%.3f m", rb.BoundingSphereRadius);
        //}
    //    ImGui::EndTable();
    //}

    // Pills detail.
    //if (!pm->Pills.empty())
    //{
    //    ImGui::Spacing();
    //    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
    //        "Pills (%zu) — model-space:", pm->Pills.size());

    //    if (ImGui::BeginTable("PillTable", 4,
    //        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
    //        ImGuiTableFlags_ScrollY, ImVec2(0, 100)))
    //    {
    //        ImGui::TableSetupColumn("Pill#");
    //        ImGui::TableSetupColumn("Radius");
    //        ImGui::TableSetupColumn("Bottom");
    //        ImGui::TableSetupColumn("Top");
    //        ImGui::TableHeadersRow();

    //        for (size_t i = 0; i < pm->Pills.size(); ++i)
    //        {
    //            const auto& p = pm->Pills[i];
    //            ImGui::TableNextRow();
    //            ImGui::TableSetColumnIndex(0); ImGui::Text("%zu", i);
    //            ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f m", p.Radius);
    //            ImGui::TableSetColumnIndex(2);
    //            ImGui::Text("(%.3f,%.3f,%.3f)", p.Bottom[0], p.Bottom[1], p.Bottom[2]);
    //            ImGui::TableSetColumnIndex(3);
    //            ImGui::Text("(%.3f,%.3f,%.3f)", p.Top[0], p.Top[1], p.Top[2]);
    //        }
    //        ImGui::EndTable();
    //    }

    //    // Verify pill height: Top.Z - Bottom.Z should approximate character height.
    //    if (!pm->Pills.empty())
    //    {
    //        float maxHeight = 0.0f;
    //        for (const auto& p : pm->Pills)
    //        {
    //            maxHeight = (std::max)(maxHeight, p.Top[2] - p.Bottom[2]);
    //        }

    //        ImGui::TextDisabled("Max pill Z span: %.3f m  "
    //            "(Spartan ~1.8m, Elite ~2.1m, Grunt ~1.2m)", maxHeight);
    //    }
    //}

    //// Boxes detail.
    //if (!pm->Boxes.empty())
    //{
    //    ImGui::Spacing();
    //    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
    //        "Boxes (%zu) — model-space:", pm->Boxes.size());

    //    for (size_t i = 0; i < pm->Boxes.size(); ++i)
    //    {
    //        const auto& b = pm->Boxes[i];
    //        ImGui::Text("  Box[%zu] HalfExtents=(%.3f,%.3f,%.3f)  "
    //            "Size=(%.3f x %.3f x %.3f m)",
    //            i,
    //            b.HalfExtents[0], b.HalfExtents[1], b.HalfExtents[2],
    //            b.HalfExtents[0] * 2.0f, b.HalfExtents[1] * 2.0f, b.HalfExtents[2] * 2.0f);
    //        ImGui::Text("         Trans=(%.3f,%.3f,%.3f)",
    //            b.Translation[0], b.Translation[1], b.Translation[2]);
    //    }
    //}

    //// Polyhedra detail.
    //if (!pm->Polyhedra.empty())
    //{
    //    ImGui::Spacing();
    //    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
    //        "Polyhedra (%zu) — AABB in model-space:", pm->Polyhedra.size());

    //    for (size_t i = 0; i < pm->Polyhedra.size() && i < 8; ++i)
    //    {
    //        const auto& p = pm->Polyhedra[i];
    //        ImGui::Text("  Poly[%zu] Center=(%.3f,%.3f,%.3f)  "
    //            "Size=(%.3f x %.3f x %.3f m)",
    //            i,
    //            p.AABBCenter[0], p.AABBCenter[1], p.AABBCenter[2],
    //            p.AABBHalfExtents[0] * 2.0f,
    //            p.AABBHalfExtents[1] * 2.0f,
    //            p.AABBHalfExtents[2] * 2.0f);
    //    }

    //    if (pm->Polyhedra.size() > 8)
    //        ImGui::TextDisabled("  ... and %zu more.", pm->Polyhedra.size() - 8);
    //}
}

// Live physics snapshot.   
void PhysicsDebugTab::DrawLivePhysicsSection(uint32_t handle)
{
    if (!ImGui::CollapsingHeader("LiveObjectPhysics (updated, world-space)",
        ImGuiTreeNodeFlags_DefaultOpen))
        return;

    //const LivePhmoObject* lop =
    //    g_pState->Domain->Phmo->GetPhmos(handle);

    //if (!lop)
    //{
    //    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
    //        "No LiveObjectPhysics for this handle.");
    //    return;
    //}

    //// Bounding sphere
    //ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Bounding Sphere (union)");
    //ImGui::Text("  HasPhysics : %s", lop->HasPhysics ? "true" : "false");
    //ImGui::Text("  Center     : (%.4f, %.4f, %.4f)",
    //    lop->BoundingSphereCenter[0],
    //    lop->BoundingSphereCenter[1],
    //    lop->BoundingSphereCenter[2]);
    //ImGui::Text("  Radius     : %.4f m", lop->BoundingSphereRadius);

    //// Verify: sphere center should be close to object position
    //const LiveObject* obj = g_pState->Domain->ObjectTable->GetObject(handle);
    //if (obj && lop->HasPhysics)
    //{
    //    float dist = Distance3D(lop->BoundingSphereCenter, obj->Position);
    //    bool ok = (dist < lop->BoundingSphereRadius * 2.0f + 0.5f);
    //    ImGui::TextColored(ok ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.4f, 0.2f, 1.0f),
    //        "  Dist from pivot: %.3f m  %s",
    //        dist, ok ? "[OK]" : "[SUSPICIOUS — check rotation]");
    //}

    //ImGui::Spacing();

    //// Union AABB
    //ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "Union AABB (world-space)");
    //ImGui::Text("  HasAABB    : %s", lop->HasAABB ? "true" : "false");
    //ImGui::Text("  Center     : (%.4f, %.4f, %.4f)",
    //    lop->AABBCenter[0], lop->AABBCenter[1], lop->AABBCenter[2]);
    //ImGui::Text("  HalfExtents: (%.4f, %.4f, %.4f)",
    //    lop->AABBHalfExtents[0], lop->AABBHalfExtents[1], lop->AABBHalfExtents[2]);
    //ImGui::Text("  Full Size  : %.3f x %.3f x %.3f m",
    //    lop->AABBHalfExtents[0] * 2.0f,
    //    lop->AABBHalfExtents[1] * 2.0f,
    //    lop->AABBHalfExtents[2] * 2.0f);

    //ImGui::Spacing();

    //// Per-RB snapshots
    //ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f),
    //    "RigidBody Snapshots (%zu):", lop->RigidBodies.size());

    //if (ImGui::BeginTable("RBSnapTable", 5,
    //    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
    //    ImGuiTableFlags_ScrollY, ImVec2(0, 130)))
    //{
    //    ImGui::TableSetupColumn("RB#");
    //    ImGui::TableSetupColumn("Node");
    //    ImGui::TableSetupColumn("SphRadius");
    //    ImGui::TableSetupColumn("AABB Size");
    //    ImGui::TableSetupColumn("AABB Center");
    //    ImGui::TableHeadersRow();

    //    for (size_t i = 0; i < lop->RigidBodies.size(); ++i)
    //    {
    //        const auto& rb = lop->RigidBodies[i];
    //        ImGui::TableNextRow();
    //        ImGui::TableSetColumnIndex(0); ImGui::Text("%zu", i);
    //        ImGui::TableSetColumnIndex(1); ImGui::Text("%d", rb.NodeIndex);
    //        ImGui::TableSetColumnIndex(2); ImGui::Text("%.3fm", rb.SphereRadius);
    //        ImGui::TableSetColumnIndex(3);
    //        if (rb.HasAABB)
    //            ImGui::Text("%.2fx%.2fx%.2f",
    //                rb.AABBHalfExtents[0] * 2.0f,
    //                rb.AABBHalfExtents[1] * 2.0f,
    //                rb.AABBHalfExtents[2] * 2.0f);
    //        else
    //            ImGui::TextDisabled("none");
    //        ImGui::TableSetColumnIndex(4);
    //        if (rb.HasAABB)
    //            ImGui::Text("(%.2f,%.2f,%.2f)",
    //                rb.AABBCenter[0], rb.AABBCenter[1], rb.AABBCenter[2]);
    //    }
    //    ImGui::EndTable();
    //}

    //ImGui::Spacing();
    //ImGui::Separator();
    //ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.f), "Validation Tests");
    //ImGui::Spacing();

    //auto PassFail = [](bool ok, const char* label, const char* detail = nullptr) {
    //    ImGui::TextColored(
    //        ok ? ImVec4(0.2f, 1.f, 0.2f, 1.f) : ImVec4(1.f, 0.3f, 0.3f, 1.f),
    //        ok ? "[PASS]" : "[FAIL]");
    //    ImGui::SameLine();
    //    ImGui::Text("%s", label);
    //    if (detail && !ok)
    //    {
    //        ImGui::SameLine();
    //        ImGui::TextDisabled("  %s", detail);
    //    }
    //};

    //// Test 1: Sphere encloses AABB.
    //if (lop->HasPhysics && lop->HasAABB)
    //{
    //    float maxHE = (std::max)({
    //        lop->AABBHalfExtents[0],
    //        lop->AABBHalfExtents[1],
    //        lop->AABBHalfExtents[2] });
    //    bool ok = lop->BoundingSphereRadius >= maxHE * 0.9f;
    //    char detail[64];
    //    snprintf(detail, sizeof(detail),
    //        "sphere=%.3f  maxHE=%.3f", lop->BoundingSphereRadius, maxHE);
    //    PassFail(ok, "Sphere radius >= max(HalfExtents)", detail);
    //}

    //// Test 2: No zero half extents when HasAABB.
    //if (lop->HasAABB)
    //{
    //    bool ok = lop->AABBHalfExtents[0] > 0.001f
    //        && lop->AABBHalfExtents[1] > 0.001f
    //        && lop->AABBHalfExtents[2] > 0.001f;
    //    char detail[96];
    //    snprintf(detail, sizeof(detail),
    //        "HE=(%.4f, %.4f, %.4f)",
    //        lop->AABBHalfExtents[0],
    //        lop->AABBHalfExtents[1],
    //        lop->AABBHalfExtents[2]);
    //    PassFail(ok, "No zero half extents", detail);
    //}

    //// Test 3: AABB center near object position.
    //if (obj && lop->HasAABB)
    //{
    //    float dist = Distance3D(lop->AABBCenter, obj->Position);
    //    float maxHE = (std::max)({
    //        lop->AABBHalfExtents[0],
    //        lop->AABBHalfExtents[1],
    //        lop->AABBHalfExtents[2] });
    //    bool ok = dist < maxHE * 2.0f + 0.5f;
    //    char detail[64];
    //    snprintf(detail, sizeof(detail),
    //        "dist=%.3f  tolerance=%.3f", dist, maxHE * 2.0f + 0.5f);
    //    PassFail(ok, "AABB center near object position", detail);
    //}

    //// Test 4: Volume stable (anti-rotation sanity).
    //// We track min/max volume seen across frames for the selected handle.
    //{
    //    static uint32_t  s_TrackedHandle = 0xFFFFFFFF;
    //    static float     s_MinVol = FLT_MAX;
    //    static float     s_MaxVol = 0.f;
    //    static int       s_Frames = 0;

    //    if (handle != s_TrackedHandle)
    //    {
    //        s_TrackedHandle = handle;
    //        s_MinVol = FLT_MAX;
    //        s_MaxVol = 0.f;
    //        s_Frames = 0;
    //    }

    //    if (lop->HasAABB)
    //    {
    //        float vol = lop->AABBHalfExtents[0]
    //            * lop->AABBHalfExtents[1]
    //            * lop->AABBHalfExtents[2];

    //        if (vol < s_MinVol) s_MinVol = vol;
    //        if (vol > s_MaxVol) s_MaxVol = vol;
    //        ++s_Frames;

    //        float ratio = (s_MinVol > 0.f) ? s_MaxVol / s_MinVol : 1.f;
    //        bool  ok = ratio < 1.15f; // <= 15% variance across frames.
    //        char  detail[80];
    //        snprintf(detail, sizeof(detail),
    //            "min=%.4f  max=%.4f  ratio=%.3f  frames=%d",
    //            s_MinVol, s_MaxVol, ratio, s_Frames);
    //        PassFail(ok, "AABB volume stable across frames (rotate object to test)", detail);

    //        ImGui::SameLine();
    //        if (ImGui::SmallButton("Reset##vol"))
    //        {
    //            s_MinVol = FLT_MAX;
    //            s_MaxVol = 0.f;
    //            s_Frames = 0;
    //        }
    //    }
    //}
}



// Helpers.
std::string PhysicsDebugTab::GetShortName(const std::string& path) const
{
    size_t pos = path.find_last_of("\\/");
    return (pos != std::string::npos) ? path.substr(pos + 1) : path;
}

const char* PhysicsDebugTab::ForwardToCardinal(float fx, float fy) const
{
    // Halo: X=East, Y=North
    // atan2(fy, fx) gives angle from East, CCW
    float angle = atan2f(fy, fx) * (180.0f / 3.14159265f);
    if (angle < 0) angle += 360.0f;

    // 0=E, 90=N, 180=W, 270=S
    if (angle >= 337.5f || angle < 22.5f)  return "E  (East)";
    if (angle < 67.5f)                    return "NE (North-East)";
    if (angle < 112.5f)                    return "N  (North)";
    if (angle < 157.5f)                    return "NW (North-West)";
    if (angle < 202.5f)                    return "W  (West)";
    if (angle < 247.5f)                    return "SW (South-West)";
    if (angle < 292.5f)                    return "S  (South)";
    return                                        "SE (South-East)";
}

float PhysicsDebugTab::Distance3D(const float a[3], const float b[3]) const
{
    float dx = a[0] - b[0];
    float dy = a[1] - b[1];
    float dz = a[2] - b[2];
    return sqrtf(dx * dx + dy * dy + dz * dz);
}