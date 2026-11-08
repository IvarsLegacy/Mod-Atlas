#pragma once
#include <filesystem>
#include <optional>
#include <string>

namespace tutorials {
struct Note {
    std::string plugin;
    std::string text;
    std::optional<std::string> savedFile;
};

std::filesystem::path UserNotesDirectory();
std::filesystem::path NotePath(const std::filesystem::path& directory, const std::string& plugin);
Note ReadNote(const std::filesystem::path& directory, const std::string& plugin);
void SaveNote(const std::filesystem::path& directory, Note& note, const std::string& text);
}
