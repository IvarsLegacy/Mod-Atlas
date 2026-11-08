# Build Mod Atlas from source

Information authors only edit text or JSON files; they do not need this build setup.

Framework development requires Windows x64, Visual Studio 2022 Build Tools with Desktop C++, the Windows SDK, CMake 3.21 or later and Ninja. The API and JSON headers are already vendored; no download or personal mod installation is needed to compile.

Open an **x64 Native Tools Command Prompt for VS 2022** in the source directory containing `CMakeLists.txt`, then run:

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
```

Alternatively, `build.cmd` configures the usual Visual Studio 2022 Build Tools path and runs these commands. With another compiler installation path, use the Native Tools prompt above.

Outputs: `build/SkyrimTutorials.dll`, `build/LibraryCheck.exe`, `build/NotesCheck.exe`, `build/BrowserCheck.exe`. The DLL retains its compatibility filename; its SKSE identity and menu section are Mod Atlas. BrowserCheck tests mod/overview/subtopic grouping and consecutive selections without starting Skyrim; pass an optional pack folder to check a real catalog. It does not simulate mouse input in game.

To check your own information folder with the same parser used by the DLL:

```bat
build\LibraryCheck.exe path\to\Packs
build\LibraryCheck.exe path\to\Packs "search words"
```

An empty or not-yet-created pack folder is valid. Invalid files are named on standard error; other valid files continue loading. A search with no matches returns exit code 3. Other parser failures return 1. These command-line checks do not start Skyrim.

`NotesCheck.exe` is a developer fixture utility. Use a temporary fixture directory, never your personal UserNotes directory.

For a runtime test, install the built DLL at `Data/SKSE/Plugins/SkyrimTutorials.dll` using a mod manager, install the separate prerequisites and launch Skyrim 1.5.97 through SKSE. Open **Mod Atlas → Browse**. Test empty-framework behaviour, then add the generic example or a real information pack, reload, search, and check My Notes keyboard focus and saving across a restart. The framework does not need a fixed game installation path or a particular modlist.
