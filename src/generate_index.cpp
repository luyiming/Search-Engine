/**
 * @file
 * @author  Lu Yiming <luyimingchn@gmail.com>
 * @version 1.0
 * @date 2017-1-4

 * @section DESCRIPTION
 *
 * Generating inverted index files
 * some loading functions are located in util.hpp
 *
 */

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <map>
#include <cstdlib>
#include <algorithm>
#include "util.hpp"
const char* const mapFilePath = "../data/map.txt";

using namespace std;

string docPath = "../data/doc/";
const char* const wordFilePath = "../data/index/word.txt";
const char* const indexFilePath = "../data/index/index.txt";
const char* const indicesFilePath = "../data/index/indices.txt";

/**
 * document item
 * with docID and offsets
 */
struct Item {
    uint32_t id;
    uint32_t count;
    set<uint32_t> offset;
    Item(uint32_t id, uint32_t off) {
        this->id = id;
        this->offset.clear();
        this->offset.insert(off);
        this->count = 1;
    }
    void insert(uint32_t off) {
        this->offset.insert(off);
        count++;
    }
    bool operator < (const Item& rhs) const {
        return this->id < rhs.id;
    }
};

/**
 * for debugging
 */
inline std::ostream& operator << (std::ostream& out, const Item& rhs) {
    return out << "id:" << rhs.id << " count:" << rhs.count << endl;
}

/**
 * generate inverted index file
 *
 * inverted index file format:
 *
 */
void generate_index() {
    map<string, vector<Item>> indices;
    for (auto it = map_doc_url.begin(); it != map_doc_url.end(); it++) {
        uint32_t doc_id = it->first;
        string filePath = docPath + itos(it->first);

        ifstream infile(filePath);
        // get length of file:
        infile.seekg (0, infile.end);
        int length = infile.tellg();
        infile.seekg (0, infile.beg);
        // allocate memory:
        char * buffer = new char [length + 1];
        // read data as a block:
        infile.read(buffer, length);
        buffer[length] = '\0';
        infile.close();

        string document(buffer);
        delete[] buffer;

        vector<cppjieba::Word> docWords;

        cout << "generating inverted indices: " << doc_id << endl;
        jieba.CutForSearch(document, docWords, true);

        for (auto it = docWords.begin(); it != docWords.end(); it++) {
            transform(it->word.begin(), it->word.end(), it->word.begin(), ::tolower);
            if (filter(it->word))
                continue;
            // find word
            auto j = indices.find(it->word);
            if (j != indices.end()) {
                vector<Item> &vec = j->second;
                // find word-docID item
                bool flag = true;
                for (auto it3 = vec.begin(); it3 != vec.end(); it3++) {
                    if (it3->id == doc_id) {
                        it3->insert(it->offset);
                        flag = false;
                        break;
                    }
                }
                if (flag) {
                    j->second.push_back(Item(doc_id, it->offset));
                }
            }
            else {
                indices[it->word] = vector<Item>{Item(doc_id, it->offset)};
            }
        }
    }
    ofstream outfile(indicesFilePath);
    for (auto it = indices.begin(); it != indices.end(); it++) {
        vector<Item> vec = it->second;
        sort(vec.begin(), vec.end());
        outfile << it->first;
        for (auto it2 = vec.begin(); it2 != vec.end(); it2++) {
            outfile << " " << it2->id << " " << it2->count;
            for (auto it3 = it2->offset.begin(); it3 != it2->offset.end(); it3++) {
                outfile << " " << *it3;
            }
            outfile << " $";
        }
        outfile << " #" << endl;
    }
    outfile.close();
    return;
}

/**
 * for debugging, jieba library
 * @method cut_test
 */
void cut_test() {
    cppjieba::Jieba jieba(DICT_PATH,
          HMM_PATH,
          USER_DICT_PATH,
          IDF_PATH,
          STOP_WORD_PATH);
  vector<cppjieba::Word> jiebawords;
  /*
    string s;
    while (cin >> s) {
        5824
        s = "小明硕士,硕士,\n毕业于";
        jieba.CutForSearch(s, jiebawords, true);
        cout << jiebawords << endl;
    }
    */
    ifstream infile("../data/doc/4731");
    // get length of file:
    infile.seekg (0, infile.end);
    int length = infile.tellg();
    infile.seekg (0, infile.beg);
    // allocate memory:
    char * buffer = new char [length + 1];
    // read data as a block:
    infile.read(buffer, length);
    buffer[length] = '\0';
    infile.close();

    string document(buffer);
    delete[] buffer;
    jieba.CutForSearch(document, jiebawords, true);
    cout << jiebawords << endl;
}

int main() {
//    cut_test();

    load_doc_id();
    load_filter_file();
    generate_index();

    return EXIT_SUCCESS;
}
