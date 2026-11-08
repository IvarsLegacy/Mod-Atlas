#pragma once
#include "Library.h"

namespace tutorials {
enum class TextBlockKind { Paragraph, Heading, ListItem };
struct TextBlock { TextBlockKind kind; std::string text; };
Pack ReadTextDocument(const std::filesystem::path& path, std::string text,
                      const std::string& title = {}, const std::string& category = "Personal");
std::vector<TextBlock> FormatDocument(const std::string& text);
}
