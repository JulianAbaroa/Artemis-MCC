module;

#include "External/minhook/include/MinHook.h"

module Tables.Object.Hook;
import :InitRootNode;

import Tables.Object.Type;
import Platform.Memory.Type;
import Platform.Hook.Common;

namespace
{
	namespace Signature = Platform::Memory::Type::Signature;

	using BonesHeader = Tables::Object::Type::BoneMatrix::BonesHeader;
}

namespace Tables::Object::Hook
{
	auto __fastcall InitRootNodeDetour::HookedInitRootNode(
		unsigned short* param_1) -> void
	{
		if (s_Instance && param_1)
		{
			auto p = reinterpret_cast<const std::uint8_t*>(param_1);

			const std::uint32_t handle = *reinterpret_cast<const std::uint32_t*>(p + 0x00);
			const std::uintptr_t objBase = *reinterpret_cast<const std::uintptr_t*>(p + 0x08);
			const std::uint32_t nodeCount = *reinterpret_cast<const std::uint32_t*>(p + 0x10);
			const std::uintptr_t boneTbl = *reinterpret_cast<const std::uintptr_t*>(p + 0x18);

			if (objBase != 0 && boneTbl != 0 && boneTbl > objBase)
			{
				const std::uintptr_t offset = boneTbl - objBase;

				if (offset < 0x100000 && nodeCount > 0 && nodeCount < 1024)
				{
					BonesHeader info;
					info.Offset = offset;
					info.NodeCount = nodeCount;

					const bool isNew =
						s_Instance->m_BoneOffsetsStore.Set(handle, info);

					//if (isNew)
					//{
					//	s_Instance->m_LogsService.Message("[InitRootNodeDetour] INFO:"
					//		" captured handle=0x{:X} offset=0x{:X} nodeCount={}",
					//		handle, static_cast<unsigned>(info.Offset),
					//		info.NodeCount);
					//}
				}
			}
		}

		m_OriginalFunction(param_1);
	}

	InitRootNodeDetour* InitRootNodeDetour::s_Instance = nullptr;

	void InitRootNodeDetour::Install()
	{
		if (m_IsHookInstalled.load()) return;
		s_Instance = this;

		void* functionAddress = (void*)m_AOBService.FindPattern(Signature::InitRootNode);
		m_FunctionAddress.store(functionAddress);

		if (!Platform::Hook::Common::InstallDetour(functionAddress,
			reinterpret_cast<void*>(&HookedInitRootNode),
			reinterpret_cast<void**>(&m_OriginalFunction),
			"[InitRootNodeDetour]", m_LogsService))
		{
			return;
		}

		m_IsHookInstalled.store(true);
	}

	void InitRootNodeDetour::Uninstall()
	{
		if (!m_IsHookInstalled.load()) return;

		Platform::Hook::Common::UninstallDetour(m_FunctionAddress.load(),
			"[InitRootNodeDetour]", m_LogsService);

		m_IsHookInstalled.store(false);
		s_Instance = nullptr;
	}
}