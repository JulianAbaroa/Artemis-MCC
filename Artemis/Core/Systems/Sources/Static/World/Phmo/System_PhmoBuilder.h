#pragma once

#include <cstdint>
#include <vector>

struct Vec3;
struct PhmoVec3;
struct PhmoShape;
struct PhmoObject;
struct PhmoGeometry;
struct Phmo_BoxesEntry;
struct Phmo_PillsEntry;
struct Phmo_SpheresEntry;
struct Phmo_PolyhedraEntry;
struct Phmo_MultiSpheresEntry;

class System_Logs;

struct Sys_PhmoBuilder_Deps
{
    System_Logs& System_Logs;
};

class System_PhmoBuilder
{
public:
    System_PhmoBuilder(Sys_PhmoBuilder_Deps deps) : m_Deps(deps) {}
    ~System_PhmoBuilder() = default;

    PhmoGeometry Build(const PhmoObject& phmo);

private:
    Sys_PhmoBuilder_Deps m_Deps;

    void BuildRigidBodies(const PhmoObject& phmo, PhmoGeometry& out);
    void BuildNodes(const PhmoObject& phmo, PhmoGeometry& out);
    void BuildBounds(PhmoGeometry& out);

    std::vector<PhmoShape> ResolveShapes(
        const PhmoObject& phmo,
        uint16_t shapeType,
        int16_t shapeIndex);

    bool ResolvePrimitive(
        const PhmoObject& phmo,
        uint16_t shapeType,
        int16_t shapeIndex,
        PhmoShape& out);

    PhmoShape MakeSphere(const Phmo_SpheresEntry& src);
    PhmoShape MakePill(const Phmo_PillsEntry& src);
    PhmoShape MakeBox(const Phmo_BoxesEntry& src);
    PhmoShape MakePolyhedron(const PhmoObject& phmo, const Phmo_PolyhedraEntry& src, int16_t polyIndex);
    PhmoShape MakeMultiSphere(const Phmo_MultiSpheresEntry& src);

    PhmoVec3 MakeVec3(const Vec3& v);

    static constexpr uint16_t k_ShapeSphere = 0x0;
    static constexpr uint16_t k_ShapePill = 0x1;
    static constexpr uint16_t k_ShapeBox = 0x2;
    static constexpr uint16_t k_ShapeTriangle = 0x3;
    static constexpr uint16_t k_ShapePolyhedron = 0x4;
    static constexpr uint16_t k_ShapeMultiSphere = 0x5;
    static constexpr uint16_t k_ShapePhantom = 0x6;
    static constexpr uint16_t k_ShapeList = 0xE;
    static constexpr uint16_t k_ShapeMOPP = 0xF;
};