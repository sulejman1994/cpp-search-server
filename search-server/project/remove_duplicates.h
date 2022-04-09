#pragma once

#include "search_server.h"
#include <set>
#include <map>
#include <vector>
#include <numeric>
using namespace std;

set<string> CalcSetOfWords(const map<string, double>& word_to_freqs);

void RemoveDuplicates(SearchServer& search_server);
