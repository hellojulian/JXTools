#include "JX8PPatch.h"
#include <sstream>
#include <iomanip>

bool JX8PPatch::IsValidSysex(const std::vector<uint8_t>& syx, size_t offset) {
    if (offset + 32 > syx.size()) return false;
    // Check Roland ID: F0 41 ...
    return syx[offset] == 0xF0 && syx[offset + 1] == 0x41;
}

std::vector<JX8PPatch> JX8PPatch::ExtractFromSysex(const std::vector<uint8_t>& syx) {
    std::vector<JX8PPatch> patches;
    size_t pos = 0;
    while (pos < syx.size()) {
        if (IsValidSysex(syx, pos)) {
            JX8PPatch patch;
            std::copy(syx.begin() + pos + 6, syx.begin() + pos + 6 + 32, patch.data);
            patches.push_back(patch);
            pos += 6 + 32 + 1; // header + data + F7
        } else {
            ++pos;
        }
    }
    return patches;
}

std::string JX8PPatch::DebugString() const {
    std::ostringstream oss;
    oss << "JX8P Patch Data:\n";
    for (int i = 0; i < 32; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << int(data[i]) << " ";
    }
    return oss.str();
}
