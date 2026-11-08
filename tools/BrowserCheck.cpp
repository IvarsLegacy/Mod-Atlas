#include "Browser.h"
#include "DisplayText.h"
#include "Documents.h"
#include <iostream>
#include <set>
#include <stdexcept>

void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
tutorials::Article Article(const char* id, const char* subject, const char* title, const char* text, bool overview = false)
{
    tutorials::Article a;
    a.id = id; a.category = "Followers"; a.subject = subject; a.title = title; a.text = text;
    a.search = tutorials::Fold(std::string(subject) + " " + title + " " + text); a.overview = overview;
    return a;
}

int wmain(int argc, wchar_t** argv)
{
    try {
        const std::string original = "Auri’s Pod – ‘hello’… → notes； Häusêr";
        Require(tutorials::DisplayText(original) == "Auri's Pod - 'hello'... -> notes; Häusêr", "Display punctuation fallback failed");
        Require(original == "Auri’s Pod – ‘hello’… → notes； Häusêr", "Display conversion changed its source");
        Require(tutorials::DisplayText("semicolon; colon: apostrophe' question?") == "semicolon; colon: apostrophe' question?", "ASCII punctuation was damaged");
        tutorials::Pack unicodeOwner;
        unicodeOwner.requiresPlugin = "Knight’s Home.esp";
        Require(tutorials::NoteKey(unicodeOwner) == "knight’s home.esp", "Display fallback changed a note identity");
        const auto document = tutorials::ReadTextDocument(L"Meine wichtigen Sachen.txt", "First paragraph.\r\n\r\n## Travel\r\n- Bring supplies\r\n## Home\r\nRemember the key.");
        Require(document.title == "Meine wichtigen Sachen" && document.articles.size() == 3, "Plain text filename/section defaults failed");
        Require(document.articles[0].overview && document.articles[1].title == "Travel" && document.articles[2].title == "Home", "Text sections do not follow overview hierarchy");
        const auto editedDocument = tutorials::ReadTextDocument(L"MEINE WICHTIGEN SACHEN.txt", "Edited content.");
        Require(tutorials::NoteKey(document) == tutorials::NoteKey(editedDocument), "Document content edits changed note ownership");
        const auto blocks = tutorials::FormatDocument("# Heading\n\nA paragraph.\nSecond line.\n\n- A list item\n\n[b]Plain words[/b]<br>Next line<script>never_execute()</script>");
        Require(blocks.size() == 4 && blocks[0].kind == tutorials::TextBlockKind::Heading && blocks[2].kind == tutorials::TextBlockKind::ListItem, "Basic paragraph/heading/list layout failed");
        Require(blocks.back().text == "Plain words\nNext line", "Safe basic markup conversion failed");
        tutorials::Library fixture;
        tutorials::Pack alpha, beta, addon;
        alpha.id = "alpha"; alpha.title = "Alpha";
        alpha.articles = {Article("spell", "Alpha", "Summon", "unicorn command"), Article("intro", "Alpha", "Meet Alpha", "Location: Example Inn", true)};
        beta.id = "beta"; beta.title = "Beta";
        beta.articles = {Article("intro", "Beta", "Meet Beta", "Location: Example Castle", true)};
        addon.id = "alpha-addon"; addon.title = "Alpha optional addon";
        addon.articles = {Article("spell", "Alpha", "Another spell", "second spell"), Article("home", "Alpha", "Home", "housing command")};
        fixture.packs = {alpha, beta, addon};
        const auto grouped = tutorials::BuildModPages(fixture);
        Require(grouped.size() == 2 && grouped[0].topics.size() == 4, "Mod entries contain duplicate child rows");
        Require(grouped[0].topics[grouped[0].overview].article->id == "intro", "Explicit overview was not selected");
        Require(tutorials::PageMatches(grouped[0], "Followers", "unicorn"), "Child search did not return its parent mod");
        Require(tutorials::PageMatches(grouped[0], "Mods", "") && tutorials::PageMatches(grouped[1], "Mods", ""), "Mods does not include entries from other categories");
        Require(!tutorials::PageMatches(grouped[0], "Player Homes", "unicorn"), "Category filtering failed");
        tutorials::BrowserSelection selection;
        Require(selection.ChooseMod(grouped, grouped[0].key), "First mod cannot be selected");
        Require(selection.ChooseTopic(grouped[0], grouped[0].topics[0].key), "Child cannot be opened");
        Require(selection.ChooseMod(grouped, grouped[1].key) && selection.mod == grouped[1].key && selection.topic.empty(), "Second mod cannot be selected or inherits a topic");
        Require(!selection.ChooseTopic(grouped[0], grouped[0].topics[0].key), "A child escaped its parent mod");
        Require(selection.ChooseMod(grouped, grouped[0].key), "Returning to first mod failed");
        Require(selection.ChooseTopic(grouped[0], grouped[0].topics[0].key), "Opening a child failed");
        Require(selection.ChooseTopic(grouped[0], grouped[0].topics[0].key) && selection.topic.empty(), "Closing a child failed");
        Require(!selection.ChooseTopic(grouped[0], grouped[0].topics[grouped[0].overview].key), "Overview is duplicated as a child");
        Require(tutorials::BuildModPages({}).empty(), "Standalone empty framework failed");
        if (argc == 2) {
            const auto library = tutorials::LoadCatalog(argv[1]);
            Require(library.errors.empty(), "Real catalog has loading errors");
            const auto pages = tutorials::BuildModPages(library);
            std::set<std::string> keys, topics;
            for (const auto& page : pages) {
                Require(tutorials::PageMatches(page, "Mods", ""), "A real mod is absent from the Mods overview");
                Require(keys.insert(page.key).second && !page.topics.empty(), "Duplicate/empty mod entry");
                Require(selection.ChooseMod(pages, page.key), "A real mod row is not selectable");
                Require(selection.mod == page.key && selection.topic.empty(), "Mod selection did not reset to overview");
                for (std::size_t i = 0; i < page.topics.size(); ++i) {
                    Require(topics.insert(page.topics[i].key).second, "Duplicate article across mod pages");
                    if (i != page.overview) Require(selection.ChooseTopic(page, page.topics[i].key), "A real subtopic is not selectable");
                }
            }
            Require(topics.size() == library.count, "Hierarchy lost information articles");
            std::cout << pages.size() << " mod pages, " << topics.size() << " nested articles\n";
        }
        std::cout << "Hierarchy, consecutive mod selections, child isolation and overview selection passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
