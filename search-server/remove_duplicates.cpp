#include "remove_duplicates.h"

using namespace std;
void RemoveDuplicates(SearchServer& search_server) {
    vector<int> to_delete;
    set<set<string>> collections;
    for (int id: search_server) {
        set<string> coll;
        for (auto& [w, _] : search_server.GetWordFrequencies(id)) {
            coll.insert(w);
        }
        if (collections.count(coll) > 0) {
            to_delete.push_back(id);
        } else {
            collections.insert(coll);
        }
    }
    for (int id : to_delete) {
        search_server.RemoveDocument(id);
        cout << "Found duplicate document id " << id << endl;
    }
}
