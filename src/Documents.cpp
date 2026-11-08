#include "Documents.h"
#include <algorithm>
#include <cstdint>
#include <format>
#include <sstream>
#include <stdexcept>

namespace tutorials {
namespace {
std::string Utf8(const std::filesystem::path& path)
{
    const auto bytes = path.u8string();
    return {bytes.begin(), bytes.end()};
}
std::string Lines(std::string text)
{
    if (text.starts_with("\xef\xbb\xbf")) text.erase(0, 3);
    std::string result;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r') { result += '\n'; if (i + 1 < text.size() && text[i + 1] == '\n') ++i; }
        else result += text[i];
    }
    return result;
}
std::string Trim(const std::string& value)
{
    const auto a = value.find_first_not_of(" \t\n");
    if (a == std::string::npos) return {};
    return value.substr(a, value.find_last_not_of(" \t\n") - a + 1);
}
std::string BasicMarkup(const std::string& source)
{
    const auto input = Lines(source);
    std::string out;
    std::string lowerInput;
    for (std::size_t i = 0; i < input.size();) {
        if (input.compare(i, 2, "**") == 0 || input.compare(i, 2, "__") == 0) { i += 2; continue; }
        if (input[i] == '<' || input[i] == '[') {
            const char close = input[i] == '<' ? '>' : ']';
            const auto end = input.find(close, i + 1);
            if (end != std::string::npos && end - i < 256) {
                const auto tag = Fold(Trim(input.substr(i + 1, end - i - 1)));
                if (input[i] == '<' && (tag == "script" || tag.starts_with("script ") || tag == "style" || tag.starts_with("style "))) {
                    if (lowerInput.empty()) {
                        lowerInput = input;
                        for (auto& c : lowerInput) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
                    }
                    const auto ending = lowerInput.find(tag.starts_with("script") ? "</script>" : "</style>", end + 1);
                    i = ending == std::string::npos ? input.size() : input.find('>', ending) + 1;
                    continue;
                }
                const auto name = tag.substr(0, tag.find_first_of(" ="));
                if (name == "br" || name == "br/" || name == "p" || name == "/p" || name == "div" || name == "/div" || name == "list" || name == "/list" || name == "ul" || name == "/ul" || name == "ol" || name == "/ol")
                    out += '\n';
                else if (name == "li" || name == "*") out += "\n- ";
                else if (name == "/li") out += '\n';
                else if (name == "h1" || name == "h2" || name == "h3") out += "\n# ";
                else if (name == "/h1" || name == "/h2" || name == "/h3") out += '\n';
                else if (name == "b" || name == "/b" || name == "strong" || name == "/strong" || name == "i" || name == "/i" || name == "em" || name == "/em" || name == "u" || name == "/u" || name == "font" || name == "/font" || name == "span" || name == "/span" || name == "color" || name == "/color" || name == "size" || name == "/size") {}
                else { out += input[i++]; continue; }
                i = end + 1;
                continue;
            }
        }
        if (input[i] == '&') {
            static const std::pair<std::string, std::string> entities[] = {
                {"&amp;", "&"}, {"&lt;", "<"}, {"&gt;", ">"}, {"&quot;", "\""}, {"&apos;", "'"}, {"&nbsp;", " "}
            };
            bool found = false;
            for (const auto& [entity, value] : entities) if (input.compare(i, entity.size(), entity) == 0) {
                out += value; i += entity.size(); found = true; break;
            }
            if (found) continue;
        }
        out += input[i++];
    }
    return out;
}
}

Pack ReadTextDocument(const std::filesystem::path& path, std::string text, const std::string& title, const std::string& category)
{
    text = Lines(std::move(text));
    if (text.size() > 2 * 1024 * 1024 || text.find('\0') != std::string::npos || Trim(text).empty())
        throw std::runtime_error("Document must contain UTF-8 text, up to 2 MiB, without null characters");
    Fold(text);
    Pack pack;
    const auto file = Utf8(path.filename());
    std::uint64_t hash = 14695981039346656037ull;
    for (const unsigned char c : Fold(file)) { hash ^= c; hash *= 1099511628211ull; }
    pack.id = std::format("document-{:016x}", hash);
    pack.title = title.empty() ? Utf8(path.stem()) : title;
    if (pack.title.empty() || pack.title.size() > 512 || category.empty() || category.size() > 512)
        throw std::runtime_error("Document title/category must contain 1 to 512 UTF-8 bytes");
    Fold(pack.title); Fold(category);
    pack.version = "1.0.0";
    pack.requiresFile = "SKSE/Plugins/SkyrimTutorials/Packs/" + file;
    Article first;
    first.id = "overview"; first.title = pack.title; first.subject = pack.title; first.category = category; first.overview = true;
    pack.articles.push_back(first);
    std::istringstream lines(text);
    for (std::string line; std::getline(lines, line);) {
        if (line.starts_with("## ") && !Trim(line.substr(3)).empty()) {
            Article section;
            section.id = "section-" + std::to_string(pack.articles.size());
            section.title = Trim(line.substr(3)); section.subject = pack.title; section.category = category; section.group = "Sections";
            if (section.title.size() > 512) throw std::runtime_error("Section title exceeds 512 UTF-8 bytes");
            pack.articles.push_back(section);
            if (pack.articles.size() > 512) throw std::runtime_error("Document exceeds 512 sections");
        } else pack.articles.back().text += line + '\n';
    }
    for (auto& article : pack.articles) {
        article.text = Trim(article.text);
        if (article.text.size() > 128 * 1024) throw std::runtime_error("Document section exceeds 128 KiB; split it with ## headings");
        article.source = "Content file: " + file;
    }
    return pack;
}

std::vector<TextBlock> FormatDocument(const std::string& text)
{
    std::vector<TextBlock> blocks;
    std::istringstream lines(BasicMarkup(text));
    std::string paragraph;
    auto flush = [&] { if (!paragraph.empty()) { blocks.push_back({TextBlockKind::Paragraph, paragraph}); paragraph.clear(); } };
    for (std::string raw; std::getline(lines, raw);) {
        const auto line = Trim(raw);
        if (line.empty()) { flush(); continue; }
        const auto marker = line.find_first_not_of('#');
        if (marker != std::string::npos && marker >= 1 && marker <= 6 && line[marker] == ' ') {
            flush(); blocks.push_back({TextBlockKind::Heading, Trim(line.substr(marker))});
        } else if (line.starts_with("- ") || line.starts_with("* ")) {
            flush(); blocks.push_back({TextBlockKind::ListItem, "- " + line.substr(2)});
        } else {
            if (!paragraph.empty()) paragraph += '\n';
            paragraph += raw;
        }
    }
    flush();
    return blocks;
}
}
