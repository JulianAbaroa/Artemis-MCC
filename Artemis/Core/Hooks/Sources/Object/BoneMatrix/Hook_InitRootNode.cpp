#include "pch.h"

#include "Hook_InitRootNode.h"

#include "Core/Types/Other/Memory/AOB/Signatures.h"

#include "Core/States/Sources/Tables/Object/BoneMatrix/State_BoneOffsets.h"

#include "Core/Systems/Sources/Tables/Object/System_ObjectTable.h"
#include "Core/Systems/Other/Memory/AOB/System_AOBScanner.h"
#include "Core/Systems/Other/Logs/System_Logs.h"

#include "External/minhook/include/MinHook.h"

void __fastcall Hook_InitRootNode::HookedInitRootNode(unsigned short* param_1)
{
	if (s_Instance && param_1)
	{
		auto p = reinterpret_cast<const uint8_t*>(param_1);

		const uint32_t  handle = *reinterpret_cast<const uint32_t*>(p + 0x00);
		const uintptr_t objBase = *reinterpret_cast<const uintptr_t*>(p + 0x08);
		const uint32_t  nodeCount = *reinterpret_cast<const uint32_t*>(p + 0x10);
		const uintptr_t boneTbl = *reinterpret_cast<const uintptr_t*>(p + 0x18);

		if (objBase != 0 && boneTbl != 0 && boneTbl > objBase)
		{
			const uintptr_t offset = boneTbl - objBase;

			if (offset < 0x100000 && nodeCount > 0 && nodeCount < 1024)
			{
				BonesHeader info;
				info.Offset = offset;
				info.NodeCount = nodeCount;

				const bool isNew =
					s_Instance->m_Deps.State_BoneOffsets.Set(handle, info);

				//if (isNew)
				//{
				//	s_Instance->m_Deps.System_Logs.Log("[InitRootNode] INFO:"
				//		" captured handle=0x%X offset=0x%X nodeCount=%u",
				//		handle, static_cast<unsigned>(info.Offset),
				//		info.NodeCount);
				//}
			}
		}
	}

	m_OriginalFunction(param_1);
}

Hook_InitRootNode* Hook_InitRootNode::s_Instance = nullptr;

void Hook_InitRootNode::Install()
{
	if (m_IsHookInstalled.load()) return;
	s_Instance = this;

	void* functionAddress = (void*)s_Instance->m_Deps.
		System_AOBScanner.FindPattern(Signatures::InitRootNode);

	if (!functionAddress)
	{
		s_Instance->m_Deps.System_Logs.Log("[InitRootNode] ERROR:"
			" Failed to obtain the function address.");
		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(),
		&this->HookedInitRootNode,
		reinterpret_cast<LPVOID*>(&m_OriginalFunction)
	) != MH_OK)
	{
		s_Instance->m_Deps.System_Logs.Log("[InitRootNode] ERROR:"
			" Failed to create the hook.");
		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		s_Instance->m_Deps.System_Logs.Log("[InitRootNode] ERROR:"
			" Failed to enable hook.");
		return;
	}

	m_IsHookInstalled.store(true);
	s_Instance->m_Deps.System_Logs.Log("[InitRootNode] INFO:"
		" Hook installed.");
	return;
}

void Hook_InitRootNode::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	s_Instance->m_Deps.System_Logs.Log("[InitRootNode] INFO:"
		" Hook uninstalled.");

	s_Instance = nullptr;
}