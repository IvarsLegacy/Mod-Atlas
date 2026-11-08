#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace tutorials {
struct Article {
    std::string id;
    std::string category;
    std::string subject;
    std::string group;
    std::string title;
    std::string text;
    std::string original;
    std::string source;
    std::string search;
    bool overview = false;
};

struct Pack {
    std::string id;
    std::string title;
    std::string version;
    std::string requiresPlugin;
    std::string requiresFile;
    std::string documentedModVersion;
    std::string installedOrigin;
    std::string reviewedOn;
    std::string sourcePluginHash;
    std::vector<Article> articles;
};

struct Library {
    std::vector<Pack> packs;
    std::vector<std::string> errors;
    std::vector<std::string> categories;
    std::size_t count = 0;
};

std::string Fold(const std::string& value);
std::string NoteKey(const Pack& pack);
Library LoadCatalog(const std::filesystem::path& directory);
bool Matches(const Article& article, const std::string& category, const std::string& query);
}
