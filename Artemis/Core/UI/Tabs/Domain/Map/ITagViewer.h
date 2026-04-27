#pragma once

#include <string>
#include <vector>

class ITagViewer
{
public:
	virtual ~ITagViewer() = default;

	virtual const char* GetTagClass() const = 0;
	virtual const char* GetShortDescription() const = 0;

	virtual std::vector<std::string> GetAvailableTags() = 0;

	virtual void DrawData(const std::string& tagName) = 0;
};