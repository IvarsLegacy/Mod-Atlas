# Add your own Mod Atlas information

You only need a text editor. No DLL compilation, Papyrus script or ESP edit is required.

## Easiest: a text file

Create **My important reminders.txt**, write or paste your text, and save it as **UTF-8**. Put it directly in `Data/SKSE/Plugins/SkyrimTutorials/Packs/`, or install a mod folder/ZIP with that same relative path. Click **Reload information packs**. The filename becomes its menu title, and the default category is **Personal**. No mod association, ID or JSON is required. `.md` files work too.

Complete example file:

```text
Remember these things before setting off.

## Travel
- Bring supplies.
- Check your equipment.

## Later
Write another reminder here.
```

The opening text is the overview. Each `## Section name` becomes a clickable subtopic underneath it. If you only write ordinary text, the entire file is one page. Copy and rename **example-note.txt** to get started.

Blank lines separate paragraphs; line breaks and readable wrapping are handled by Mod Atlas. `# Heading` is displayed as a heading inside an article. `- item` and `* item` are list items. Basic HTML paragraph/line-break/list/heading tags and simple BBCode bold/italic/list tags are converted to readable text. Bold/italic markers may be simplified to plain text. Unknown markup remains literal. HTML script/style blocks are omitted; no scripts, HTML applications, console commands or remote images are executed or loaded. Pasting text does not preserve every website's fonts, tables or rich formatting.

The original file stays unchanged. Reload after editing it. Free-document My Notes belong to the content filename and survive edits and game/save/character changes. Keep the filename to retain that association. These saved My Notes remain separate from the content file.

## Minimal JSON, if preferred

This complete file also works, taking its title and Personal category from the filename:

```json
{"text": "My own information.\n\nAnother paragraph."}
```

Optional `title`, `category`, `source` and `requires_plugin` fields can customize it. JSON needs escaped quotes and newlines; TXT/Markdown avoids that requirement entirely.

## Structured packs for mod authors

1. Copy **example-pack.json**, rename it to a unique name such as `yourname-yourmod.json`, and open it in a text editor.
2. Replace the example mod name, plugin filename, IDs, article title and text. Keep the quotes and commas. Save as **UTF-8** with the `.json` extension, not `.json.txt`.
3. Put the file directly in `Data/SKSE/Plugins/SkyrimTutorials/Packs/`. With a mod manager, put `SKSE/Plugins/SkyrimTutorials/Packs/yourname-yourmod.json` inside a separate mod folder or ZIP and install it. A mod author can ship the same relative path with their gameplay mod. No absolute machine-specific paths are needed.
4. Open **Mod Atlas → Browse → Reload information packs**. Your text appears in search and under its category and subject.

Minimal complete example:

```json
{
  "schema_version": 1,
  "id": "example-mod-information",
  "title": "Example Mod",
  "version": "1.0.0",
  "requires_plugin": "ExampleMod.esp",
  "articles": [
    {
      "id": "getting-started",
      "is_overview": true,
      "category": "Gameplay",
      "subject": "Example Mod",
      "title": "Getting started",
      "text": "Explain where to begin and what to do.\n\nAdd a second paragraph here."
    }
  ]
}
```

**Category** groups information. Suggested values: `Followers`, `Player Homes`, `Gameplay`, `Quests & Locations`. **Subject** names a mod page, individual follower or home. If omitted, the pack title is used. Articles with the same subject appear together on one page, including compatible addon packs. **Title** labels the article; **text** is its plain text body. `\n` inserts a line break.

Set **`is_overview: true`** on the main article for each subject. Opening the mod shows that introduction first. For a second article, copy the article object inside the array, separate it with a comma, give it another ID and omit `is_overview` or set it to `false`. Keep the same subject: this article becomes a clickable subtopic under **More information**, not another mod in the main list. Use `group` to label a set of related subtopics. Existing packs without this new optional flag remain supported; Mod Atlas selects a location/introduction article where available, otherwise the first article.

Use unique pack IDs, and unique article IDs within each pack. IDs use lowercase letters, digits, periods, hyphens or underscores. Preserve IDs when updating an existing pack. Do not reuse the example IDs in a published pack.

`requires_plugin` names the mod that owns these notes; it does not hide information when that plugin is inactive. Replace the example filename with the real plugin name. For a mod without a plugin, use `requires_file` instead, for example `SKSE/Plugins/Example.dll`, or omit both for a general guide. With neither field, the pack ID owns its notes. Keep this identity stable across updates to preserve My Notes.

Optional article fields: `group` (section), `source` (documentation link or attribution), `tags` (a JSON array of search words). Optional pack field: `documented_mod_version` (the gameplay-mod version your text describes). Plain text is displayed literally; HTML, scripts and console commands are not executed.

Files must be directly inside `Packs`; subfolders are not scanned. Mod Atlas accepts `.json`, `.txt` and `.md`, up to 512 articles per pack, 2 MiB per file and 128 KiB of UTF-8 text per article/section. Split long text files with `##` headings. Duplicate pack IDs use the first valid file in filename order. Invalid files are named under **Information pack issues**; other valid files continue loading. Correct the named file and reload.

Distribute your JSON as an information pack with **Mod Atlas** and its prerequisites listed as requirements. Do not bundle personal My Notes, another author's private files or their work without permission.
