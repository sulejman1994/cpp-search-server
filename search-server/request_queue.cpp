#include "request_queue.h"

using namespace std;

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    const auto result = search_server_.FindTopDocuments(raw_query, status);
    AddRequest(result.size());
    return result;
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query) { 
    const auto result = search_server_.FindTopDocuments(raw_query);
    AddRequest(result.size());
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    return no_results_requests_;
}

void RequestQueue::AddRequest(int results_num) {
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
