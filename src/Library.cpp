#include "Library.h"
#include "DisplayText.h"
#include "Documents.h"
#include <Windows.h>
#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace tutorials {
using Json = nlohmann::json;

std::string Fold(const std::string& value)
{
    if (value.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                                       static_cast<int>(value.size()), nullptr, 0);
    if (!size) throw std::runtime_error("Text is not valid UTF-8");
    std::wstring wide(size, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                        static_cast<int>(value.size()), wide.data(), size);
    CharLowerBuffW(wide.data(), size);
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, wide.data(), size, nullptr, 0, nullptr, nullptr);
    std::string result(bytes, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), size, result.data(), bytes, nullptr, nullptr);
    return result;
}

std::string NoteKey(const Pack& pack)
{
    if (!pack.requiresPlugin.empty()) return Fold(pack.requiresPlugin);
    if (!pack.requiresFile.empty()) {
        auto path = Fold(pack.requiresFile);
        std::replace(path.begin(), path.end(), '\\', '/');
        return "file:" + path;
    }
    return "pack:" + pack.id;
}

static std::string Text(const Json& object, const char* key, bool required = true,
                        std::size_t maximum = 512)
{
    if (!object.contains(key)) {
        if (required) throw std::runtime_error(std::string("Missing field: ") + key);
        return {};
    }
    const auto& value = object.at(key);
    if (!value.is_string()) throw std::runtime_error(std::string("Expected text: ") + key);
    auto result = value.get<std::string>();
    if ((required && result.empty()) || result.size() > maximum ||
        result.find('\0') != std::string::npos)
        throw std::runtime_error(std::string("Invalid text length: ") + key);
    Fold(result);
    return result;
}

static std::string Identifier(const Json& object)
{
    auto id = Text(object, "id", true, 128);
    if (!std::all_of(id.begin(), id.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.';
    })) throw std::runtime_error("IDs may contain only a-z, 0-9, periods, hyphens and underscores");
    return id;
}

static void ValidateOwner(const Pack& pack)
{
    if (!pack.requiresFile.empty()) {
        const std::filesystem::path relative(std::u8string(pack.requiresFile.begin(), pack.requiresFile.end()));
        if (relative.has_root_path() || relative.filename().empty() ||
            std::any_of(relative.begin(), relative.end(), [](const auto& part) { return part == ".." || part == "."; }))
            throw std::runtime_error("requires_file must be a relative file path inside Data");
    }
}

static void Index(Pack& pack)
{
    for (auto& article : pack.articles) {
        std::string visible;
        for (const auto& block : FormatDocument(article.text)) visible += block.text + " ";
        const auto searchable = pack.title + " " + article.subject + " " + article.category + " " + article.group + " " +
                                article.title + " " + article.text + " " + visible + " " + article.original + " " + article.search;
        article.search = Fold(searchable + " " + DisplayText(searchable));
    }
}

static Pack ReadPack(const std::filesystem::path& path)
{
    if (std::filesystem::file_size(path) > 2 * 1024 * 1024)
        throw std::runtime_error("Pack exceeds 2 MiB");
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("File could not be read");
    const std::string raw{std::istreambuf_iterator<char>(input), {}};
    if (Fold(path.extension().string()) != ".json") {
        auto pack = ReadTextDocument(path, raw);
        Index(pack);
        return pack;
    }
    const auto json = Json::parse(raw);
    if (json.is_object() && json.contains("text") && !json.contains("articles")) {
        if (json.contains("schema_version") && (!json["schema_version"].is_number_integer() || json["schema_version"] != 1))
            throw std::runtime_error("schema_version must be 1 when supplied");
        auto pack = ReadTextDocument(path, Text(json, "text", true, 2 * 1024 * 1024), Text(json, "title", false),
                                     json.contains("category") ? Text(json, "category") : "Personal");
        if (json.contains("id")) pack.id = Identifier(json);
        if (json.contains("version")) pack.version = Text(json, "version");
        if (json.contains("requires_plugin")) {
            pack.requiresPlugin = Text(json, "requires_plugin", false);
            if (!pack.requiresPlugin.empty()) pack.requiresFile.clear();
        }
        if (json.contains("requires_file")) pack.requiresFile = Text(json, "requires_file", false);
        if (json.contains("source")) for (auto& a : pack.articles) a.source = Text(json, "source", false, 4096);
        ValidateOwner(pack);
        Index(pack);
        return pack;
    }
    if (!json.is_object() || !json.contains("schema_version") ||
        !json.at("schema_version").is_number_integer() || json.at("schema_version") != 1)
        throw std::runtime_error("schema_version must be 1");
    Pack pack;
    pack.id = Identifier(json);
    pack.title = Text(json, "title");
    pack.version = Text(json, "version");
    pack.requiresPlugin = Text(json, "requires_plugin", false);
    pack.requiresFile = Text(json, "requires_file", false);
    ValidateOwner(pack);
    pack.documentedModVersion = Text(json, "documented_mod_version", false, 128);
    pack.installedOrigin = Text(json, "installed_origin", false, 1024);
    pack.reviewedOn = Text(json, "reviewed_on", false, 32);
    pack.sourcePluginHash = Text(json, "source_plugin_sha256", false, 64);
    if (pack.sourcePluginHash.empty()) pack.sourcePluginHash = Text(json, "source_file_sha256", false, 64);
    if (!pack.sourcePluginHash.empty() && (pack.sourcePluginHash.size() != 64 ||
        !std::all_of(pack.sourcePluginHash.begin(), pack.sourcePluginHash.end(), [](unsigned char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        }))) throw std::runtime_error("Source SHA-256 must be a 64-digit hexadecimal hash");
    const auto& articles = json.at("articles");
    if (!articles.is_array() || articles.empty() || articles.size() > 512)
        throw std::runtime_error("Pack must contain 1 to 512 articles");
    std::set<std::string> ids;
    for (const auto& item : articles) {
        Article article;
        article.id = Identifier(item);
        article.overview = item.value("is_overview", false);
        if (!ids.insert(article.id).second) throw std::runtime_error("Duplicate article ID: " + article.id);
        article.category = Text(item, "category");
        article.subject = Text(item, "subject", false);
        if (article.subject.empty()) article.subject = pack.title;
        article.group = Text(item, "group", false);
        article.title = Text(item, "title");
        article.text = Text(item, "text", true, 128 * 1024);
        article.original = Text(item, "original", false, 128 * 1024);
        article.source = Text(item, "source", false, 4096);
        std::string tags;
        if (item.contains("tags")) {
            if (!item["tags"].is_array() || item["tags"].size() > 64)
                throw std::runtime_error("tags must be a list of at most 64 strings");
            for (const auto& tag : item["tags"]) {
                if (!tag.is_string() || tag.get_ref<const std::string&>().size() > 256)
                    throw std::runtime_error("Invalid search tag");
                tags += " " + tag.get<std::string>();
            }
        }
        article.search = tags;
        pack.articles.push_back(std::move(article));
    }
    Index(pack);
    return pack;
}

Library LoadCatalog(const std::filesystem::path& directory)
{
    Library library;
    std::vector<std::filesystem::path> paths;
    std::error_code error;
    // The standalone framework is valid before any information pack is installed.
    if (!std::filesystem::exists(directory, error) && !error) return library;
    if (!std::filesystem::is_directory(directory, error)) {
        library.errors.push_back("Pack directory is missing or unreadable: " + directory.string());
        return library;
    }
    std::filesystem::directory_iterator iterator(directory, error);
    const std::filesystem::directory_iterator end;
    for (; !error && iterator != end; iterator.increment(error)) {
        if (iterator->is_regular_file(error)) {
            const auto ext = Fold(iterator->path().extension().string());
            if (ext == ".json" || ext == ".txt" || ext == ".md") paths.push_back(iterator->path());
        }
    }
    if (error) library.errors.push_back("Pack directory could not be fully read: " + error.message());
    std::sort(paths.begin(), paths.end());
    std::set<std::string> ids;
    std::set<std::string> categories;
    for (const auto& path : paths) {
        try {
            auto pack = ReadPack(path);
            if (!ids.insert(pack.id).second) throw std::runtime_error("Duplicate pack ID: " + pack.id);
            for (const auto& article : pack.articles) categories.insert(article.category);
            library.count += pack.articles.size();
            library.packs.push_back(std::move(pack));
        } catch (const std::exception& failure) {
            const auto name = path.filename().u8string();
            library.errors.push_back(std::string(name.begin(), name.end()) + ": " + failure.what());
        }
    }
    std::stable_sort(library.packs.begin(), library.packs.end(), [](const Pack& a, const Pack& b) {
        return Fold(a.title) < Fold(b.title);
    });
    categories.insert("Followers");
    library.categories.assign(categories.begin(), categories.end());
    return library;
}

bool Matches(const Article& article, const std::string& category, const std::string& query)
{
    if (!category.empty() && category != article.category) return false;
    std::istringstream words(query);
    for (std::string word; words >> word;) {
        if (article.search.find(word) == std::string::npos) return false;
    }
    return true;
}
}
