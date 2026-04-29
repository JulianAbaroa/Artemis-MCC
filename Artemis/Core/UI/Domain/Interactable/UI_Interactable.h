#pragma once

#include "Core/Types/Domain/Domains/Interactable/InteractableTypes.h"
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

	std::string GetShortName(const std::string& fullPath) const;

	uint32_t m_SelectedHandle = 0xFFFFFFFF;

	static constexpr float m_CanvasHeight = 320.0f;
};