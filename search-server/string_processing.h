#pragma once
#include "document.h"
#include<iostream>
#include<vector>
#include<string> 
#include<set>
using namespace std;

vector<string> SplitIntoWords(const string& text);

ostream& operator << (ostream& out, const Document& document);

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}