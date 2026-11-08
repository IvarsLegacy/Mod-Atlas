#include <Windows.h>
#include "Library.h"
#include <iostream>

int wmain(int argc, wchar_t** argv)
{
    if (argc < 2 || argc > 3) return 2;
    const auto library = tutorials::LoadCatalog(argv[1]);
    std::cout << library.packs.size() << " packs, " << library.count << " articles\n";
    for (const auto& error : library.errors) std::cerr << error << '\n';
    if (argc == 3) {
        const int size = WideCharToMultiByte(CP_UTF8, 0, argv[2], -1, nullptr, 0, nullptr, nullptr);
        std::string input(size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, argv[2], -1, input.data(), size, nullptr, nullptr);
        input.pop_back();
        const auto query = tutorials::Fold(input);
        std::size_t matches = 0;
        for (const auto& pack : library.packs) {
            for (const auto& article : pack.articles) {
                if (tutorials::Matches(article, "", query)) ++matches;
            }
        }
        std::cout << "matches " << matches << '\n';
        if (matches == 0) return 3;
    }
    return library.errors.empty() ? 0 : 1;
}
