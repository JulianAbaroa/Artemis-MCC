#include "pch.h"

#include "System_Environment.h"

#include "Core/Types/Map/MapMagics.h"

#include "Core/States/MapReader/State_MapReader.h"
#include "Core/States/MapReader/Coll/State_MapColl.h"
#include "Core/States/MapReader/Phmo/State_MapPhmo.h"
#include "Core/States/MapReader/Mode/State_MapMode.h"
#include "Core/States/MapReader/Bipd/State_MapBipd.h"
#include "Core/States/MapReader/Scen/State_MapScen.h"
#include "Core/States/Tables/Object/State_ObjectTable.h"
#include "Core/States/Filtered/Classifier/State_Classifier.h"
#include "Core/States/Domains/Environment/State_Environment.h"

#include "Coll/System_CollBuilder.h"
#include "Phmo/System_PhmoBuilder.h"
#include "Mode/System_ModeBuilder.h"
#include "Core/Systems/Logs/System_Logs.h"

// TODO: Child objects doesnt update their rotation correctly.
// TODO: There are four-vectors of child objects that do not represent 
// correctly the real collisions in-game.

// ----- Static Data -----

void System_Environment::BuildForMap()
{
	const int32_t tagCount = 
		static_cast<int32_t>(m_Deps.State_Map.GetTagsSize());

	int32_t collCount = 0;
	int32_t phmoCount = 0;
	int32_t modeCount = 0;
	int32_t bipdCount = 0;
	int32_t scenCount = 0;

	for (int32_t i = 0; i < tagCount; ++i)
	{
		const Map_TagTableEntry& entry = m_Deps.State_Map.GetTag(i);
		if (entry.TagGroupIndex < 0) continue;

		const uint32_t magic =
			m_Deps.State_Map.GetGroupMagic(entry.TagGroupIndex);

		const std::string tagName = m_Deps.State_Map.GetTagName(i);
		if (tagName.empty()) continue;

		if (magic == MapMagics::k_CollMagic)
		{
			if (!this->BuildColl(tagName)) continue;
			++collCount;
		}
		else if (magic == MapMagics::k_PhmoMagic)
		{
			if (!this->BuildPhmo(tagName)) continue;
			++phmoCount;
		}
		else if (magic == MapMagics::k_ModeMagic)
		{
			if (!this->BuildMode(tagName)) continue;
			++modeCount;
		}
	}

	m_Deps.System_Logs.Log("[EnvironmentSystem] INFO: Environment built."
		" Coll: %d | Phmo: %d | Mode: %d | Bipd: %d | Scen: %d", 
		collCount, phmoCount, modeCount, bipdCount, scenCount);
}

bool System_Environment::BuildColl(const std::string& tagName)
{
	const CollObject* coll = m_Deps.State_MapColl.GetColl(tagName);
	if (!coll)
	{
		m_Deps.System_Logs.Log("[EnvironmentSystem] WARNING: Coll tag found"
			" in table but not loaded: ", tagName);
		return false;
	}

	CollGeometry geometry = 
		m_Deps.System_CollBuilder.Build(*coll);

	m_Deps.State_Environment.AddCollGeometry(tagName, std::move(geometry));
	return true;
}

bool System_Environment::BuildPhmo(const std::string& tagName)
{
	const PhmoObject* phmo = m_Deps.State_MapPhmo.GetPhmo(tagName);
	if (!phmo)
	{
		m_Deps.System_Logs.Log("[EnvironmentSystem] WARNING: Phmo tag found"
			" in table but not loaded: ", tagName);
		return false;
	}

	PhmoGeometry geometry = 
		m_Deps.System_PhmoBuilder.Build(*phmo);

	m_Deps.State_Environment.AddPhmoGeometry(tagName, std::move(geometry));
	return true;
}

bool System_Environment::BuildMode(const std::string& tagName)
{
	const ModeObject* mode = m_Deps.State_MapMode.GetMode(tagName);
	if (!mode)
	{
		m_Deps.System_Logs.Log("[EnvironmentSystem] WARNING: Mode tag found"
			" in table but not loaded: ", tagName);
		return false;
	}

	ModeGeometry geometry = m_Deps.System_ModeBuilder.Build(*mode);

	m_Deps.State_Environment.AddModeGeometry(tagName, std::move(geometry));
	return true;
}

// ----- Dynamic Data -----

void System_Environment::UpdateEnvironment()
{
	const auto& classifieds = m_Deps.State_Classification.GetClassifieds();
	const auto& objects = m_Deps.State_ObjectTable.GetObjectTable();

	this->BuildPhysicsInstances(classifieds, objects);
}

void System_Environment::BuildPhysicsInstances(	
	const std::vector<Classified>& classifieds,
	const std::unordered_map<uint32_t, LiveObject>& objects)
{
	std::vector<ActivePhysicsInstance> instances;

	for (const auto& classified : classifieds)
	{
		switch (classified.Role)
		{	
		case ObjectRole::CrateObstacle:
		case ObjectRole::SceneryObstacle:
		case ObjectRole::DeviceMachine:
		case ObjectRole::Vehicle:
		case ObjectRole::VehiclePart:
		case ObjectRole::Explosive:
		case ObjectRole::Pallet:
		case ObjectRole::PortableShield:
		case ObjectRole::Lift:
		case ObjectRole::Shield:
			break;
		default:
			continue;
		}

		auto objectIt = objects.find(classified.Handle);
		if (objectIt == objects.end()) continue;

		const LiveObject& object = objectIt->second;
		if (object.Address == 0) continue;

		ActivePhysicsInstance instance{};

		instance.Handle = object.Handle;
		instance.TagName = object.TagName;

		instance.Position = object.Position;
		instance.Forward = object.Forward;
		instance.Up = object.Up;

		// Broad-phase: AABB from CollGeometry.
		const CollGeometry* coll =
			m_Deps.State_Environment.GetCollGeometry(object.TagName);
		if (coll)
		{
			instance.CollBoundsMin = {
				coll->BoundsMin.X,
				coll->BoundsMin.Y,
				coll->BoundsMin.Z
			};
			instance.CollBoundsMax = {
				coll->BoundsMax.X,
				coll->BoundsMax.Y,
				coll->BoundsMax.Z
			};
		}

		// Narrow-phase: PhmoGeometry pointer (no copy).
		instance.Phmo = m_Deps.State_Environment.GetPhmoGeometry(object.TagName);

		instance.WorldRigidBodies = this->BuildWorldRigidBodies(instance);

		instances.push_back(std::move(instance));
	}

	m_Deps.State_Environment.SetActivePhysicsInstances(std::move(instances));
}

// Cleanup.
void System_Environment::Cleanup()
{
	m_Deps.State_Environment.Cleanup();
	m_Deps.System_Logs.Log("[EnvironmentSystem] INFO: Cleanup completed.");
}

// --- Helpers ---

std::array<float, 3> System_Environment::Cross(
	const std::array<float, 3>& a,
	const std::array<float, 3>& b)
{
	return {
		a[1] * b[2] - a[2] * b[1],
		a[2] * b[0] - a[0] * b[2],
		a[0] * b[1] - a[1] * b[0]
	};
}

std::array<float, 3> System_Environment::TransformPoint(
	const std::array<float, 3>& pos,
	const std::array<float, 3>& right,
	const std::array<float, 3>& forward,
	const std::array<float, 3>& up,
	float lx, float ly, float lz)
{
	return {
		pos[0] + forward[0] * lx + right[0] * ly + up[0] * lz,
		pos[1] + forward[1] * lx + right[1] * ly + up[1] * lz,
		pos[2] + forward[2] * lx + right[2] * ly + up[2] * lz
	};
}

std::vector<WorldRigidBody> System_Environment::BuildWorldRigidBodies(
	const ActivePhysicsInstance& inst)
{
	if (!inst.Phmo) return {};

	const auto& pos = inst.Position;
	const auto& fwd = inst.Forward;
	const auto& up = inst.Up;
	const auto rgt = Cross(up, fwd); // right = up x forward

	std::vector<WorldRigidBody> result;
	result.reserve(inst.Phmo->RigidBodies.size());

	for (const auto& rb : inst.Phmo->RigidBodies)
	{
		WorldRigidBody wrb;

		// BoundingSphere center in world-space.
		wrb.BoundingSphereCenter = TransformPoint(
			pos, rgt, fwd, up,
			rb.BoundingSphereOffset.X,
			rb.BoundingSphereOffset.Y,
			rb.BoundingSphereOffset.Z);
		wrb.BoundingSphereRadius = rb.BoundingSphereRadius;

		for (const auto& shape : rb.Shapes)
		{
			WorldShape ws;

			switch (shape.Type)
			{
			case PhmoShapeType::Sphere:
			{
				ws.Type = WorldShapeType::Sphere;
				ws.SphereCenter = TransformPoint(
					pos, rgt, fwd, up,
					shape.Sphere.Center.X,
					shape.Sphere.Center.Y,
					shape.Sphere.Center.Z);
				ws.SphereRadius = shape.Sphere.Radius;
				break;
			}
			case PhmoShapeType::Pill:
			{
				ws.Type = WorldShapeType::Pill;
				ws.PillBottom = TransformPoint(
					pos, rgt, fwd, up,
					shape.Pill.Bottom.X,
					shape.Pill.Bottom.Y,
					shape.Pill.Bottom.Z);
				ws.PillTop = TransformPoint(
					pos, rgt, fwd, up,
					shape.Pill.Top.X,
					shape.Pill.Top.Y,
					shape.Pill.Top.Z);
				ws.PillRadius = shape.Pill.Radius;
				break;
			}
			case PhmoShapeType::Box:
			{
				ws.Type = WorldShapeType::Box;

				// The 8 corners of the box in local space,
				// combining ±HalfExtents on each axis.
				// The orientation of the box in local space comes from
				// RotationI/J/K — we need to apply it first,
				// then the object's pose.
				const auto& he = shape.Box.HalfExtents;
				const auto& ri = shape.Box.RotationI; // local X-axis of the box
				const auto& rj = shape.Box.RotationJ; // local Y-axis of the box
				const auto& rk = shape.Box.RotationK; // local Z-axis of the box
				const auto& c = shape.Box.Center;

				// 8 combinations of signs for the 3 axes of the box.
				const float sx[8] = { -1,+1,-1,+1,-1,+1,-1,+1 };
				const float sy[8] = { -1,-1,+1,+1,-1,-1,+1,+1 };
				const float sz[8] = { -1,-1,-1,-1,+1,+1,+1,+1 };

				for (int i = 0; i < 8; ++i)
				{
					// Point in the object's local-space:
					// center + Rotation*±hx + Rotation J*±hy + RotationK*±hz
					float lx = c.X
						+ ri.X * he.X * sx[i]
						+ rj.X * he.Y * sy[i]
						+ rk.X * he.Z * sz[i];
					float ly = c.Y
						+ ri.Y * he.X * sx[i]
						+ rj.Y * he.Y * sy[i]
						+ rk.Y * he.Z * sz[i];
					float lz = c.Z
						+ ri.Z * he.X * sx[i]
						+ rj.Z * he.Y * sy[i]
						+ rk.Z * he.Z * sz[i];

					ws.BoxCorners[i] = TransformPoint(
						pos, rgt, fwd, up, lx, ly, lz);
				}
				break;
			}
			case PhmoShapeType::Polyhedron:
			{
				ws.Type = WorldShapeType::Polyhedron;
				ws.PolyhedronVertices.reserve(
					shape.Polyhedron.Vertices.size());

				for (const auto& v : shape.Polyhedron.Vertices)
				{
					ws.PolyhedronVertices.push_back(TransformPoint(
						pos, rgt, fwd, up, v.X, v.Y, v.Z));
				}
				break;
			}
			case PhmoShapeType::MultiSphere:
			{
				// Each sphere expands as an individual WorldShape
				// to simplify consumption by the AI/UI.
				for (const auto& sp : shape.MultiSphere.Spheres)
				{
					WorldShape wsp;
					wsp.Type = WorldShapeType::Sphere;
					wsp.SphereCenter = TransformPoint(
						pos, rgt, fwd, up,
						sp.Center.X, sp.Center.Y, sp.Center.Z);
					wsp.SphereRadius = sp.Radius;
					wrb.Shapes.push_back(wsp);
				}
				continue;
			}
			default:
				continue;
			}

			wrb.Shapes.push_back(ws);
		}

		result.push_back(std::move(wrb));
	}

	return result;
}