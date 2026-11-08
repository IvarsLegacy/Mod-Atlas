#include <Windows.h>
#include <iostream>
#include <stdexcept>
#include "Notes.h"
#include "Library.h"
#include "Documents.h"

std::string Utf8(const wchar_t* value)
{
    const int size = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), size, nullptr, nullptr);
    result.pop_back();
    return result;
}

int wmain(int argc, wchar_t** argv)
{
    if (argc != 4) return 2;
    try {
        tutorials::Pack dllPack, renamedPack, oldPluginPack;
        dllPack.id = "original-pack";
        dllPack.requiresFile = "SKSE\\Plugins\\Example.dll";
        renamedPack.id = "renamed-pack";
        renamedPack.requiresFile = "skse/plugins/EXAMPLE.DLL";
        oldPluginPack.requiresPlugin = "ExampleMod.esp";
        tutorials::Pack replacementPluginPack;
        replacementPluginPack.id = "replacement-information-pack";
        replacementPluginPack.title = "Changed information title";
        replacementPluginPack.requiresPlugin = "EXAMPLEMOD.ESP";
        if (tutorials::NoteKey(dllPack) != tutorials::NoteKey(renamedPack) ||
            tutorials::NoteKey(oldPluginPack) != "examplemod.esp" ||
            tutorials::NoteKey(oldPluginPack) != tutorials::NoteKey(replacementPluginPack))
            throw std::runtime_error("Stable mod note identity failed");
        const std::filesystem::path root(argv[1]);
        const auto action = Utf8(argv[2]);
        const auto text = Utf8(argv[3]);
        if (action == "document-write" || action == "document-read") {
            const auto document = tutorials::ReadTextDocument(L"My Guide.txt", action == "document-write" ? "Initial guide." : "Replaced guide text.");
            auto documentNote = tutorials::ReadNote(root, tutorials::NoteKey(document));
            if (action == "document-write") tutorials::SaveNote(root, documentNote, text);
            else if (documentNote.text != text) throw std::runtime_error("Document notes did not survive replacing content and restarting");
            return 0;
        }
        if (action == "file-write") {
            auto fileNote = tutorials::ReadNote(root, tutorials::NoteKey(dllPack));
            tutorials::SaveNote(root, fileNote, text);
            return 0;
        }
        if (action == "file-read") {
            if (tutorials::ReadNote(root, tutorials::NoteKey(renamedPack)).text != text)
                throw std::runtime_error("DLL notes lost across slash, case or pack ID change");
            return 0;
        }
        auto note = tutorials::ReadNote(root, "ExampleMod.esp");
        if (action == "write") {
            tutorials::SaveNote(root, note, text);
        } else if (action == "read") {
            const auto reopened = tutorials::ReadNote(root, "EXAMPLEMOD.ESP");
            if (reopened.text != text) throw std::runtime_error("Restart persistence or case-stable key failed");
        } else if (action == "conflict") {
            auto stale = tutorials::ReadNote(root, "ExampleMod.esp");
            tutorials::SaveNote(root, note, text);
            bool refused = false;
            try { tutorials::SaveNote(root, stale, "Stale update"); }
            catch (const std::runtime_error&) { refused = true; }
            if (!refused || tutorials::ReadNote(root, "ExampleMod.esp").text != text)
                throw std::runtime_error("Concurrent update was not protected");
        } else if (action == "clear") {
            tutorials::SaveNote(root, note, "");
            if (!tutorials::ReadNote(root, "EXAMPLEMOD.ESP").text.empty())
                throw std::runtime_error("Explicit clearing did not persist");
        } else if (action == "limit") {
            tutorials::SaveNote(root, note, std::string(65535, 'a'));
            bool refused = false;
            try { tutorials::SaveNote(root, note, std::string(65536, 'b')); }
            catch (const std::runtime_error&) { refused = true; }
            if (!refused || tutorials::ReadNote(root, "ExampleMod.esp").text != std::string(65535, 'a'))
                throw std::runtime_error("Size limit damaged the saved note");
        } else return 2;
        std::cout << action << " passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}

