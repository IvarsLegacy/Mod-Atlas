#include "Browser.h"
#include <algorithm>
#include <map>

namespace tutorials {
namespace {
int OverviewScore(const Topic& topic, const std::string& subject)
{
    if (topic.article->overview) return 1000;
    const auto title = Fold(topic.article->title);
    const auto id = topic.article->id;
    int score = Fold(topic.pack->title) == Fold(subject) ? 5 : 0;
    if (id.find("fundort") != std::string::npos || title.find("location") != std::string::npos)
        score += 100;
    else if (title.find("overview") != std::string::npos || title.find("getting started") != std::string::npos ||
             title.find("welcome") != std::string::npos || title.find("introduction") != std::string::npos)
        score += 80;
    else if (title.find("basics") != std::string::npos || title.find("access") != std::string::npos)
        score += 40;
    return score;
}
}

std::vector<ModPage> BuildModPages(const Library& library)
{
    std::map<std::string, ModPage> grouped;
    for (const auto& pack : library.packs) {
        for (const auto& article : pack.articles) {
            const auto key = Fold(article.subject);
            auto& page = grouped[key];
            page.key = key;
            if (page.title.empty()) page.title = article.subject;
            page.topics.push_back({&pack, &article, pack.id + "/" + article.id});
        }
    }
    std::vector<ModPage> pages;
    for (auto& [key, page] : grouped) {
        for (std::size_t i = 1; i < page.topics.size(); ++i)
            if (OverviewScore(page.topics[i], page.title) > OverviewScore(page.topics[page.overview], page.title))
                page.overview = i;
        pages.push_back(std::move(page));
    }
    return pages;
}

bool PageMatches(const ModPage& page, const std::string& category, const std::string& query)
{
    return std::any_of(page.topics.begin(), page.topics.end(), [&](const Topic& topic) {
        return Matches(*topic.article, category == "Mods" ? "" : category, query);
    });
}

bool BrowserSelection::ChooseMod(const std::vector<ModPage>& pages, const std::string& key)
{
    if (std::none_of(pages.begin(), pages.end(), [&](const ModPage& page) { return page.key == key; })) return false;
    mod = key;
    topic.clear();
    return true;
}

bool BrowserSelection::ChooseTopic(const ModPage& page, const std::string& key)
{
    if (mod != page.key || key == page.topics[page.overview].key ||
        std::none_of(page.topics.begin(), page.topics.end(), [&](const Topic& item) { return item.key == key; })) return false;
    topic = topic == key ? "" : key;
    return true;
}
}
