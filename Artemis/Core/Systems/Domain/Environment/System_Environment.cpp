#include "pch.h"

// Header.
#include "System_Environment.h"

// Types.
#include "Core/Types/Domain/Map/MapMagics.h"

// --- States ---

#include "Core/States/Domain/Map/State_Map.h"
#include "Core/States/Domain/Map/Coll/State_MapColl.h"
#include "Core/States/Domain/Map/Phmo/State_MapPhmo.h"
#include "Core/States/Domain/Map/Mode/State_MapMode.h"
#include "Core/States/Domain/Map/Scnr/State_MapScnr.h"
#include "Core/States/Domain/Map/Bipd/State_MapBipd.h"
#include "Core/States/Domain/Map/Scen/State_MapScen.h"

#include "Core/States/Domain/Object/State_ObjectTable.h"

#include "Core/States/Domain/Classification/State_Classification.h"

#include "Core/States/Domain/Environment/State_Environment.h"

// --- Systems ---

#include "Coll/System_CollGeometryBuilder.h"
#include "Phmo/System_PhmoGeometryBuilder.h"
#include "Mode/System_ModeGeometryBuilder.h"
#include "Scnr/System_ScnrZoneBuilder.h"
#include "Bipd/System_BipdDataBuilder.h"	
#include "Scen/System_ScenZoneBuilder.h"

#include "Core/Systems/Interface/Debug/System_Debug.h"

// ----- Static Data -----

void System_Environment::BuildForMap()
{
	const int32_t tagCount = 
		static_cast<int32_t>(m_Deps.State_Map.GetTagsSize());

	int32_t collCount = 0;
	int32_t phmoCount = 0;
	int32_t modeCount = 0;
	int32_t scnrCount = 0;
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
		else if (magic == MapMagics::k_ScnrMagic)
		{
			if (!this->BuildScnr(tagName)) continue;
			++scnrCount;
		}
		else if (magic == MapMagics::k_BipdMagic)
		{
			if (!this->BuildBipd(tagName)) continue;
			++bipdCount;
		}
		else if (magic == MapMagics::k_ScenMagic)
		{
			if (!this->BuildScen(tagName)) continue;
			++scenCount;
		}
	}

	m_Deps.System_Debug.Log("[EnvironmentSystem] INFO: Environment built."
		" Coll: %d | Phmo: %d | Mode: %d | Scnr: %d | Bipd: %d | Scen: %d", 
		collCount, phmoCount, modeCount, scnrCount, bipdCount, scenCount);
}

bool System_Environment::BuildColl(const std::string& tagName)
{
	const CollObject* coll = m_Deps.State_MapColl.GetColl(tagName);
	if (!coll)
	{
		m_Deps.System_Debug.Log("[EnvironmentSystem] WARNING: Coll tag found"
			" in table but not loaded: ", tagName);
		return false;
	}

	CollGeometry geometry = 
		m_Deps.System_CollGeometryBuilder.BuildGeometry(*coll);

	m_Deps.State_Environment.AddCollGeometry(tagName, std::move(geometry));
	return true;
}

bool System_Environment::BuildPhmo(const std::string& tagName)
{
	const PhmoObject* phmo = m_Deps.State_MapPhmo.GetPhmo(tagName);
	if (!phmo)
	{
		m_Deps.System_Debug.Log("[EnvironmentSystem] WARNING: Phmo tag found"
			" in table but not loaded: ", tagName);
		return false;
	}

	PhmoGeometry geometry = 
		m_Deps.System_PhmoGeometryBuilder.BuildGeometry(*phmo);

	m_Deps.State_Environment.AddPhmoGeometry(tagName, std::move(geometry));
	return true;
}

bool System_Environment::BuildMode(const std::string& tagName)
{
	const ModeObject* mode = m_Deps.State_MapMode.GetMode(tagName);
	if (!mode)
	{
		m_Deps.System_Debug.Log("[EnvironmentSystem] WARNING: Mode tag found"
			" in table but not loaded: ", tagName);
		return false;
	}

	ModeGeometry geometry = m_Deps.System_ModeGeometryBuilder.BuildGeometry(*mode);

	m_Deps.State_Environment.AddModeGeometry(tagName, std::move(geometry));
	return true;
}

bool System_Environment::BuildScnr(const std::string& tagName)
{
	const ScnrObject* scnr = m_Deps.State_MapScnr.GetScnr(tagName);
	if (!scnr)
	{
		m_Deps.System_Debug.Log("[EnvironmentSystem] WARNING: Scnr tag found"
			" in table but not loaded: ", tagName);
		return false;
	}

	ScnrMapZones zones = m_Deps.System_ScnrZoneBuilder.BuildZone(*scnr);

	m_Deps.State_Environment.SetMapZones(std::move(zones));
	return true;
}

bool System_Environment::BuildBipd(const std::string& tagName)
{
	const BipdObject* bipd = m_Deps.State_MapBipd.GetBipd(tagName);
	if (!bipd)
	{
		m_Deps.System_Debug.Log("[EnvironmentSystem] WARNING: Bipd tag found"
			" in table but not loaded: ", tagName);
		return false;
	}

	BipdPhysicsData data = m_Deps.System_BipdDataBuilder.BuildData(*bipd);

	m_Deps.State_Environment.AddBipdData(tagName, std::move(data));
	return true;
}

bool System_Environment::BuildScen(const std::string& tagName)
{
	const ScenObject* scen = m_Deps.State_MapScen.GetScen(tagName);
	if (!scen)
	{
		m_Deps.System_Debug.Log("[EnvironmentSystem] WARNING: Scen tag found"
			" in table but not loaded: ", tagName);
		return false;
	}

	if (!m_Deps.System_ScenZoneBuilder.IsMpZone(*scen)) return false;

	SceneryZoneData data = m_Deps.System_ScenZoneBuilder.BuildData(*scen);
	m_Deps.State_Environment.AddScenData(tagName, std::move(data));
	return true;
}

// ----- Dynamic Data -----

void System_Environment::UpdateEnvironment()
{
	const auto& classifieds = m_Deps.State_Classification.GetObjects();
	const auto& objects = m_Deps.State_ObjectTable.GetObjectTable();

	this->BuildPhysicsInstances(classifieds, objects);
}

void System_Environment::BuildPhysicsInstances(	
	const std::vector<ClassifiedObject>& classifieds,
	const std::unordered_map<uint32_t, LiveObject>& objects)
{
	std::vector<ActivePhysicsInstance> instances;

	for (const auto& classified : classifieds)
	{
		switch (classified.Role)
		{	
		case ObjectRole::CrateObstacle:
		case ObjectRole::SceneryObstacle:
		case ObjectRole::Vehicle:
		case ObjectRole::Explosive:
		case ObjectRole::Pallet:
		case ObjectRole::PortableShield:
		case ObjectRole::DeviceMachine:
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

		instance.WorldRigidBodies = BuildWorldRigidBodies(instance);

		instances.push_back(std::move(instance));
	}

	m_Deps.State_Environment.SetActivePhysicsInstances(std::move(instances));
}

// Cleanup.
void System_Environment::Cleanup()
{
	m_Deps.State_Environment.Cleanup();
	m_Deps.System_Debug.Log("[EnvironmentSystem] INFO: Cleanup completed.");
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
	// Probar: lx → forward, ly → right, lz → up
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

		// BoundingSphere center en world-space.
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

				// Los 8 corners del box en local-space,
				// combinando ±HalfExtents en cada eje.
				// La orientación del box en local-space viene de
				// RotationI/J/K — necesitamos aplicarla primero,
				// luego la pose del objeto.
				const auto& he = shape.Box.HalfExtents;
				const auto& ri = shape.Box.RotationI; // eje X local del box
				const auto& rj = shape.Box.RotationJ; // eje Y local del box
				const auto& rk = shape.Box.RotationK; // eje Z local del box
				const auto& c = shape.Box.Center;

				// 8 combinaciones de signos para los 3 ejes del box.
				const float sx[8] = { -1,+1,-1,+1,-1,+1,-1,+1 };
				const float sy[8] = { -1,-1,+1,+1,-1,-1,+1,+1 };
				const float sz[8] = { -1,-1,-1,-1,+1,+1,+1,+1 };

				for (int i = 0; i < 8; ++i)
				{
					// Punto en local-space del objeto:
					// center + RotationI*±hx + RotationJ*±hy + RotationK*±hz
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
				// Cada esfera se expande como WorldShape individual
				// para simplificar el consumo por la AI/UI.
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