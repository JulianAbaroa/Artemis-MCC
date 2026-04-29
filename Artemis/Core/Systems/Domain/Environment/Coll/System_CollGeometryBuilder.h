#pragma once

#include "Core/Types/Domain/Domains/Environment/CollGeometry.h"
#include "Generated/Coll/CollObject.h"

class System_CollGeometryBuilder
{
public:
	// Takes a fully-deserialized CollObject and returns a CollGeometry.
    // CollObject can be discarded afterwards.
	CollGeometry BuildGeometry(const CollObject& coll);

private:
	void BuildPathfindingSpheres(const CollObject& coll, CollGeometry& out);
	void BuildNodes(const CollObject& coll, CollGeometry& out);
	void BuildBounds(CollGeometry& out);

	CollVec3 MakeVec3(const Vec3& v) { return { v.X, v.Y, v.Z }; }
};