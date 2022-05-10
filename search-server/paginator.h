#pragma once

#include <iostream>
#include <set>
#include <vector>
#include <cmath>
#include <algorithm>


template <typename Iterator>
class IteratorRange {
public:
   
    IteratorRange(Iterator begin, Iterator end) {
      begin_ = begin;
      end_ = end;
    }
    Iterator begin() {
       return begin_;
    }
    Iterator end() {
       return end_;
    }
    Iterator size() {
       return distance(begin_, end_);
    }
private:
    Iterator begin_, end_;
};


template <typename Iterator>
ostream& operator << (ostream& out, IteratorRange<Iterator> it_range) {
    for(auto it = it_range.begin(); it != it_range.end(); ++it) {
        out << *it;
    }
    return out;
}

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, int page_size) {
        for (int left = distance(begin, end); left > 0;) {
            const int current_page_size = min(page_size, left);
            const Iterator current_page_end = next(begin, current_page_size);
            pages_.push_back({begin, current_page_end});
            left -= current_page_size;
            begin = current_page_end;
        }
    }
   
    auto begin() const {
       return pages_.begin();
    }
   
    auto end() const {
       return pages_.end();
    }
   
    auto size() const {
       return pages_.size();
    }
   
private:
    vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, int page_size) {
    return Paginator(begin(c), end(c), page_size);
}



