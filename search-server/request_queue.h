#pragma once
#include "search_server.h"
#include <deque>
class RequestQueue {
 public:
  explicit RequestQueue(const SearchServer& search_server);
  template <typename DocumentPredicate>
  std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
  std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
  std::vector<Document> AddFindRequest(const std::string& raw_query);
  int GetNoResultRequests() const;
 private:
  struct QueryResult {
    QueryResult(bool output): res_(output) {};
    bool res_;
  };
  std::deque<QueryResult> requests_;
  const static int min_in_day_ = 1440;
  int not_found_query = 0;
  const SearchServer& my_search_server;
};


template<typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
  auto res = my_search_server.FindTopDocuments(raw_query,document_predicate);
  if (!res.empty() && requests_.size() == min_in_day_ ) {
    --not_found_query;
    requests_.pop_front();
    requests_.emplace_back(true);
    return res;
  }
  if (requests_.size() == min_in_day_) {
    requests_.pop_front();
    requests_.emplace_back(true);
    return res;
  }
  if (res.empty()) {
    requests_.emplace_back(true);
    ++not_found_query;
    return res;
  }
  requests_.emplace_back(true);
  return res;
}
