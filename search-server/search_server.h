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
#include <type_traits>

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    template <typename StringContainer>
    SearchServer(const StringContainer& stop_words);
    
    SearchServer(const std::string& stop_words_text);
    
    SearchServer(std::string_view stop_words_text);

    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;       
    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;
    
    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(ExecutionPolicy execution_type, std::string_view raw_query, DocumentPredicate document_predicate) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy execution_type, std::string_view raw_query, DocumentStatus status) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy execution_type, std::string_view raw_query) const;
    
    
    int GetDocumentCount() const;
    
    std::set<int>::const_iterator begin() const;   
    std::set<int>::const_iterator end() const;
    
    const std::unordered_map<std::string_view, double>& GetWordFrequencies(int document_id) const;
    
    void RemoveDocument(int document_id);
    
    template<typename ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy execution_type, int document_id);

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;
    
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy execution_type, std::string_view raw_query, int document_id) const;
    
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy execution_type, std::string_view raw_query, int document_id) const;
    
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    std::set<std::string> stop_words_;
    std::unordered_set<std::string> all_words;
    std::unordered_map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    std::map<int, std::unordered_map<std::string_view, double> > id_to_word_freqs_;

    bool IsStopWord(const std::string& word) const;
    
    static bool IsValidWord(std::string_view word);

    std::vector<std::string> SplitIntoWordsNoStop(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::vector<std::string> plus_words;
        std::vector<std::string> minus_words;
    };
    

    Query ParseQuery(std::string_view text) const;
        
    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const; 

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(ExecutionPolicy execution_type, const Query& query, DocumentPredicate document_predicate) const;

};



template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid");
    }
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy execution_type, std::string_view raw_query, DocumentStatus status) const {
        return FindTopDocuments(execution_type, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy execution_type, std::string_view raw_query) const {
        return FindTopDocuments(execution_type, raw_query, DocumentStatus::ACTUAL);
}


template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy execution_type, std::string_view raw_query, DocumentPredicate document_predicate) const {

    const auto query = ParseQuery(raw_query);

    std::vector<Document> matched_documents;
    if constexpr (std::is_same_v<ExecutionPolicy, std::execution::parallel_policy>) {
        matched_documents = FindAllDocuments(std::execution::par, query, document_predicate);
    } else {
         matched_documents = FindAllDocuments(query, document_predicate);
    }
    
    static const double EPSILON = 1e-6;
    std::sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
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
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {

    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words) {
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
    

    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    matched_documents.reserve(document_to_relevance.size());
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
} 

    
template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy execution_type, const Query& query, DocumentPredicate document_predicate) const {

    const size_t buckets_count = 3000;
    ConcurrentMap<int, double> document_to_relevance(buckets_count);
    
    std::for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(), [&] (const auto& word) {
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
    
    const size_t buckets_count_for_deleted_ids = 100;
    ConcurrentMap<int, bool> concurrent_deleted_ids(buckets_count_for_deleted_ids); // используем map как set
    std::for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(), [&] (const auto& word) {
        if (word_to_document_freqs_.count(word) == 0) {
            return;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            concurrent_deleted_ids[document_id];
        }
    });
    
    auto deleted_ids = concurrent_deleted_ids.BuildOrdinaryMap();

    std::vector<Document> matched_documents;
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
    
    std::vector<std::string> words_to_delete;
    words_to_delete.reserve(all_words.size());
    for(const auto& [word, _]: id_to_word_freqs_[document_id]) {
        words_to_delete.push_back(word);
    }
    
    std::for_each(execution_type, words_to_delete.begin(), words_to_delete.end(), [this, document_id] (const std::string& word) {
        word_to_document_freqs_[word].erase(document_id);
    }); 
    
    id_to_word_freqs_.erase(document_id);
}
