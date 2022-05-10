#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <unordered_set>
#include <set>
#include <algorithm>

using std::vector, std::string, std::string_view, std::set;

vector<string_view> SplitIntoWords(string_view str);

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const auto& str : strings) { 
        if (!str.empty()) {   
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

