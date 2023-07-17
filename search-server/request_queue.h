#pragma once
#include <deque>
#include <vector>
#include <string>
#include "document.h"
#include "search_server.h"

class RequestQueue {
 public:
  explicit RequestQueue(const SearchServer &search_server);

  template<typename DocumentPredicate>
  std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentPredicate document_predicate) {
    std::vector<Document> ans = ss.FindTopDocuments(raw_query, document_predicate);
    bool succes = !ans.empty();
    requests_.emplace_back(succes);
    if (!succes) { ++no_result; }
    if (requests_.size() > min_in_day_) {
      if (!requests_.front().result) {
        --no_result;
      }
      requests_.pop_front();
    }
    return ans;
  }

  std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentStatus status);

  std::vector<Document> AddFindRequest(const std::string &raw_query);

  int GetNoResultRequests() const;

 private:
  struct QueryResult {
    QueryResult(bool b) : result(b) {}
    bool result;
  };
  std::deque<QueryResult> requests_;
  const static int min_in_day_ = 1440;
  int no_result = 0;
  const SearchServer &ss;
  // возможно, здесь вам понадобится что-то ещё
};