#include "search_server.h"

using namespace std;

 SearchServer::SearchServer(string_view stop_words_text) {
    for (string_view word: SplitIntoWords(stop_words_text)) {
        if (!IsValidWord(word)) {
             throw invalid_argument("Some of stop words are invalid");
        }
        stop_words_.insert(string(word));
    }
}

 SearchServer::SearchServer(const string& stop_words_text) : SearchServer(string_view(stop_words_text)) {
     
} 
                         
void SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status, const vector<int>& ratings) {
        if ((document_id < 0) || (documents_.count(document_id) > 0)) {
            throw invalid_argument("Invalid document_id"s);
        }
        const auto words = SplitIntoWordsNoStop(document);
    
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            all_words.insert(word);
   
            auto it = all_words.find(word);
            string_view sv(*it);
    
            word_to_document_freqs_[sv][document_id] += inv_word_count;
            id_to_word_freqs_[document_id][sv] += inv_word_count;
            
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
        document_ids_.insert(document_id);
}


vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}


vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}


int SearchServer::GetDocumentCount() const {
        return documents_.size();
}


set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}


const unordered_map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const unordered_map<string_view, double> empty_map;
    if (document_ids_.count(document_id) == 0) {
         return empty_map;
    }
    return id_to_word_freqs_.at(document_id);
}


void SearchServer::RemoveDocument(int document_id) {
    documents_.erase(document_id);
    document_ids_.erase(document_id);
    
    for(const auto& [word, _]: id_to_word_freqs_[document_id]) {
        word_to_document_freqs_[word].erase(document_id);
    }
    
    id_to_word_freqs_.erase(document_id);
}


tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(string_view raw_query, int document_id) const {
        if (document_ids_.count(document_id) == 0) {
            throw out_of_range("out_of_range");
        }
        const auto query = ParseQuery(raw_query);

        vector<string_view> matched_words;
        matched_words.reserve(query.plus_words.size());
        for (const string& word : query.plus_words) {
            auto it = all_words.find(word);
            if (it == all_words.end()) {
                continue;
            }
            string_view sv(*it);
            if (id_to_word_freqs_.at(document_id).count(sv)) {
                matched_words.push_back(sv);
            }
        }
        for (const string& word : query.minus_words) {
            auto it = all_words.find(word);
            if (it == all_words.end()) {
                continue;
            }
            string_view sv(*it);
            if (id_to_word_freqs_.at(document_id).count(sv)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(execution::sequenced_policy execution_type, string_view raw_query, int document_id) const {
    return SearchServer::MatchDocument(raw_query, document_id);
}


tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(execution::parallel_policy execution_type, string_view raw_query, int document_id) const {
   
    const auto query = ParseQuery(raw_query);
  
    vector<string_view> matched_words(query.plus_words.size());
    
    if (any_of(query.minus_words.begin(), query.minus_words.end(), [this, document_id] (const string& word) {
        auto it = all_words.find(word);
        if (it == all_words.end()) {
            return false;
        }
        string_view sv(*it);
        if (id_to_word_freqs_.at(document_id).count(sv)) {
            return true;
        }
        return false;
    }) ) {
    
        return {{}, documents_.at(document_id).status};
    }
    

    transform(std::execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [this, document_id] (const string& word) {
        auto it = all_words.find(word);
        if (it == all_words.end()) {
            return string_view{};
        }
        string_view sv(*it);
        if (id_to_word_freqs_.at(document_id).count(sv)) {
            return sv;
        }
        
        return string_view{};
    } ) ;
    
    
    unordered_set<string_view> set_of_matched_words(matched_words.begin(), matched_words.end());
    if (set_of_matched_words.count(string_view{})) {
        set_of_matched_words.erase(string_view{});
    }
    matched_words.clear();
    matched_words.resize(set_of_matched_words.size());
    transform(set_of_matched_words.begin(), set_of_matched_words.end(), matched_words.begin(), [] (auto sv) {
       return sv; 
    });
    
    return {matched_words, documents_.at(document_id).status};
}


                        
bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(string_view word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

                        
vector<string> SearchServer::SplitIntoWordsNoStop(string_view text) const {
    vector<string> words;
    for (string_view sv : SplitIntoWords(text)) {
        string word(sv);
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + word + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

                        
int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

                        
SearchServer::QueryWord SearchServer::ParseQueryWord(string_view word) const {
    if (word.empty()) {
        throw invalid_argument("Query word is empty"s);
    }

    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word.remove_prefix(1);
    }
    
    string res_word(word);
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + res_word + " is invalid");
    }
    
    return {res_word, is_minus, IsStopWord(res_word)};
}


SearchServer::Query SearchServer::ParseQuery(string_view text) const {
    
    set<string> set_of_minus_words, set_of_plus_words;
    for (const string_view& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                set_of_minus_words.insert(query_word.data);
            } else {
                set_of_plus_words.insert(query_word.data);
            }
        }
    }
    return {{set_of_plus_words.begin(), set_of_plus_words.end()}, {set_of_minus_words.begin(), set_of_minus_words.end()}};
}



double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}


