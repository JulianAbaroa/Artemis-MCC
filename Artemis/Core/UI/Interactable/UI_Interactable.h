#pragma once

#include "Core/Types/Interactable/InteractableTypes.h"
#include "Core/Types/Interaction/LiveInteraction.h"
#include "Core/Types/Object/LiveObject.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

class State_ObjectTable;
class State_InteractionTable;
class State_Interactable;

struct UI_Interactable_Dependencies
{
	State_ObjectTable& State_ObjectTable;
	State_InteractionTable& State_InteractionTable;
	State_Interactable& State_Interactable;
};

class UI_Interactable
{
public:
	UI_Interactable(UI_Interactable_Dependencies dependencies) :
		m_Deps(dependencies) {}
	~UI_Interactable() = default;

	void Draw();
	void Cleanup();

private:
	UI_Interactable_Dependencies m_Deps;

	void DrawEngineInteractionPanel(const LiveInteraction& interaction);
	void DrawInteractableList(const std::vector<AIInteractable>& interactables);
	void DrawSelectedDetails(const AIInteractable& interactable);

	const char* InteractionDetailToString(InteractionType type, InteractionDetail detail);
	std::string GetShortName(const std::string& fullPath) const;
	std::string GetTagName(uint32_t handle) const;

	uint32_t m_SelectedHandle = 0xFFFFFFFF;

	static constexpr float m_CanvasHeight = 320.0f;
};