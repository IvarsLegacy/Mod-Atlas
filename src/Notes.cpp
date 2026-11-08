#include <Windows.h>
#include <wincrypt.h>
#include <ShlObj.h>
#include <array>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include "Library.h"
#include "Notes.h"

namespace tutorials {
namespace {
struct Handle {
    HANDLE value = INVALID_HANDLE_VALUE;
    ~Handle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
};

std::optional<std::string> Contents(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path)) return std::nullopt;
    if (std::filesystem::file_size(path) > 512 * 1024)
        throw std::runtime_error("The notes file is too large.");
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("The notes file could not be read.");
    return std::string(std::istreambuf_iterator<char>(input), {});
}

void CheckText(const std::string& text)
{
    if (text.size() > 65535 || text.find('\0') != std::string::npos)
        throw std::runtime_error("Notes must be valid text of at most 65535 UTF-8 bytes.");
    Fold(text);
}
}

std::filesystem::path UserNotesDirectory()
{
    PWSTR folder = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &folder)))
        throw std::runtime_error("The Windows user data folder is unavailable.");
    const auto directory = std::filesystem::path(folder) / L"SkyrimTutorials" / L"UserNotes";
    CoTaskMemFree(folder);
    return directory;
}

std::filesystem::path NotePath(const std::filesystem::path& directory, const std::string& plugin)
{
    const auto key = Fold(plugin);
    if (key.empty()) throw std::runtime_error("A stable plugin key is required for notes.");
    std::array<BYTE, 32> hash{};
    DWORD size = static_cast<DWORD>(hash.size());
    if (!CryptHashCertificate2(L"SHA256", 0, nullptr, reinterpret_cast<const BYTE*>(key.data()),
                               static_cast<DWORD>(key.size()), hash.data(), &size))
        throw std::runtime_error("The notes key could not be generated.");
    constexpr char digits[] = "0123456789abcdef";
    std::string name;
    for (const BYTE byte : hash) {
        name += digits[byte >> 4];
        name += digits[byte & 15];
    }
    return directory / (name + ".json");
}

Note ReadNote(const std::filesystem::path& directory, const std::string& plugin)
{
    Note note{ Fold(plugin), {}, Contents(NotePath(directory, plugin)) };
    if (note.savedFile) {
        const auto json = nlohmann::json::parse(*note.savedFile);
        if (json.at("schema_version") != 1 || Fold(json.at("plugin").get<std::string>()) != note.plugin)
            throw std::runtime_error("The saved notes belong to a different plugin or schema.");
        note.text = json.at("text").get<std::string>();
        CheckText(note.text);
    }
    return note;
}

void SaveNote(const std::filesystem::path& directory, Note& note, const std::string& text)
{
    CheckText(text);
    std::filesystem::create_directories(directory);
    const auto path = NotePath(directory, note.plugin);
    Handle lock{ CreateFileW((path.wstring() + L".lock").c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                             nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr) };
    if (lock.value == INVALID_HANDLE_VALUE)
        throw std::runtime_error("These notes are being saved by another process. Try again.");
    if (Contents(path) != note.savedFile)
        throw std::runtime_error("The notes file changed after it was opened. Reopen it before saving.");
    const auto data = nlohmann::json{ {"schema_version", 1}, {"plugin", note.plugin}, {"text", text} }.dump(2) + "\n";
    GUID id{};
    if (FAILED(CoCreateGuid(&id))) throw std::runtime_error("A temporary save name could not be generated.");
    std::array<wchar_t, 40> unique{};
    StringFromGUID2(id, unique.data(), static_cast<int>(unique.size()));
    const auto temporary = directory / (std::wstring(unique.data()) + L".tmp");
    try {
        {
            Handle file{ CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                     FILE_ATTRIBUTE_NORMAL, nullptr) };
            if (file.value == INVALID_HANDLE_VALUE) throw std::runtime_error("A temporary notes file could not be created.");
            DWORD written = 0;
            if (!WriteFile(file.value, data.data(), static_cast<DWORD>(data.size()), &written, nullptr) ||
                written != data.size() || !FlushFileBuffers(file.value))
                throw std::runtime_error("Notes could not be written. The previous saved version was preserved.");
        }
        if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Notes could not be committed. The previous saved version was preserved.");
    } catch (...) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        throw;
    }
    note.text = text;
    note.savedFile = data;
}
}
