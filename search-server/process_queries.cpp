#include "process_queries.h"

#include <algorithm>
#include <numeric>
#include <execution>
#include <vector>
#include <string>

using namespace std;

vector<vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const vector<string>& queries) {
    
    vector<vector<Document>> result(queries.size());
    transform(execution::par, queries.begin(), queries.end(), result.begin(), [&search_server] (const string& query) {
        return search_server.FindTopDocuments(query); 
    }); 
    return result;
}

list<Document> ProcessQueriesJoined(const SearchServer& search_server, const vector<string>& queries) {
    
    vector<vector<Document>> res_of_queries = ProcessQueries(search_server, queries);
    size_t sz = res_of_queries.size();
    vector<list<Document>> lists(sz);
    transform(std::execution::par, res_of_queries.begin(), res_of_queries.end(), lists.begin(), [] (const auto& res_of_query) {
        return list(res_of_query.begin(), res_of_query.end()); 
    });
    
    list<Document> res_list;
    for (int i = 0; i < sz; ++i) {
        res_list.splice(res_list.end(), lists[i]);
    }
    return res_list; 
} 