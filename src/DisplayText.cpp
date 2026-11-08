#include "DisplayText.h"
#include <utility>

namespace tutorials {
std::string DisplayText(std::string_view text)
{
    static constexpr std::pair<std::string_view, std::string_view> replacements[] = {
        {"‘", "'"}, {"’", "'"}, {"‚", "'"}, {"‛", "'"}, {"′", "'"},
        {"“", "\""}, {"”", "\""}, {"„", "\""}, {"‟", "\""}, {"″", "\""},
        {"‐", "-"}, {"‑", "-"}, {"‒", "-"}, {"–", "-"}, {"—", "-"}, {"―", "-"}, {"−", "-"},
        {"…", "..."}, {"•", "-"}, {"·", "-"}, {"→", "->"}, {"←", "<-"}, {"⇒", "=>"}, {"↔", "<->"},
        {"；", ";"}, {"：", ":"}, {"，", ","}, {"．", "."},
        {"\xc2\xa0", " "}, {"\xe2\x80\xaf", " "}, {"\xe2\x80\x87", " "},
        {"\xe2\x80\x8b", ""}, {"\xef\xbb\xbf", ""}, {"\xc2\xad", ""}
    };
    std::string result;
    result.reserve(text.size());
    for (std::size_t offset = 0; offset < text.size();) {
        if (static_cast<unsigned char>(text[offset]) < 0x80) {
            result += text[offset++];
            continue;
        }
        bool replaced = false;
        for (const auto& [from, to] : replacements) {
            if (text.substr(offset).starts_with(from)) {
                result += to;
                offset += from.size();
                replaced = true;
                break;
            }
        }
        if (!replaced) result += text[offset++];
    }
    return result;
}
}
