#pragma once
#include "Library.h"

namespace tutorials {
struct Topic {
    const Pack* pack;
    const Article* article;
    std::string key;
};
struct ModPage {
    std::string key, title;
    std::vector<Topic> topics;
    std::size_t overview = 0;
};
struct BrowserSelection {
    std::string mod, topic;
    bool ChooseMod(const std::vector<ModPage>& pages, const std::string& key);
    bool ChooseTopic(const ModPage& page, const std::string& key);
};
std::vector<ModPage> BuildModPages(const Library& library);
bool PageMatches(const ModPage& page, const std::string& category, const std::string& query);
}
