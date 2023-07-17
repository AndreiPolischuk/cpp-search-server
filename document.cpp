#include "document.h"
using namespace std;

Document::Document(int id, double relevance, int rating) : id(id), relevance(relevance), rating(rating) {}

std::ostream &operator<<(std::ostream &os, const Document &seg) {
  os << "{ document_id = " << seg.id << ", relevance = " << seg.relevance << ", rating = " << seg.rating << " }";
  return os;
}