#pragma once

#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"
#include <stdexcept>
#include <map>
#include <algorithm>
#include <cmath>
#include "log_duration.h"
#include <vector>
#include <execution>
#include <string_view>
#include <list>
#include <mutex>
#include "concurrent_map.h"


const int MAX_RESULT_DOCUMENT_COUNT = 5;
const auto EPSILON = 1e-6;

class SearchServer {
public:
    template<typename StringContainer>
    explicit SearchServer(const StringContainer &stop_words);   // Extract non-empty stop words
    explicit SearchServer(const std::string &stop_words_text);

    explicit SearchServer(const std::string_view stop_words);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status,
                     const std::vector<int> &ratings);


    template<typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query,
                                           DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;


    template<typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, const std::string_view raw_query,
                                           DocumentPredicate document_predicate) const;

    template<typename ExecutionPolicy>
    std::vector<Document>
    FindTopDocuments(ExecutionPolicy policy, const std::string_view raw_query, DocumentStatus status) const;

    template<typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, const std::string_view raw_query) const;


    int GetDocumentCount() const;

    std::vector<int>::const_iterator begin();

    std::vector<int>::const_iterator end();

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query,
                                                                            int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus>
    MatchDocument(std::execution::parallel_policy, const std::string_view raw_query,
                  int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus>
    MatchDocument(std::execution::sequenced_policy, const std::string_view raw_query,
                  int document_id) const;

    const std::map<std::string_view, double> &GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    void RemoveDocument(std::execution::parallel_policy, int document_id);

    void RemoveDocument(std::execution::sequenced_policy, int document_id);


private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::map<std::string_view, double> document_words_;
    };

    std::list<std::string> storage_;
    std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> document_ids_;

    bool IsStopWord(const std::string_view word) const;

    static bool IsValidWord(const std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string &text) const;

    static int ComputeAverageRating(const std::vector<int> &ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };
    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    QueryWord ParseQueryWord(const std::string_view text) const;

    Query ParseQuery(const std::string_view text, bool to_sort = true) const;

    double ComputeWordInverseDocumentFreq(const std::string_view word) const;


    template<typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document>
    FindAllDocuments(ExecutionPolicy policy, const Query &query, DocumentPredicate document_predicate) const;

};

template<typename StringContainer>
SearchServer::SearchServer(const StringContainer &stop_words) {
    for (auto &w: MakeUniqueNonEmptyStrings(stop_words)) {
        stop_words_.insert(w);
    }
    if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid");
    }
}

void AddDocument(SearchServer &ss, int document_id, const std::string_view document, DocumentStatus status,
                 const std::vector<int> &ratings);


template<typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query,
                                                     DocumentPredicate document_predicate) const {
    //LOG_DURATION_STREAM("Operation time", std::cerr);
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);
    std::sort(matched_documents.begin(), matched_documents.end(),
              [](const Document &lhs, const Document &rhs) {
                  if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
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


template<typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy, const std::string_view raw_query,
                                                     DocumentPredicate document_predicate) const {
    //LOG_DURATION_STREAM("Operation time", std::cerr);
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(policy, query, document_predicate);
    std::sort(policy, matched_documents.begin(), matched_documents.end(),
              [](const Document &lhs, const Document &rhs) {
                  if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
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

template<typename ExecutionPolicy>
std::vector<Document>
SearchServer::FindTopDocuments(ExecutionPolicy policy, const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, std::string(raw_query),
                            [status](int document_id, DocumentStatus document_status, int rating) {
                                return document_status == status;
                            });
}

template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy, const std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}


template<typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document>
SearchServer::FindAllDocuments(ExecutionPolicy policy, const Query &query, DocumentPredicate document_predicate) const {


    ConcurrentMap<int, double> document_to_relevance(50);


    std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), [&](auto word) {
        if (word_to_document_freqs_.count(word) == 0) {
            return;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

        for (const auto [document_id, term_freq]: word_to_document_freqs_.at(word)) {
            const auto &document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id].ref_to_value += (term_freq * inverse_document_freq);
            }
        }


    });

    std::for_each(policy, query.minus_words.begin(), query.minus_words.end(), [&](auto &word) {
        if (word_to_document_freqs_.count(word) == 0) {
            return;
        }
        for (const auto [document_id, _]: word_to_document_freqs_.at(word)) {
            document_to_relevance.Erase(document_id);
        }

    });

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance]: document_to_relevance.BuildOrdinaryMap()) {
        matched_documents.emplace_back(document_id, relevance, documents_.at(document_id).rating);
    }
    return matched_documents;
}

