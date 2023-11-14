#include "process_queries.h"


std::vector<std::vector<Document>> ProcessQueries(
        const SearchServer& search_server,
        const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> output(queries.size());

    transform(std::execution::par, queries.begin(), queries.end(), output.begin(), [&search_server](const auto& string){return search_server.FindTopDocuments(string);});

    return output;
}

std::vector<Document> ProcessQueriesJoined(
        const SearchServer& search_server,
        const std::vector<std::string>& queries) {

    std::vector<std::vector<Document>> input(queries.size());

    transform(std::execution::par, queries.begin(), queries.end(), input.begin(), [&search_server](const auto& string){return search_server.FindTopDocuments(string);});


    return std::reduce( input.begin(), input.end(), std::vector<Document>(),
                       [](std::vector<Document>& acc, const std::vector<Document>& innerVector) {
                           acc.insert(acc.end(), innerVector.begin(), innerVector.end());
                           return (acc);
                       });


}