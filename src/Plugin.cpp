#include <Windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <set>
#include <string>
#include <vector>
#include "Library.h"
#include "Browser.h"
#include "DisplayText.h"
#include "Documents.h"
#include "Notes.h"
namespace RE { class InputEvent; }
#include "SKSEMenuFramework.h"

// Public SKSE 2.0.20 C ABI. No game-memory addresses or engine hooks are used.
// Layout verified against SKSE/Interfaces.h in the local CommonLibSSE SDK.
namespace skse_abi {
struct PluginInfo { std::uint32_t infoVersion; const char* name; std::uint32_t version; };
struct Interface {
    std::uint32_t skseVersion, runtimeVersion, editorVersion, isEditor;
    void* (*QueryInterface)(std::uint32_t);
    std::uint32_t (*GetPluginHandle)();
    std::uint32_t (*GetReleaseIndex)();
    const void* (*GetPluginInfo)(const char*);
};
struct Message { const char* sender; std::uint32_t type, length; void* data; };
struct Messaging {
    std::uint32_t version;
    bool (*RegisterListener)(std::uint32_t, const char*, void (*)(Message*));
    bool (*Dispatch)(std::uint32_t, std::uint32_t, void*, std::uint32_t, const char*);
    void* (*GetEventDispatcher)(std::uint32_t);
};
static_assert(sizeof(Interface) == 48 && sizeof(Message) == 24 && sizeof(PluginInfo) == 24);
constexpr std::uint32_t runtime_1597 = 0x01050610;
}

namespace {
namespace UI = ImGuiMCP;
std::filesystem::path pluginDirectory;
tutorials::Library library;
std::array<char, 256> query{};
std::string category;
std::vector<tutorials::ModPage> modPages;
tutorials::BrowserSelection browser;
bool scrollToOverview = false;
std::string originalTopic;
bool registered = false;
std::string renderError;
SKSEMenuFramework::Model::WindowInterface* notesWindow = nullptr;
std::optional<tutorials::Note> note;
std::filesystem::path notesDirectory;
std::array<char, 65536> noteBuffer{};
std::string noteTitle, noteMessage;
bool notesChanged = false;
bool focusNotes = false;

void OpenNotes(const tutorials::Pack& pack)
{
    if (!notesWindow) return;
    const auto key = tutorials::NoteKey(pack);
    if (note && note->plugin != key && notesChanged) {
        noteMessage = "Save or discard the current changes before opening another mod's notes.";
    } else if (!note || note->plugin != key) {
        try {
            notesDirectory = tutorials::UserNotesDirectory();
            auto loaded = tutorials::ReadNote(notesDirectory, key);
            noteBuffer.fill(0);
            std::copy(loaded.text.begin(), loaded.text.end(), noteBuffer.begin());
            note = std::move(loaded);
            noteTitle = pack.title;
            notesChanged = false;
            noteMessage.clear();
        } catch (const std::exception& error) {
            noteMessage = error.what();
        }
    }
    focusNotes = true;
    notesWindow->IsOpen = true;
}

void __stdcall RenderNotes()
{
    bool open = notesWindow->IsOpen;
    UI::SetNextWindowSize({ 740, 520 }, UI::ImGuiCond_FirstUseEver);
    const bool visible = UI::Begin("My Notes###SkyrimTutorialsNotes", &open);
    if (visible) {
        UI::TextWrapped("%s", tutorials::DisplayText(noteTitle).c_str());
        if (note) {
            UI::TextWrapped("%s", "Shared across all saves and characters. Click Save to keep changes. To clear notes, delete the text and Save.");
            if (focusNotes) {
                UI::SetKeyboardFocusHere();
                focusNotes = false;
            }
            const auto available = UI::GetContentRegionAvail();
            if (UI::InputTextMultiline("##personal-notes", noteBuffer.data(), noteBuffer.size(),
                                      { -1, std::max(100.0f, available.y - 110) }, UI::ImGuiInputTextFlags_AllowTabInput))
                notesChanged = true;
            if (UI::Button("Save")) {
                try {
                    tutorials::SaveNote(notesDirectory, *note, noteBuffer.data());
                    notesChanged = false;
                    noteMessage = "Saved for all saves and characters, including after restarting the game.";
                } catch (const std::exception& error) { noteMessage = error.what(); }
            }
            UI::SameLine();
            if (UI::Button("Discard changes / reload saved")) {
                try {
                    auto loaded = tutorials::ReadNote(notesDirectory, note->plugin);
                    noteBuffer.fill(0);
                    std::copy(loaded.text.begin(), loaded.text.end(), noteBuffer.begin());
                    note = std::move(loaded);
                    notesChanged = false;
                    noteMessage = "Saved notes reloaded.";
                } catch (const std::exception& error) { noteMessage = error.what(); }
            }
            UI::SameLine();
            if (UI::Button("Close")) open = false;
            if (notesChanged) UI::TextUnformatted("Unsaved changes");
        }
        if (!noteMessage.empty()) UI::TextWrapped("%s", tutorials::DisplayText(noteMessage).c_str());
    }
    UI::End();
    notesWindow->IsOpen = open;
}

void Log(const std::string& text)
{
    std::ofstream file(pluginDirectory / "SkyrimTutorials.log", std::ios::app);
    file << text << '\n';
}

void Reload()
{
    auto next = tutorials::LoadCatalog(pluginDirectory / "SkyrimTutorials" / "Packs");
    library = std::move(next);
    category.clear();
    modPages = tutorials::BuildModPages(library);
    browser = {};
    originalTopic.clear();
    Log(std::format("Loaded {} packs, {} articles, {} errors", library.packs.size(), library.count, library.errors.size()));
    for (const auto& error : library.errors) Log(error);
}

using Result = tutorials::Topic;

void Document(const Result* result, bool showTitle = true)
{
    if (!result) {
        if (library.packs.empty()) {
            UI::TextWrapped("%s", "Mod Atlas is installed. Install a separate mod information pack, then click Reload information packs.");
            return;
        }
        UI::TextWrapped("%s", "No matching articles. Choose another category or clear the search.");
        if (category == "Followers")
            UI::TextWrapped("%s", "Follower guides appear here when a corresponding tutorial pack is installed.");
        return;
    }
    const auto& article = *result->article;
    if (showTitle) UI::TextWrapped("%s", tutorials::DisplayText(article.title).c_str());
    bool showOriginal = originalTopic == result->key;
    if (!article.original.empty() && article.original != article.text && UI::Checkbox("Show original source text", &showOriginal))
        originalTopic = showOriginal ? result->key : "";
    const auto& body = showOriginal && !article.original.empty() ? article.original : article.text;
    for (const auto& block : tutorials::FormatDocument(tutorials::DisplayText(body))) {
        if (block.kind == tutorials::TextBlockKind::Heading) {
            UI::Spacing();
            UI::TextWrapped("%s", block.text.c_str());
            UI::Separator();
        } else {
            UI::TextWrapped("%s", block.text.c_str());
            if (block.kind == tutorials::TextBlockKind::Paragraph) UI::Spacing();
        }
    }
    UI::Spacing();
    UI::Separator();
    if (UI::CollapsingHeader("Sources and mod version")) {
        if (!article.source.empty()) UI::TextWrapped("%s", tutorials::DisplayText(article.source).c_str());
        UI::TextWrapped("Information pack: %s - %s", tutorials::DisplayText(result->pack->title).c_str(), result->pack->version.c_str());
        if (!result->pack->documentedModVersion.empty())
            UI::TextWrapped("Documented mod version: %s", tutorials::DisplayText(result->pack->documentedModVersion).c_str());
        if (!result->pack->installedOrigin.empty())
            UI::TextWrapped("Source archive: %s", tutorials::DisplayText(result->pack->installedOrigin).c_str());
        if (!result->pack->reviewedOn.empty())
            UI::TextWrapped("Reviewed: %s", result->pack->reviewedOn.c_str());
        if (!result->pack->requiresPlugin.empty())
            UI::TextWrapped("Plugin: %s", tutorials::DisplayText(result->pack->requiresPlugin).c_str());
        if (!result->pack->requiresFile.empty())
            UI::TextWrapped("Mod file: %s", tutorials::DisplayText(result->pack->requiresFile).c_str());
        if (!result->pack->sourcePluginHash.empty())
            UI::TextWrapped("Source file SHA-256: %s", result->pack->sourcePluginHash.c_str());
    }
}

void Navigation()
{
    UI::TextUnformatted("Choose a mod");
    UI::SetNextItemWidth(-1);
    UI::InputTextWithHint("##search", "Find a mod, follower, home or topic...", query.data(), query.size());
    UI::SetNextItemWidth(-1);
    if (UI::BeginCombo("##category", category.empty() ? "Mods" : tutorials::DisplayText(category).c_str())) {
        if (UI::Selectable("Mods", category.empty())) {
            category.clear();
            query.fill(0);
            browser = {};
        }
        for (const auto& item : library.categories) {
            if (item == "Mods") continue;
            if (UI::Selectable(tutorials::DisplayText(item).c_str(), category == item)) category = item;
        }
        UI::EndCombo();
    }
    if (UI::Button("Clear filters")) {
        query.fill(0);
        category.clear();
        browser = {};
    }
    const auto search = tutorials::Fold(query.data());
    std::size_t count = 0;
    bool selectionVisible = false;
    for (const auto& page : modPages) {
        if (!tutorials::PageMatches(page, category, search)) continue;
        ++count;
        if (page.key == browser.mod) selectionVisible = true;
    }
    if (!selectionVisible) browser = {};
    UI::Text("%zu mods / subjects", count);
    UI::Separator();
    if (UI::BeginChild("mod-list", {0, 0})) {
        const float rowHeight = UI::GetFontSize() + 12.0f;
        for (const auto& page : modPages) {
            if (!tutorials::PageMatches(page, category, search)) continue;
            UI::PushID(page.key.c_str());
            // Every row is selectable; no interleaved, inert article/group headings.
            if (UI::Selectable(tutorials::DisplayText(page.title).c_str(), browser.mod == page.key, 0, {0, rowHeight})) {
                browser.ChooseMod(modPages, page.key);
                scrollToOverview = true;
                originalTopic.clear();
            }
            UI::PopID();
        }
    }
    UI::EndChild();
}

void ModDocument(const tutorials::ModPage& page)
{
    UI::PushID(page.key.c_str());
    UI::TextWrapped("%s", tutorials::DisplayText(page.title).c_str());
    const auto& main = page.topics[page.overview];
    if (UI::Button("My Notes")) OpenNotes(*main.pack);
    UI::Separator();
    UI::TextUnformatted("Overview");
    UI::PushID("overview");
    Document(&main, false);
    UI::PopID();
    if (page.topics.size() > 1) {
        UI::Spacing();
        UI::Separator();
        UI::TextUnformatted("More information");
        const auto search = tutorials::Fold(query.data());
        std::string lastGroup;
        for (std::size_t i = 0; i < page.topics.size(); ++i) {
            if (i == page.overview) continue;
            const auto& topic = page.topics[i];
            if (!search.empty() && !tutorials::Matches(*topic.article, "", search)) continue;
            const auto& group = topic.article->group;
            if (!group.empty() && group != page.title && group != lastGroup) {
                UI::Spacing();
                UI::TextWrapped("%s", tutorials::DisplayText(group).c_str());
                lastGroup = group;
            }
            UI::PushID(topic.key.c_str());
            const auto label = std::string(browser.topic == topic.key ? "- " : "+ ") + topic.article->title;
            if (UI::Button(tutorials::DisplayText(label).c_str(), {-1, 0})) {
                browser.ChooseTopic(page, topic.key);
                originalTopic.clear();
            }
            if (browser.topic == topic.key) {
                UI::Spacing();
                Document(&topic, false);
                UI::Spacing();
            }
            UI::PopID();
        }
    }
    UI::PopID();
}

void __stdcall Render()
{
    try {
        UI::TextUnformatted("Mod Atlas");
        UI::SameLine();
        if (UI::Button("Reload information packs")) Reload();
        if (!library.errors.empty() && UI::CollapsingHeader("Information pack issues"))
            for (const auto& error : library.errors) UI::TextWrapped("%s", error.c_str());
        UI::Separator();
        const float width = UI::GetContentRegionAvail().x;
        const float scale = UI::GetFontSize() / 18.0f;
        const bool stacked = width < 650.0f * scale;
        const float navigationWidth = std::clamp(width * .40f, 250.0f * scale, 450.0f * scale);
        if (UI::BeginChild("mods", {stacked ? 0 : navigationWidth, stacked ? 270.0f * scale : 0}, 1))
            Navigation();
        UI::EndChild();
        if (!stacked) UI::SameLine();
        // Resolve after navigation: a click updates this page in the same frame.
        if (UI::BeginChild("mod-page", {0, 0}, 1)) {
            if (scrollToOverview) {
                UI::SetScrollY(0);
                scrollToOverview = false;
            }
            const auto current = std::find_if(modPages.begin(), modPages.end(), [](const auto& page) {
                return page.key == browser.mod;
            });
            if (current != modPages.end()) ModDocument(*current);
            else if (library.packs.empty()) Document(nullptr);
            else {
                UI::TextUnformatted("Choose a mod to get started");
                UI::TextWrapped("%s", "Select a mod, follower or home on the left. Its overview appears here, with more topics underneath.");
            }
        }
        UI::EndChild();
    } catch (const std::exception& error) {
        if (renderError != error.what()) {
            renderError = error.what();
            Log("Render error: " + renderError);
        }
    }
}

void Register()
{
    if (registered) return;
    auto module = GetModuleHandleW(L"SKSEMenuFramework.dll");
    if (!module) {
        Log("SKSE Menu Framework is not loaded. Mod Atlas cannot register.");
        return;
    }
    // Every wrapper used by this plugin is checked before its first call.
    constexpr const char* exports[] = {
        "AddSectionItem", "igTextUnformatted", "igTextWrappedV", "igTextV", "igSeparator",
        "igSpacing", "igCheckbox", "igSetNextItemWidth", "igInputTextWithHint",
        "igBeginCombo", "igEndCombo", "igSelectable_Bool", "igPushID_Str", "igPopID",
        "igButton", "igBeginChild_Str", "igEndChild", "igSameLine", "igCollapsingHeader_TreeNodeFlags",
        "igGetContentRegionAvail", "igGetFontSize", "AddWindow", "igBegin", "igEnd",
        "igSetNextWindowSize", "igSetKeyboardFocusHere", "igInputTextMultiline", "igSetScrollY_Float"
    };
    for (const auto* name : exports) {
        if (!GetProcAddress(module, name)) {
            Log(std::string("Incompatible menu framework, missing export: ") + name);
            return;
        }
    }
    Reload();
    // A framework blocking window routes typing to ImGui while suppressing game input.
    auto createWindow = GetMenuFrameworkFunction<SKSEMenuFramework::Model::AddWindowFunction>("AddWindow");
    notesWindow = createWindow(RenderNotes);
    if (!notesWindow) {
        Log("The My Notes window could not be registered.");
        return;
    }
    notesWindow->BlockUserInput = true;
    SKSEMenuFramework::SetSection("Mod Atlas");
    SKSEMenuFramework::AddSectionItem("Browse", Render);
    registered = true;
    Log("Registered Mod Atlas/Browse and My Notes");
}

void OnMessage(skse_abi::Message* message)
{
    if (message && (message->type == 1 || message->type == 8)) {
        try { Register(); }
        catch (const std::exception& error) { Log(std::string("Registration failed: ") + error.what()); }
    }
}
}

extern "C" __declspec(dllexport) bool SKSEPlugin_Query(const skse_abi::Interface* skse, skse_abi::PluginInfo* info)
{
    if (!skse || !info) return false;
    *info = { 1, "Mod Atlas", 6 };
    return !skse->isEditor && skse->runtimeVersion == skse_abi::runtime_1597;
}

extern "C" __declspec(dllexport) bool SKSEPlugin_Load(const skse_abi::Interface* skse)
{
    if (!skse || skse->isEditor || skse->runtimeVersion != skse_abi::runtime_1597 ||
        !skse->QueryInterface || !skse->GetPluginHandle) return false;
    try {
        HMODULE self = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCWSTR>(&SKSEPlugin_Load), &self);
        std::array<wchar_t, 32768> path{};
        const auto length = GetModuleFileNameW(self, path.data(), static_cast<DWORD>(path.size()));
        if (!length || length >= path.size()) return false;
        pluginDirectory = std::filesystem::path(path.data()).parent_path();
        Log("Mod Atlas 0.4.1 loading for Skyrim 1.5.97");
        auto* messaging = static_cast<skse_abi::Messaging*>(skse->QueryInterface(5));
        return messaging && messaging->version >= 2 && messaging->RegisterListener &&
               messaging->RegisterListener(skse->GetPluginHandle(), "SKSE", OnMessage);
    } catch (...) { return false; }
}
