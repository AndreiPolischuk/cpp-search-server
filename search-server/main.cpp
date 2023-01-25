#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
  string s;
  getline(cin, s);
  return s;
}

int ReadLineWithNumber() {
  int result = 0;
  cin >> result;
  ReadLine();
  return result;
}

struct Query {
  set<string> plus_words;
  set<string> minus_words;
};

Query MakeValuedQuery(const set<string> &query) {
  Query output;
  for (string word : query) {
    if (word[0] == '-') {

      output.minus_words.insert(word.substr(1));
    } else {
      output.plus_words.insert(word);
    }
  }
  return output;
}

vector<string> SplitIntoWords(const string &text) {
  vector<string> words;
  string word;
  for (const char c : text) {
    if (c == ' ') {
      if (!word.empty()) {
        words.push_back(word);
        word.clear();
      }
    } else {
      word += c;
    }
  }
  if (!word.empty()) {
    words.push_back(word);
  }
  return words;
}

struct Document {
  int id;
  double relevance;
};

class SearchServer {
 public:
  void SetStopWords(const string &text) {
    for (const string &word : SplitIntoWords(text)) {
      stop_words_.insert(word);
    }
  }

  void AddDocument(int document_id, const string &document) {
    vector<string> words = SplitIntoWordsNoStop(document);
    ++document_count_;
    size_of_document_[document_id] = words.size();
    for (const string &word : words) {
      words_and_documents_[word].insert({document_id, (count(words.begin(), words.end(), word))});
    }
  }

  vector<Document> FindTopDocuments(const string &raw_query) const {
    const set<string> query_words = ParseQuery(raw_query);
    Query valued_query = MakeValuedQuery(query_words);


    auto matched_documents = FindAllDocuments(valued_query);

    sort(matched_documents.begin(), matched_documents.end(),
         [](const Document &lhs, const Document &rhs) {
           return lhs.relevance > rhs.relevance;
         });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
      matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
  }

 private:

  int document_count_ = 0;

  map<string, set<pair<int, int>>> words_and_documents_;

  set<string> stop_words_;

  map<int, int> size_of_document_;

  bool IsStopWord(const string &word) const {
    return stop_words_.count(word) > 0;
  }

  vector<string> SplitIntoWordsNoStop(const string &text) const {
    vector<string> words;
    for (const string &word : SplitIntoWords(text)) {
      if (!IsStopWord(word)) {
        words.push_back(word);
      }
    }
    return words;
  }

  set<string> ParseQuery(const string &text) const {
    set<string> query_words;
    for (const string &word : SplitIntoWordsNoStop(text)) {
      query_words.insert(word);
    }
    return query_words;
  }

  vector<Document> FindAllDocuments(const Query &valued_query) const {
    vector<Document> matched_documents;
    map<int, float> processing;
    if (valued_query.plus_words.empty()) {
      return matched_documents;
    }
    float dc = document_count_;

    //ProcessPlusWords
    for (const string &plus : valued_query.plus_words) {
      if (words_and_documents_.count(plus)) {
        double idf = log(dc / words_and_documents_.at(plus).size());
        for (pair<int, float> id_number : words_and_documents_.at(plus)) {
          processing[id_number.first] +=
              idf * (id_number.second / size_of_document_.at(id_number.first));
        }
      }
    }

    //ProcessMinusWords
    for (const string &minus : valued_query.minus_words) {
      if (words_and_documents_.count(minus)) {
        for (pair<int, int> id_number : words_and_documents_.at(minus)) {
          processing.erase(id_number.first);
        }
      }
    }

    for (const auto &doc : processing) {
      matched_documents.push_back({doc.first, doc.second});
    }
    return matched_documents;
  }

};

SearchServer CreateSearchServer() {
  SearchServer search_server;
  search_server.SetStopWords(ReadLine());

  const int document_count = ReadLineWithNumber();
  for (int document_id = 0; document_id < document_count; ++document_id) {
    search_server.AddDocument(document_id, ReadLine());
  }
  return search_server;
}

int main() {
  const SearchServer search_server = CreateSearchServer();
  const string query = ReadLine();
  for (const auto &[document_id, relevance] : search_server.FindTopDocuments(query)) {
    cout << "{ document_id = "s << document_id << ", "
         << "relevance = "s << relevance << " }"s << endl;
  }
}
