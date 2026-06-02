#pragma once

class State_Lifecycle;
class State_Render;

struct UI_Overlay_Dependencies
{
	State_Lifecycle& State_Lifecycle;
	State_Render& State_Render;
};

class UI_Overlay
{
public:
	UI_Overlay(UI_Overlay_Dependencies dependencies) : 
		m_Deps(dependencies) {}
	~UI_Overlay() = default;

	void Draw();

	void ToggleVisible() { m_IsVisible = !m_IsVisible; }

private:
	UI_Overlay_Dependencies m_Deps;
	bool m_IsVisible = false;

	void DrawFPS();
};