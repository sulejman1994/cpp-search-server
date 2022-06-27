#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <unordered_set>
#include <set>
#include <algorithm>

std::vector<std::string_view> SplitIntoWords(std::string_view str);

template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string> non_empty_strings;
    for (const auto& str : strings) { 
        if (!str.empty()) {   
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

