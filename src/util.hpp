#ifndef _UTIL_HPP
#define _UTIL_HPP
/**
 * @file
 * @author  Lu Yiming <luyimingchn@gmail.com>
 * @version 1.0
 * @date 2017-1-4

 * @section DESCRIPTION
 *
 * Utilities &&
 * Constant definition
 *
 */

#include <string>
#include <fstream>
#include <iostream>
#include "cppjieba/Jieba.hpp"
#include <sstream>

using namespace std;

/**
 * cppjieba initialization
 */
const char* const DICT_PATH = "deps/cppjieba/dict/jieba.dict.utf8";
const char* const HMM_PATH = "deps/cppjieba/dict/hmm_model.utf8";
const char* const USER_DICT_PATH = "deps/cppjieba/dict/user.dict.utf8";
const char* const IDF_PATH = "deps/cppjieba/dict/idf.utf8";
const char* const STOP_WORD_PATH = "deps/cppjieba/dict/stop_words.utf8";

cppjieba::Jieba jieba(DICT_PATH,
      HMM_PATH,
      USER_DICT_PATH,
      IDF_PATH,
      STOP_WORD_PATH);

/**
 * loading function and constant definition
 */
const char* const filterFilePath = "./filter_words.txt";
const char* const docIDFilePath = "../data/doc/_.map";
map<int, string> map_doc_url;
map<int, string> map_doc_title;
set<string> filterWords;

/**
 * load filter words from filterFilePath
 * store in std::set<string> filterWords
 * @method load_filter_file
 */
void load_filter_file() {
    ifstream infile(filterFilePath);
    string s;
    while (infile >> s) {
        filterWords.insert(s);
    }
    infile.close();
    cout << "Load " << filterWords.size() << " filter words" << endl;
}

/**
 * determine whether the word should be filtered
 * the filter words set is std::set<string> filterWords
 * it is loaded by function load_filter_file()
 * @method filter
 * @param  word   [input word]
 * @return        [true if the word should be filtered]
 */
inline bool filter(string word) {
    int flag = true;
    for (size_t i = 0; i < word.size(); i++) {
        if (!(isalnum(word[i]) || isdigit(word[i]) || isblank(word[i]) || iscntrl(word[i]) || ispunct(word[i]))) {
            flag = false;
            break;
        }
    }
    if (flag)
        return true;
    if (filterWords.find(word) != filterWords.end())
        return true;
    else
        return false;
}


/**
 * load document-ID and the corresponding urls
 * the document-ID is specified in doc/_.map file
 * store in std::map<int, std::string> map_doc_url
 * @method load_doc_id
 */
void load_doc_id() {
    ifstream infile(docIDFilePath);
    int id;
    string str_id, url, title;
    char buf[512];
    while (infile >> id) {
        infile.getline(buf, 512);
        if (buf[0] == ' ')
            url = string(buf + 1);
        else
            url = string(buf);
        infile.getline(buf, 512);
        title = string(buf);
        map_doc_url[id] = url;
        map_doc_title[id] = title;
    }
    infile.close();
    cout << "Load " << map_doc_url.size() << " doc id" << endl;
}


/**
 * convert integer to std::string
 * @method itos
 * @param  num   [input umber]
 * @param  radix [radix]
 * @return       [the result std::string]
 */
std::string itos(int num, int radix = 10) {
    int length = 1, t = num;
    while (t) {
        length++;
        t /= radix;
    }
    char *str = new char[length];
    char index[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    unsigned unum;
    int i = 0, j, k;
    if (radix == 10 && num < 0) {
        unum = (unsigned)-num;
        str[i++] = '-';
    }
    else
        unum = (unsigned)num;
    do {
        str[i++] = index[unum % (unsigned)radix];
        unum /= radix;
    } while (unum);
    str[i] = '\0';
    if (str[0] == '-')
        k = 1; // negative integer
    else
        k = 0;
    char temp;
    for(j = k; j <= (i - k - 1) / 2.0; j++) {
        temp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = temp;
    }
    string res(str);
    delete[] str;
    return res;
}

/**
 * split std::string by delimiter
 * @method split
 * @param  s         [string]
 * @param  delim     [delimiter]
 * @param  skipEmpty [skip empty string]
 * @return           [result string vectors]
 */
vector<string> split(const std::string &s, char delim = ' ', bool skipEmpty = true) {
    std::vector<std::string> elems;
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        if (skipEmpty && item.empty()) {
            continue;
        }
        elems.push_back(item);
    }
    return elems;
}

#endif // _UTIL_HPP
