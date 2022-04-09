#include "remove_duplicates.h"

set<string> CalcSetOfWords(const map<string, double>& word_to_freqs) {
    set<string> result;
    for(const auto& [word, _]: word_to_freqs) {
        result.insert(word);
    }
    return result;
}


void RemoveDuplicates(SearchServer& search_server) {
    set< set<string> > words_in_docs;
    set<int> deleted_ids;
    
    for (const int document_id: search_server) {
        auto word_to_freqs = search_server.GetWordFrequencies(document_id);
        auto set_of_words = CalcSetOfWords(word_to_freqs);
        
        if (words_in_docs.count(set_of_words) > 0) {
            deleted_ids.insert(document_id);
        } else {
            words_in_docs.insert(set_of_words);
        }
    }
    
    for(int id: deleted_ids) {
        search_server.RemoveDocument(id);
        cout << "Found duplicate document id "s << id << endl;
    }
}

