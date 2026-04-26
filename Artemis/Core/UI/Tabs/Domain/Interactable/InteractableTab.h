#pragma once

#include "Core/Common/Types/Domain/Interactable/InteractableTypes.h"
//#include "Core/Common/Types/Domain/Map/PhmoTypes.h"
#include <vector>
#include <string>

class InteractableTab
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