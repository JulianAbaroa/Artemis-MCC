#pragma once

#include "Core/UI/Tabs/Domain/Map/ITagViewer.h"
#include "Generated/Phmo/PhmoObject.h"

class PhmoViewer : public ITagViewer
{
public:
	const char* GetTagClass() const override { return "phmo"; }
	const char* GetShortDescription() const override { "physics_model"; }

	std::vector<std::string> GetAvailableTags() override;

	void DrawData(const std::string& tagName) override;

private:
	void DrawPhmoData(const PhmoObject* phmo);
};