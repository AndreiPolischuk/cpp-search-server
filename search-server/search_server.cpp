#include "search_server.h"

using namespace std;

SearchServer::SearchServer(const std::string_view stop_words_text) {
    for (char ch : stop_words_text) {
        if (iscntrl(ch)) {
            throw invalid_argument("");
        }
    }
    for (auto &n: SplitIntoWords(stop_words_text)) {
        stop_words_.emplace(n.begin(), n.end());
    }

}

SearchServer::SearchServer(const std::string &stop_words_text) : SearchServer(std::string_view(stop_words_text)) {}


void SearchServer::AddDocument(int document_id,
                               const std::string_view document,
                               DocumentStatus status,
                               const std::vector<int> &ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id");
    }

    storage_.emplace_back(document.begin(), document.end());


    const auto words = SplitIntoWordsNoStop(storage_.back());

    const double inv_word_count = 1.0 / words.size();
    for (const auto word: words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }

    map<string_view, double> w_f;
    for (auto w: words) {
        if (!w_f.count(w)) {
            double inv_word_count = (count(words.begin(), words.end(), w) * 1.0) / words.size();
            w_f[w] = inv_word_count;
            word_to_document_freqs_[w][document_id] = inv_word_count;
        }
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status, w_f});
    document_ids_.push_back(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}


std::vector<Document> SearchServer::FindTopDocuments(execution::sequenced_policy, const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
}

std::vector<Document> SearchServer::FindTopDocuments(execution::sequenced_policy, const std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}



std::vector<Document> SearchServer::FindTopDocuments(execution::parallel_policy, const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(execution::par, std::string(raw_query),  [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(execution::parallel_policy, const std::string_view raw_query) const {
    return FindTopDocuments(execution::par, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

vector<int>::const_iterator SearchServer::begin() {
    return document_ids_.begin();
}

vector<int>::const_iterator SearchServer::end() {
    return document_ids_.end();
}

const map<string_view, double> &SearchServer::GetWordFrequencies(int document_id) const {
    if (documents_.count(document_id) > 0) {
        return documents_.at(document_id).document_words_;
    }
    static const map<string_view, double> ans;
    return ans;
}


bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view word) {
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string &text) const {
    std::vector<std::string_view> words;
    for (const auto &word: SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int> &ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating: ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty");
    }
    auto word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word " + std::string(text) + " is invalid");
    }

    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text, bool to_sort) const {
    Query result;

    for (const auto &word: SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    if (to_sort) {
        std::sort(result.plus_words.begin(), result.plus_words.end());
        std::sort(result.minus_words.begin(), result.minus_words.end());
        auto last = std::unique(result.plus_words.begin(), result.plus_words.end());
        result.plus_words.erase(last, result.plus_words.end());
        last = std::unique(result.minus_words.begin(), result.minus_words.end());
        result.minus_words.erase(last, result.minus_words.end());
    }

    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

void AddDocument(SearchServer &ss, int document_id, const std::string &document, DocumentStatus status,
                 const std::vector<int> &ratings) {
    ss.AddDocument(document_id, document, status, ratings);
}

void SearchServer::RemoveDocument(int document_id) {
    for (auto &wrds: documents_[document_id].document_words_) {
        word_to_document_freqs_[wrds.first].erase(document_id);
        if (word_to_document_freqs_[wrds.first].empty()) {
            word_to_document_freqs_.erase(wrds.first);
        }
    }
    document_ids_.erase(lower_bound(document_ids_.begin(), document_ids_.end(), document_id));
    documents_.erase(document_id);

}

void SearchServer::RemoveDocument(std::execution::sequenced_policy, int document_id) {

    for (auto &wrds: documents_[document_id].document_words_) {
        word_to_document_freqs_[wrds.first].erase(document_id);
        if (word_to_document_freqs_[wrds.first].empty()) {
            word_to_document_freqs_.erase(wrds.first);
        }
    }
    document_ids_.erase(lower_bound(document_ids_.begin(), document_ids_.end(), document_id));
    documents_.erase(document_id);


}

void SearchServer::RemoveDocument(std::execution::parallel_policy, int document_id) {


    const std::map<std::string_view, double> &word_freqs = documents_.at(document_id).document_words_;

    std::vector<const std::string_view *> words_to_erase(word_freqs.size());

    std::transform(std::execution::par, word_freqs.begin(), word_freqs.end(),
                   words_to_erase.begin(),
                   [](const auto &words_freq) { return &words_freq.first; });

    std::for_each(std::execution::par, words_to_erase.begin(), words_to_erase.end(),
                  [this, document_id](const auto &word) { word_to_document_freqs_.at(*word).erase(document_id); });


    document_ids_.erase(lower_bound(document_ids_.begin(), document_ids_.end(), document_id));
    documents_.erase(document_id);


}

std::tuple<std::vector<std::string_view>, DocumentStatus>
SearchServer::MatchDocument(std::execution::parallel_policy, const std::string_view raw_query, int document_id) const {


    if (!std::count(document_ids_.begin(), document_ids_.end(), document_id)) {
        throw std::out_of_range("No such document");
    }
    //LOG_DURATION_STREAM("Operation time", cerr);
    const auto query = ParseQuery(raw_query, false);

    std::vector<std::string_view> matched_words;
    matched_words.reserve(query.plus_words.size());

    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
                    [this, document_id](const auto word) {
                        return
                                word_to_document_freqs_.at(word).count(document_id);
                    })) {
        return {vector<string_view>(), documents_.at(document_id).status};
    }

    matched_words.resize(query.plus_words.size());

    auto ll = std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(),
                           matched_words.begin(), [this, document_id](const auto word) {

                return word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id);

            });

    std::sort(execution::par, matched_words.begin(), ll);

    auto last = std::unique(matched_words.begin(), ll);
    matched_words.erase(last, matched_words.end());


    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus>
SearchServer::MatchDocument(std::execution::sequenced_policy, const std::string_view raw_query,
                            int document_id) const {
    return MatchDocument(raw_query, document_id);
}


std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query,
                                                                                      int document_id) const {

    if (!std::count(document_ids_.begin(), document_ids_.end(), document_id)) {
        throw std::out_of_range("No such document");
    }
    //LOG_DURATION_STREAM("Operation time", cerr);
    const auto query = ParseQuery(raw_query);

    std::vector<std::string_view> matched_words;

    if (std::any_of(query.minus_words.begin(), query.minus_words.end(), [this, document_id](const auto &word) {
        return (word_to_document_freqs_.count(word) != 0) && word_to_document_freqs_.at(word).count(document_id);
    })) {
        return {vector<string_view>(), documents_.at(document_id).status};
    }

    for (const auto word: query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return {matched_words, documents_.at(document_id).status};
}


