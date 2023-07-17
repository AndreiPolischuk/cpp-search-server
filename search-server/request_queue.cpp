#include "request_queue.h"

using namespace std;

RequestQueue::RequestQueue(const SearchServer &search_server) : ss(search_server) {}

vector<Document> RequestQueue::AddFindRequest(const string &raw_query, DocumentStatus status) {
  vector<Document> ans = ss.FindTopDocuments(raw_query, status);
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
vector<Document> RequestQueue::AddFindRequest(const string &raw_query) {
  vector<Document> ans = ss.FindTopDocuments(raw_query);
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

int RequestQueue::GetNoResultRequests() const {
  return no_result;
}
