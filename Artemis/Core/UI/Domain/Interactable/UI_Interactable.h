#pragma once

// Types.
#include "Core/Types/Domain/Interactable/InteractableTypes.h"
#include "Core/Types/Domain/Interaction/LiveInteraction.h"
#include "Core/Types/Domain/Object/LiveObject.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

class UI_Interactable
{
public:
	void Draw();
	void Cleanup();

private:
	void DrawEngineInteractionPanel(const LiveInteraction& interaction);
	void DrawInteractableList(const std::vector<AIInteractable>& interactables);
	void DrawSelectedDetails(const AIInteractable& interactable);

	const char* InteractionDetailToString(InteractionType type, InteractionDetail detail);
	std::string GetShortName(const std::string& fullPath) const;
	std::string GetTagName(uint32_t handle) const;

	uint32_t m_SelectedHandle = 0xFFFFFFFF;

	static constexpr float m_CanvasHeight = 320.0f;
};