#pragma once
#include <string>
#include <string_view>

namespace tutorials {
// Font-friendly punctuation for display only; never use for IDs or note storage.
std::string DisplayText(std::string_view text);
}
