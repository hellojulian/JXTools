#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct JX8PPatch {
    uint8_t data[32];

    static bool IsValidSysex(const std::vector<uint8_t>& syx, size_t offset);
    static std::vector<JX8PPatch> ExtractFromSysex(const std::vector<uint8_t>& syx);

    std::string DebugString() const;
};
