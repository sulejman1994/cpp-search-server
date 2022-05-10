#pragma once

#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

#include <vector>
#include <string>
#include <string_view>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <utility>
#include <stdexcept>
#include <iterator>
#include <execution>
#include <future>
#include <mutex>

using std::string, std::set, std::vector, std::map, std::tuple, std::invalid_argument, std::min, std::abs, std::any_of, std::out_of_range, std::transform, std::cout, std::endl, std:: unordered_set, std::unordered_map, std::string_view, std::mutex;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;
const unordered_map<string_view, double> empty_map;

class SearchServer {
public:
    template <typename StringContainer>
    SearchServer(const StringContainer& stop_words);
    
    SearchServer(const string& stop_words_text);
    
    SearchServer(string_view stop_words_text);

    void AddDocument(int document_id, const string_view& document, DocumentStatus status, const vector<int>& ratings);

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string_view& raw_query, DocumentPredicate document_predicate) const;
    vector<Document> FindTopDocuments(const string_view& raw_query, DocumentStatus status) const;       
    vector<Document> FindTopDocuments(const string_view& raw_query) const;
    
    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(std::execution::sequenced_policy execution_type, const string_view& raw_query, DocumentPredicate document_predicate) const;
    vector<Document> FindTopDocuments(std::execution::sequenced_policy execution_type, const string_view& raw_query, DocumentStatus status) const;   
    vector<Document> FindTopDocuments(std::execution::sequenced_policy execution_type, const string_view& raw_query) const;
    
    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(std::execution::parallel_policy execution_type, const string_view& raw_query, DocumentPredicate document_predicate) const;
    vector<Document> FindTopDocuments(std::execution::parallel_policy execution_type, const string_view& raw_query, DocumentStatus status) const;   
    vector<Document> FindTopDocuments(std::execution::parallel_policy execution_type, const string_view& raw_query) const;
    
    
    int GetDocumentCount() const;
    
    set<int>::const_iterator begin() const;   
    set<int>::const_iterator end() const;
    
    const unordered_map<string_view, double>& GetWordFrequencies(int document_id) const;
    
    void RemoveDocument(int document_id);
    
    template<typename ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy execution_type, int document_id);

    tuple<vector<string_view>, DocumentStatus> MatchDocument(const string_view& raw_query, int document_id) const;
    
    tuple<vector<string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy execution_type, const string_view& raw_query, int document_id) const;
    
    tuple<vector<string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy execution_type, const string_view& raw_query, int document_id) const;
    
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    set<string> stop_words_;
    unordered_set<string> all_words;
    unordered_map<string_view, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    set<int> document_ids_;
    map<int, unordered_map<string_view, double> > id_to_word_freqs_;

    bool IsStopWord(const string& word) const;
    
    static bool IsValidWord(const string_view& word);

    vector<string> SplitIntoWordsNoStop(string_view text) const;

    static int ComputeAverageRating(const vector<int>& ratings);

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string_view text) const;

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };
    
    struct Query_for_par {
        vector<string> plus_words;
        vector<string> minus_words;
    };

    Query ParseQuery(string_view text) const;
    
    Query_for_par ParseQuery_for_par(const string_view& text) const;
    
    double ComputeWordInverseDocumentFreq(const string& word) const;

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(std::execution::parallel_policy execution_type, const Query_for_par& query, DocumentPredicate document_predicate) const;

};



template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw invalid_argument("Some of stop words are invalid");
    }
}


template <typename DocumentPredicate>
vector<Document> SearchServer::FindTopDocuments(const string_view& raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
            return lhs.rating > rhs.rating;
        } else {
            return lhs.relevance > rhs.relevance;
        }
    });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename DocumentPredicate>
vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy execution_type, const string_view& raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(raw_query, document_predicate);
}

template <typename DocumentPredicate>
vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy execution_type, const string_view& raw_query, DocumentPredicate document_predicate) const {

    const auto query = ParseQuery_for_par(raw_query);

    auto matched_documents = FindAllDocuments(execution_type, query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
            return lhs.rating > rhs.rating;
        } else {
            return lhs.relevance > rhs.relevance;
        }
    });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename DocumentPredicate>
vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {

    map<int, double> document_to_relevance;
    for (const string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }
    

    for (const string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    vector<Document> matched_documents;
    matched_documents.reserve(document_to_relevance.size());
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}

    
template <typename DocumentPredicate>
vector<Document> SearchServer::FindAllDocuments(std::execution::parallel_policy execution_type, const Query_for_par& query, DocumentPredicate document_predicate) const {
    const size_t buckets_count = 3000;
    ConcurrentMap<int, double> document_to_relevance(buckets_count);
    for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(), [&] (const auto& word) {
        if (word_to_document_freqs_.count(word) == 0) {
            return;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
            }
        }
    });
    
    unordered_set<int> deleted_ids;
    for (const string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            deleted_ids.insert(document_id);
        }
    }

    vector<Document> matched_documents;
    matched_documents.reserve(GetDocumentCount());
    for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
        if (deleted_ids.count(document_id)) {
            continue;
        }
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}



template<typename ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy execution_type, int document_id) {
    documents_.erase(document_id);
    document_ids_.erase(document_id);
    
    vector<string> words_to_delete;
    words_to_delete.reserve(all_words.size());
    for(const auto& [word, _]: id_to_word_freqs_[document_id]) {
        words_to_delete.push_back(word);
    }
    
    for_each(execution_type, words_to_delete.begin(), words_to_delete.end(), [this, document_id] (const string& word) {
        word_to_document_freqs_[word].erase(document_id);
    }); 
    
    id_to_word_freqs_.erase(document_id);
}
