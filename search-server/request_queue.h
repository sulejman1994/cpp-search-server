#pragma once
#include "search_server.h"
#include "document.h"
#include<vector>
#include<deque>

using namespace std;
class RequestQueue {
public:
    explicit RequestQueue( const SearchServer& search_server) :search_server_(search_server), no_results_requests_(0), current_time_(0) {
    }
   
    template <typename DocumentPredicate>
    vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
        const auto result = search_server_.FindTopDocuments(raw_query, document_predicate);
        AddRequest(result.size());
        return result;
    }

    vector<Document> AddFindRequest(const string& raw_query, DocumentStatus status);
     
    vector<Document> AddFindRequest(const string& raw_query);

    int GetNoResultRequests() const;
    
private:
    struct QueryResult {
        uint64_t timestamp;
        int results;
    };
    deque<QueryResult> requests_;
    const SearchServer& search_server_;
    int no_results_requests_;
    uint64_t current_time_;
    const static int min_in_day_ = 1440;
    
    void AddRequest(int results_num) {
        ++current_time_;
        while(!requests_.empty() && current_time_ - requests_.front().timestamp >= min_in_day_) {
            if (requests_.front().results == 0) {
                --no_results_requests_;
            }
            requests_.pop_front();
        }
        
        requests_.push_back({current_time_, results_num});
        if (results_num == 0) {
            ++no_results_requests_;
        }
    }
        
};

