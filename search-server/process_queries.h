#pragma once

#include "document.h"
#include "search_server.h"
#include <list>

using std::vector, std::string, std::list;

struct Iter {
        vector<Document>::const_iterator it;
        vector<Document>::const_iterator it_end;
        vector<vector<Document>::const_iterator> begins;
        vector<size_t> sizes;
        size_t i = 0, j = 0;
        
        Iter(const vector<vector<Document>>& data) {
            it = data[0].begin();
            it_end = (data.back()).end();
            sizes.reserve(data.size());
            begins.reserve(data.size());
            for (const auto& v: data) {
                sizes.push_back(v.size());
                begins.push_back(v.begin());
            }
        }
        
        Iter(const vector<vector<Document>>& data, bool flag) {
            it = (data.back()).end();
        }
        
        bool operator== (const Iter& rhs) {
            return it == rhs.it;
        }
        
         bool operator!= (const Iter& rhs) {
            return !(it == rhs.it);
        }
        
        auto operator++ () {
            if (i == sizes.size() - 1 && j == sizes[i] - 1) {
                return it_end;
            }
            if (j < sizes[i] - 1) {
                ++it;
            } else {
                j = 0;
                ++i;
                it = begins[i];
            }
            return it;
        }
        
        auto operator * () {
            return *it;
        }
    };

/*int distance(Iter lhs, Iter rhs) {
    int result = 0;
    while (lhs != rhs) {
        ++result;
        ++lhs;
    }
    return result;
}*/

class ResultOfQueries {
public:
    ResultOfQueries(const vector<vector<Document>>& data) : it_begin_(data), it_end_(data, true) {
    }
    
    Iter begin() {
        return it_begin_;
    }
    
    Iter end() {
        return it_end_;
    }
    
private:
    Iter it_begin_, it_end_;
};

/*class ResultOfQueries {
public:
    ResultOfQueries(const vector<vector<Document>& data) : data_(move(data)) {
        it_ = data[0].begin();
        it_begin_ = it_;
        it_end_ = (data.back()).end();
        sizes_.reserve(data.size());
        for (const auto& v: data) {
            sizes_.push_back(v.size());
        }
    }
    
    auto begin() {
        return it_begin_;
    }
    
    auto end() {
        return it_end_;
    }
    
    auto operator++ () {
        if (j_ < sizes_[i_] - 1) {
            ++it_;
        } else {
            ++i_;
            j_ = 0;
            it_ = data_[i_].begin();
        }
        return it_;
    }
    
    auto operator * () {
        return *it_;
    }
    
private:
    vector<vector<Document>> data_;
    vector<Document>::iterator it_, it_begin_, it_end_;
    vector<size_t> sizes_;
    size_t i_ = 0;
    size_t j_ = 0;
}; */

/*class ResultOfQueries {
public:
    ResultOfQueries(const vector<vector<Document>>& data) : data_(data) {
        sizes_.reserve(data_.size());
        for (const auto& v: data_) {
            sizes_.push_back(v.size());
        } 
    }
    
    struct Iter {
        vector<Document> it;
        vector<size_t> sizes;
        size_t k;
        
        auto operator++() {
            if ( 
                ++it_;
            } else {
                ++k_;
                it_ = data_[k_].begin();
            }
            return it_;
        }
    };
    
    auto begin() {
        return data_[0].begin();
    }
    auto end() {
        (data_.back()).end();
    }
    
    auto operator++() {
        if (it_ < (data_[k_].end() - 1)) {
            ++it_;
        } else {
            ++k_;
            it_ = data_[k_].begin();
        }
        return it_;
    }
    
    auto operator * () {
        return *it_;
    }
            
private:
    vector<vector<Document>> data_;
  //  vector<size_t> sizes_;
    vector<Document>::iterator it_ = data_[0].begin();
    size_t k_ = 0;
};*/

vector<vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const vector<string>& queries);

list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const vector<string>& queries);
