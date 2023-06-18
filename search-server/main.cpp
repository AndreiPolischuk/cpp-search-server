#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <optional>
#include <stdexcept>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
  string s;
  getline(cin, s);
  return s;
}

int ReadLineWithNumber() {
  int result;
  cin >> result;
  ReadLine();
  return result;
}

vector<string> SplitIntoWords(const string& text) {
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
  Document() = default;

  Document(int id, double relevance, int rating)
      : id(id)
      , relevance(relevance)
      , rating(rating) {
  }

  int id = 0;
  double relevance = 0.0;
  int rating = 0;
};

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
  set<string> non_empty_strings;
  for (const string& str : strings) {
    if (!str.empty()) {
      non_empty_strings.insert(str);
    }
  }
  return non_empty_strings;
}

enum class DocumentStatus {
  ACTUAL,
  IRRELEVANT,
  BANNED,
  REMOVED,
};

class SearchServer {
 public:

  inline static constexpr int INVALID_DOCUMENT_ID = -1;

  template <typename StringContainer>
  explicit SearchServer(const StringContainer& stop_words)
      : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        for (const auto& word : stop_words_) {
          if (DoesTextContainSpecialSymbols(word)) {
            throw invalid_argument("stop words should not contain special symbols"s);
          }
        }
  }

  explicit SearchServer(const string& stop_words_text)
      : SearchServer(
      SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
  {
    for (const auto& word : stop_words_) {
      if (DoesTextContainSpecialSymbols(word)) {
        throw invalid_argument("stop words should not contain special symbols"s);
      }
    }
  }



  void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
    if (IsDocumentIncorrect(document_id, document)) {
      throw invalid_argument("Document has inappropriate id or contains special symbols in"s);
    }
    const vector<string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const string& word : words) {
      word_to_document_freqs_[word][document_id] += inv_word_count;
    }

    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    number_index_[documents_.size()] = document_id;
  }

  template <typename DocumentPredicate>
  vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {

    const Query query = ParseQuery(raw_query);
    if (IsQueryIncorrect(query)) {
      throw invalid_argument("Query should not contain special symbols, minus words should be wrote correctly"s);
    }
    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(),
         [](const Document& lhs, const Document& rhs) {
           if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
             return lhs.rating > rhs.rating;
           } else {
             return lhs.relevance > rhs.relevance;
           }
         });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
      matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
  }

  vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
          return document_status == status;
        });
  }

  vector<Document> FindTopDocuments(const string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
  }

  int GetDocumentCount() const {
    return documents_.size();
  }


  tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
    const Query query = ParseQuery(raw_query);
    if (IsQueryIncorrect(query)) {
      throw invalid_argument("Query should not contain special symbols, minus words should be wrote correctly"s);
    }
    vector<string> matched_words;
    for (const string& word : query.plus_words) {
      if (word_to_document_freqs_.count(word) == 0) {
        continue;
      }
      if (word_to_document_freqs_.at(word).count(document_id)) {
        matched_words.push_back(word);
      }
    }
    for (const string& word : query.minus_words) {
      if (word_to_document_freqs_.count(word) == 0) {
        continue;
      }
      if (word_to_document_freqs_.at(word).count(document_id)) {
        matched_words.clear();
        break;
      }
    }
    return tuple{matched_words, documents_.at(document_id).status};
  }

  int GetDocumentId(int index) const {
    if (index >= documents_.size() || index < 0) {
      throw out_of_range("Out of range"s);
    }
    return number_index_.at(index + 1);
  }

 private:
  struct DocumentData {
    int rating;
    DocumentStatus status;
  };
  const set<string> stop_words_;
  map<string, map<int, double>> word_to_document_freqs_;
  map<int, DocumentData> documents_;
  map<int, int> number_index_;

  bool IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0;
  }



  vector<string> SplitIntoWordsNoStop(const string& text) const {
    vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
      if (!IsStopWord(word)) {
        words.push_back(word);
      }
    }
    return words;
  }

  static int ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
      return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
      rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
  }

  struct QueryWord {
    string data;
    bool is_minus;
    bool is_stop;
  };

  QueryWord ParseQueryWord(string text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-') {
      is_minus = true;
      text = text.substr(1);
    }
    return {text, is_minus, IsStopWord(text)};
  }

  struct Query {
    set<string> plus_words;
    set<string> minus_words;
  };


  bool IsQueryIncorrect(Query query) const {
    for (const auto& word : query.minus_words) {
      if (word.empty() || word[0] == '-' || word[word.size() - 1] == '-') {
        return true;
      }
      if (DoesTextContainSpecialSymbols(word)) {
        return true;
      }
    }
    for (const auto& word : query.plus_words) {
      if (word.empty() || word[0] == '-' ) {
        return true;
      }
      if (DoesTextContainSpecialSymbols(word)) {
        return true;
      }
    }

    return false;
  }

  Query ParseQuery(const string& text) const {
    Query query;
    for (const string& word : SplitIntoWords(text)) {
      const QueryWord query_word = ParseQueryWord(word);
      if (!query_word.is_stop) {
        if (query_word.is_minus) {
          query.minus_words.insert(query_word.data);
        } else {
          query.plus_words.insert(query_word.data);
        }
      }
    }
    return query;
  }

  bool DoesTextContainSpecialSymbols (const string& text) const {
    for (const char& symbol : text) {
      if (static_cast<int>(symbol) < 32 && static_cast<int>(symbol) >= 0) {
        return true;
      }
    }
    return false;
  }

  bool IsDocumentIncorrect (int id, const string& text) {
    if (DoesTextContainSpecialSymbols(text)) {
      return true;
    }

    if (id < 0) {
      return true;
    }

    if (documents_.count(id)) {
      return true;
    }

    return false;
  }

  // Existence required
  double ComputeWordInverseDocumentFreq(const string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
  }

  template <typename DocumentPredicate>
  vector<Document> FindAllDocuments(const Query& query,
                                    DocumentPredicate document_predicate) const {
    map<int, double> document_to_relevance;
    for (const string& word : query.plus_words) {
      if (word_to_document_freqs_.count(word) == 0) {
        continue;
      }
      const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
      for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
        const auto& document_data = documents_.at(document_id);
        if (document_predicate(document_id, document_data.status, document_data.rating)) {
          document_to_relevance[document_id] += term_freq * inverse_document_freq;
        }
      }
    }

    for (const string& word : query.minus_words) {
      if (word_to_document_freqs_.count(word) == 0) {
        continue;
      }
      for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
        document_to_relevance.erase(document_id);
      }
    }

    vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
      matched_documents.push_back(
          {document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
  }
};

// ==================== для примера =========================

void PrintDocument(const Document& document) {
  cout << "{ "s
       << "document_id = "s << document.id << ", "s
       << "relevance = "s << document.relevance << ", "s
       << "rating = "s << document.rating << " }"s << endl;
}
