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
    const double inv_word_count = 1.0 / words.size();
    for (const string &word : words) {
      distributed_index_tf_[word][document_id] = inv_word_count;
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
  map<string, map<int, double>> distributed_index_tf_;
  set<string> stop_words_;

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


  double IDF(const string& word) const {
    return log((document_count_ * 1.0) / distributed_index_tf_.at(word).size());
  }



  vector<Document> FindAllDocuments(const Query &valued_query) const {
    vector<Document> matched_documents;
    map<int, float> processing;
    if (valued_query.plus_words.empty()) {
      return matched_documents;
    }


    //ProcessPlusWords
    for (const string &plus : valued_query.plus_words) {
      if (distributed_index_tf_.count(plus)) {
        double idf = IDF(plus);

        for (auto [id, tf] : distributed_index_tf_.at(plus)) {
          processing[id] += tf * idf;
        }
      }
    }

    //ProcessMinusWords
    for (const string &minus : valued_query.minus_words) {
      if (distributed_index_tf_.count(minus)) {
        for (const auto [id, _] : distributed_index_tf_.at(minus)) {
          processing.erase(id);
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
