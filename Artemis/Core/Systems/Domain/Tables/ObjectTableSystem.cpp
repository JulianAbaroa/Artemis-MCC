#include "pch.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/Hooks/Memory/CoreMemoryHook.h"
#include "Core/Hooks/Memory/ObjectTableHook.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Tables/ObjectTableState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Map/MapSystem.h"
#include "Core/Systems/Domain/Tables/ObjectTableSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"

void ObjectTableSystem::OnObjectCreated(uint32_t handle, uint32_t datumIndex)
{
	auto info = g_pSystem->Domain->Map->ResolveHandle(datumIndex);
	if (!info.IsValid)
	{
		g_pSystem->Debug->Log("[ObjectTableSystem] WARNING: Failed to resolve DatumIndex 0x%X for handle 0x%X.", datumIndex, handle);
		return;
	}

	LiveObject object;
	object.Handle = handle;
	object.DatumIndex = datumIndex;
	object.Class = info.ClassName;
	object.TagName = info.TagName;

	if (object.Class == "" || object.TagName == "")
	{
		g_pSystem->Debug->Log("[ObjectTableSystem] WARNING: Invalid object created, skipping.");
		return;
	}

	// TODO: Add more classes.
	object.SpecificObject = [&]() -> SpecificObject {
		if (object.Class == "weap") return WeaponObject{};
		if (object.Class == "vehi") return VehicleObject{};
		if (object.Class == "bipd") return BipedObject{};
		if (object.Class == "bloc") return BlockObject{};
		return std::monostate{};
	}();

	g_pState->Domain->ObjectTable->AddObject(handle, object);
}

void ObjectTableSystem::OnObjectDestroyed(uint32_t handle)
{
	auto deletedObject = g_pState->Domain->ObjectTable->RemoveObject(handle);

	if (!deletedObject.has_value())
	{
		g_pSystem->Debug->Log("[ObjectTableSystem] WARNING:"
			" OnObjectDestroyed called for unknown handle 0x%X.", handle);
	}
}

// TODO: There's a possiblity that the ObjectTable contains
// objects that weren't created by CreateObject.
// Is this important to us?
void ObjectTableSystem::UpdateObjectTable()
{
	uintptr_t tableBase = g_pState->Domain->ObjectTable->GetObjectTableBase();
	if (tableBase == 0) return;

	g_pState->Domain->ObjectTable->UpdateObjects(
		[&](uint32_t handle, LiveObject& object)
	{
		// 1. Index = Handle & 0xFFFF
		uint32_t index = handle & 0xFFFF;
	
		// 2. Offset = Index * 0x18
		uintptr_t offset = (uintptr_t)index * 0x18;
	
		// 3. Address = ObjectTableBase + Offset
		uintptr_t entryAddr = tableBase + offset;
	
		if (entryAddr != 0)
		{
			uint32_t tableHandle = *(uint32_t*)(entryAddr);
			TagType tagType = *(TagType*)(entryAddr + 0x04);
			uintptr_t entityPtr = *(uintptr_t*)(entryAddr + 0x10);

			// The ObjectTable only uses the 'Salt' of the handle.
			uint16_t tableSalt = (uint16_t)(tableHandle & 0xFFFF);
			uint16_t expectedSalt = (uint16_t)(handle >> 16);
	
			if (entityPtr != 0 && tableSalt == expectedSalt)
			{
				if (object.Address == 0 ||
					object.Address != entityPtr)
				{
					object.Address = entityPtr;
				}

				if (object.TagType == TagType::Invalid && 
					tagType != TagType::Invalid)
				{
					object.TagType = tagType;
				}
	
				this->UpdateObjectData(object);
			}
			else
			{
				object.Address = 0;
			}
		}
	});
}

void ObjectTableSystem::FindObjectTableBase()
{
	uintptr_t tableBase = g_pState->Domain->ObjectTable->GetObjectTableBase();
	if (tableBase == 0)
	{
		tableBase = g_pHook->Memory->ObjectTable->GetObjectTable();
		if (!tableBase)
		{
			g_pSystem->Debug->Log("[ObjectTableSystem] ERROR:"
				" ObjectTableBase invalid.");

			return;
		}

		g_pSystem->Debug->Log("[ObjectTableSystem] INFO: ObjectTable: 0x%llX", tableBase);
		g_pState->Domain->ObjectTable->SetObjectTableBase(tableBase);
	}
}

void ObjectTableSystem::Cleanup()
{
	g_pState->Domain->ObjectTable->Cleanup();
	g_pSystem->Debug->Log("[ObjectTableSystem] INFO: Cleanup completed.");
}


void ObjectTableSystem::UpdateObjectData(LiveObject& object)
{
	if (object.Address == 0) return;

	const size_t bytesToRead = sizeof(ObjectHeader);
	ObjectHeader header{};

	std::memcpy(&header, reinterpret_cast<void*>(object.Address), bytesToRead);

	object.NextSiblingHandle = header.NextSiblingHandle;
	object.ChildHandle = header.ChildHandle;
	object.ParentHandle = header.ParentHandle;

	object.Position[0] = header.PhysicsPosition[0];
	object.Position[1] = header.PhysicsPosition[1];
	object.Position[2] = header.PhysicsPosition[2];

	object.Forward[0] = header.Forward[0];
	object.Forward[1] = header.Forward[1];
	object.Forward[2] = header.Forward[2];

	object.Up[0] = header.Up[0];
	object.Up[1] = header.Up[1];
	object.Up[2] = header.Up[2];

	object.LinearVelocity[0] = header.LinearVelocity[0];
	object.LinearVelocity[1] = header.LinearVelocity[1];
	object.LinearVelocity[2] = header.LinearVelocity[2];

	object.AngularVelocity[0] = header.AngularVelocity[0];
	object.AngularVelocity[1] = header.AngularVelocity[1];
	object.AngularVelocity[2] = header.AngularVelocity[2];
}