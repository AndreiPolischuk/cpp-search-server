#pragma once
#include "search_server.h"
template<typename Iterator>
class IteratorRange {
 private:
  Iterator begin_;
  Iterator end_;
 public:
  IteratorRange(Iterator begin, Iterator end) {
    begin_ = begin;
    end_ = end;
  }
  Iterator begin() const {
    return begin_;
  }
  Iterator begin() {
    return begin_;
  }
  Iterator end() const {
    return end_;
  }
  Iterator end() {
    return end_;
  }
};
template<typename Iterator>
class Paginator {
 private:
  std::vector<IteratorRange<Iterator>> pages_;
 public:
  Paginator(Iterator begin, Iterator end, int size) {
    int len = static_cast<int> (distance(begin, end ));
    int new_size = std::min(len, size);
    for (auto it = begin; it < end; advance(it, new_size)) {
      IteratorRange<Iterator> tmp(it, it + new_size);
      pages_.push_back(tmp);
    }
  }
  auto size() const {
    return pages_.size();
  }
  auto begin() const {
    return pages_.begin();
  }
  auto begin() {
    return pages_.begin();
  }
  auto end() const {
    return pages_.end();
  }
  auto end() {
    return pages_.end();
  }

};

template<typename Container>
auto Paginate(const Container& c, size_t page_size) {
  return Paginator(begin(c), end(c), page_size);
}
template<typename Iterator>
std::ostream& operator<<(std::ostream& out, const Paginator<Iterator>& p) {
  for (auto it = p.begin(); distance(it, p.end()) > 0; advance(p)) {
    out << *it;
  }
  return out;
}
template<typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& range) {
  for (Iterator currentIt = range.begin(); currentIt < range.end(); ++currentIt) {
    out << *currentIt;
  }
  return out;
}
template<typename Iterator>
IteratorRange<Iterator>& operator*(const IteratorRange<Iterator>& it) {
  return it;
}
