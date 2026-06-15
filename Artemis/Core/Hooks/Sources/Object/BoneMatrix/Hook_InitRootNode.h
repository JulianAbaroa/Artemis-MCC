#pragma once

#include <atomic>

class State_BoneOffsets;
class System_ObjectTable;
class System_AOBScanner;
class System_Logs;

struct Hook_InitRootNode_Dependencies
{
	State_BoneOffsets& State_BoneOffsets;
	System_ObjectTable& System_ObjectTable;
	System_AOBScanner& System_AOBScanner;
	System_Logs& System_Logs;
};

class Hook_InitRootNode
{
public:
	Hook_InitRootNode(Hook_InitRootNode_Dependencies dependencies) :
		m_Deps(dependencies) {
	}
	~Hook_InitRootNode() = default;

	void Install();
	void Uninstall();

private:
	static Hook_InitRootNode* s_Instance;
	Hook_InitRootNode_Dependencies m_Deps;

	static void __fastcall HookedInitRootNode(unsigned short* param_1);

	typedef void(__fastcall* NodeTransform_t)(unsigned short* param_1);

	static inline NodeTransform_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};