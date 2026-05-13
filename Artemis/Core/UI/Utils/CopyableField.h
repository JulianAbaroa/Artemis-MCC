#pragma once

#include <cstdint>
#include <string>

class CopyableField
{
public:
    void Draw(const char* label, const std::string& value, uint32_t ownerHandle);

private:
    std::string m_AnimateCopyLabel = "";
    float       m_AnimationStartTime = 0.0f;
    float       m_AnimationDuration = 0.6f;
};