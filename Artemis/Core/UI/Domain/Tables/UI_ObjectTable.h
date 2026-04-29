#pragma once

#include "Core/Types/Domain/Tables/ObjectTypes.h"
#include <unordered_map>
#include <cstdint>

class UI_ObjectTable
{
public:
	void Draw();

	void Cleanup();

private:
	std::unordered_map<uint32_t, LiveObject> m_CacheObjects;

	void DrawObjectCard(const LiveObject& object);
};