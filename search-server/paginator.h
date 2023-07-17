#pragma once

#include <iostream>
#include <vector>
#include "document.h"

template<typename Iterator>
class Segment {
 public:
  Iterator begin_, end_;
  Segment(Iterator begin, Iterator end) : begin_(begin), end_(end) {}
  Iterator begin() const {
    return begin_;
  }
  Iterator end() const {
    return end_;
  }
};

template<typename Iterator>
class Paginator {
 public:

  int page_size_;
  std::vector<Segment<Iterator>> segmentation_;

  Paginator(Iterator begin, Iterator end, int page_size) : page_size_(page_size) {

    while (true) {
      if (std::distance(begin, end) >= page_size) {
        segmentation_.emplace_back(begin, begin + page_size);
        begin += page_size;
      } else {
        if (std::distance(begin, end) >= 1) {
          segmentation_.emplace_back(begin, end);
        }
        break;
      }
    }
  }

  auto begin() const {
    return segmentation_.begin();
  }

  auto end() const {
    return segmentation_.end();
  }

};

template<typename c>
std::ostream &operator<<(std::ostream &os, const Segment<c> &seg) {
  auto begin = seg.begin();
  auto end = seg.end();
  while (begin != end) {
    os << (*begin);
    ++begin;
  }
  return os;
}

template<typename container, typename iterator>
iterator begin(container &c) {
  return c.begin();
}

template<typename container, typename iterator>
iterator end(container &c) {
  return c.end();
}

template<typename Container>
auto Paginate(const Container &c, size_t page_size) {
  return Paginator(begin(c), end(c), page_size);
}