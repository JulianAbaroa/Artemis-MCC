#pragma once

#include <cstdint>
#include <vector>

// Forward declarations.
struct Phmo_MultiSpheresEntry;
struct Phmo_PolyhedraEntry;
struct Phmo_SpheresEntry;
struct Phmo_PillsEntry;
struct Phmo_BoxesEntry;
struct PhmoGeometry;
struct PhmoObject;
struct PhmoShape;
struct PhmoVec3;
struct Vec3;

class System_PhmoGeometryBuilder
{
public:
    PhmoGeometry BuildGeometry(const PhmoObject& phmo);

private:
    void BuildRigidBodies(const PhmoObject& phmo, PhmoGeometry& out);
    void BuildNodes(const PhmoObject& phmo, PhmoGeometry& out);
    void BuildBounds(PhmoGeometry& out);

    // Resolves the shapes of a RigidBody.
    // If ShapeType is a List, expands via ListShapes->primitives.
    // If it's a direct primitive, returns a single ShapeShape.
    std::vector<PhmoShape> ResolveShapes(
        const PhmoObject& phmo,
        uint16_t shapeType,
        int16_t shapeIndex);

    // Resolves a single primitive given type and index.
    // Returns false if the index is invalid or the type is not supported.
    bool ResolvePrimitive(
        const PhmoObject& phmo,
        uint16_t shapeType,
        int16_t shapeIndex,
        PhmoShape& out);

    PhmoShape MakeSphere(const Phmo_SpheresEntry& src);
    PhmoShape MakePill(const Phmo_PillsEntry& src);
    PhmoShape MakeBox(const Phmo_BoxesEntry& src);
    PhmoShape MakePolyhedron(const Phmo_PolyhedraEntry& src);
    PhmoShape MakeMultiSphere(const Phmo_MultiSpheresEntry& src);

    PhmoVec3 MakeVec3(const Vec3& v);

    // ShapeType enum values ​​of the tag.
    static constexpr uint16_t k_ShapeSphere = 0x0;
    static constexpr uint16_t k_ShapePill = 0x1;
    static constexpr uint16_t k_ShapeBox = 0x2;
    static constexpr uint16_t k_ShapeTriangle = 0x3;
    static constexpr uint16_t k_ShapePolyhedron = 0x4;
    static constexpr uint16_t k_ShapeMultiSphere = 0x5;
    static constexpr uint16_t k_ShapeList = 0xE;
    static constexpr uint16_t k_ShapeMOPP = 0xF;
};