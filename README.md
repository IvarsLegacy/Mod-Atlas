# Mod Atlas 0.4.1

Mod Atlas displays searchable mod information in Skyrim. Authors and players can simply write or paste text into a UTF-8 `.txt` or `.md` file; the filename supplies its title. JSON information packs remain supported. No DLL compilation or programming is needed to add content.

## Requirements

- Windows x64, Skyrim Special Edition **1.5.97**, SKSE **2.0.20**.
- **SKSE Menu Framework 3**, developed against version **3.14**.

This build checks the exact Skyrim runtime. Other Skyrim versions are not supported by this build. SKSE and SKSE Menu Framework are separate requirements and are not included.

## Install and open

1. Install `Mod-Atlas-0.4.1-Core.zip` with your mod manager.
2. Start Skyrim through SKSE and open SKSE Menu Framework using its configured key.
3. Choose **Mod Atlas → Browse**.

The framework contains no installed mod articles. With no information packs, the browser displays an instruction to add them. Install a compatible information pack separately, or copy and rename **example-note.txt** into `Data/SKSE/Plugins/SkyrimTutorials/Packs/` and replace its text. See **AUTHORING.md** for this beginner route and the structured JSON option. After adding or editing a file, click **Reload information packs**, or restart the game.

**Mods** is the complete overview of all installed information pages. Choosing it also clears the search. Followers, Player Homes and other categories narrow this overview. Typographic quotes, long dashes and similar punctuation use font-friendly display equivalents. The original UTF-8 pack text, mod identities and personal note files are preserved; accented letters are not converted to ASCII.

Choose a category, then click a mod, follower or home in the list on the left. Its **Overview** opens first. Expand a topic under **More information** to read it underneath; topics never appear as extra mods in that list. Search accepts several words and also finds text in subtopics while keeping the parent overview available. On a narrow window the mod list appears above the page. **Information pack issues** identifies invalid files while other valid packs remain usable. Article sources are supplied by their authors; Mod Atlas does not track NPC positions or check the truth of a guide.

## My Notes

Open an article, click **My Notes**, type and click **Save**. Unsaved edits are not persisted. Saved notes are **shared across every save and character for the same Windows user**. Loading an older save, starting a new character or restarting Skyrim does not roll them back. Notes are stored separately under `%LOCALAPPDATA%/SkyrimTutorials/UserNotes`, not in game saves or the mod installation.

To edit, change the text and **Save**. To clear a mod's saved note, delete all its text and **Save**; closing without saving does not erase the saved version. Notes survive framework updates, reinstallation and information-pack replacement when the same stable mod identity is retained. Removing an information pack does not remove its notes. Back up the UserNotes folder to transfer your own notes. Mod Atlas does not include or upload personal notes.

## Files and removal

The installed framework DLL is `SKSE/Plugins/SkyrimTutorials.dll`. Information goes directly under `SKSE/Plugins/SkyrimTutorials/Packs/`. These compatibility filenames are retained from the prototype. The public product and menu name is **Mod Atlas**. No ESP is needed.

Close the game before disabling the framework. Information packs can be enabled, updated or removed independently. If upgrading from the combined Skyrim Tutorials prototype, disable that combined package first; do not run two copies of its DLL. Existing compatible information files and note identities remain usable.

Logs: `SKSE/Plugins/SkyrimTutorials.log`.

## Verification and source

Offline build, catalog loading, invalid-file isolation, search, note persistence and split-package checks are available. The user reported the preceding 0.4.0 build working in game. This naming update has no separate in-game test; the agent has not launched Skyrim or performed a mod-manager UI test.

`Mod-Atlas-0.4.1-Source.zip` supplies the framework source, build configuration, generic example and dependency licenses. See **BUILDING.md** for exact commands. Information authors do not need these tools. Mod Atlas's original framework code and public documentation use the MIT license in **LICENSE**; **THIRD_PARTY.md** explains the separately licensed dependencies.

The vendored SKSE Menu Framework API header is available from https://github.com/QTR-Modding/SKSE-Menu-Framework-3-API under LGPL 2.1. nlohmann/json 3.12.0 uses the MIT license. Their license texts accompany Mod Atlas. The SKSE Menu Framework DLL is not bundled.



