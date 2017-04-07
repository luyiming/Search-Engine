/**
 * @file
 * @author  Lu Yiming <luyimingchn@gmail.com>
 * @version 1.0
 * @date 2017-1-4

 * @section DESCRIPTION
 *
 * Main searching program
 *
 */

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <set>
#include "util.hpp"
#include "cppjieba/Unicode.hpp"

#define OPTIMIZE
//#define DEBUG
using namespace std;

const char* const wordFilePath = "../data/index/word.txt";
const char* const indexFilePath = "../data/index/index.txt";
const char* const indicesFilePath = "../data/index/indices.txt";
const char* const docFilePath = "../data/doc/";
const char* const pagerankPath = "../data/pagerank.txt";

map<int, double> map_doc_rank;

struct IndicesItem {
    uint32_t docID;
    uint32_t count;
    set<uint32_t> offset;
    IndicesItem(uint32_t docID, uint32_t count) {
        this->docID = docID;
        this->offset.clear();
        this->count = count;
    }
    void update(uint32_t docID, uint32_t count) {
        this->docID = docID;
        this->offset.clear();
        this->count = count;
    }
    void insert(uint32_t off) {
        this->offset.insert(off);
    }
    bool operator < (const IndicesItem& rhs) {
        return this->docID < rhs.docID;
    }
};

map<string, vector<IndicesItem>> indices;

/**
 * load text from document file
 * check utf8 character starting position correctly
 * @method load_text
 * @param  docID     [document id]
 * @param  word      [matched word]
 * @param  offset    [word offset in document]]
 * @param  range     [the range of text around word]
 * @return           [description]
 */
string load_text(uint32_t docID, int start = 0, int end = 512) {
    ifstream infile(string(docFilePath) + itos(docID));
    int length = end - start;

    if (length < 15) {
        length = length + 512;
        start = (start - length / 3 > 0) ? (start - length / 3) : 0;
    }

    char *buffer = new char[length + 1];
    memset(buffer, '\0', length + 1);
    string result = "";
    infile.seekg(start);
    infile.read(buffer, length);

    /**
     * if str is 1 byte ascii character: return 1
     * if str is 3 byte utf8 :
     * 3byte utf8 character:
     *          0xaa 0xbb 0xcc
     *           ^    ^    ^
     *  return:  3    2    2
     */
    int valid_word_start = 0;
    int word_length = cppjieba::DecodeRuneInString(buffer + valid_word_start, 10).len;
    if (word_length == 2) {
        int count = 6;
        do {
            valid_word_start++;
            count--;
            word_length = cppjieba::DecodeRuneInString(buffer + valid_word_start, 10).len;
            if (word_length == 3 || word_length == 1)
                break;
        } while (count != 0);
    }
    int valid_word_end = valid_word_start;
    while (valid_word_end < length - 6) {
        valid_word_end += cppjieba::DecodeRuneInString(buffer + valid_word_end, 10).len;
    }

    buffer[valid_word_end] = '\0';
    result += string(buffer + valid_word_start);


    delete[] buffer;
    infile.close();
    //cout << "length:" << length << " result: " << result << endl;
    return result;
}

struct ResultDocItem {
    uint32_t docID;
    string url;
    string title;
    uint32_t count = 0;
    double pagerank = 1.0;
    bool isTitle = false;
    string text;
    uint32_t offset_left = 0, offset_right = 512;
    // word -> offset, offset, offset ...
    map<string, set<uint32_t> > words;
#ifndef OPTIMIZE
    vector<string> text_list;     // for debug
    vector<uint32_t> text_offset; // for debug
#endif
    ResultDocItem(const ResultDocItem& rhs) {
        this->count = rhs.count;
        this->title = rhs.title;
        this->docID = rhs.docID;
        this->words = rhs.words;
        this->pagerank = rhs.pagerank;
        this->url = rhs.url;
        this->text = rhs.text;
        this->isTitle = rhs.isTitle;
        this->offset_left = rhs.offset_left;
        this->offset_right = rhs.offset_right;
#ifndef OPTIMIZE
        this->text_list = rhs.text_list;
        this->text_offset = rhs.text_offset;
#endif
    }

    ResultDocItem(const string &word, const IndicesItem& rhs) {
        this->count = rhs.count;
        this->docID = rhs.docID;
        words.clear();
        words[word] = rhs.offset;
        this->offset_left = *min_element(rhs.offset.begin(), rhs.offset.end());
        this->offset_right = *max_element(rhs.offset.begin(), rhs.offset.end());
        this->pagerank = map_doc_rank[this->docID];
        this->url = map_doc_url[this->docID];
        this->title = map_doc_title[this->docID];
        if (this->title.find(word) != string::npos)
            this->isTitle = true;
        else
            this->isTitle = false;

    }
    void insert(const string &word, const IndicesItem& rhs) {
        assert(rhs.docID == docID);
        uint32_t tmin = *min_element(rhs.offset.begin(), rhs.offset.end());
        uint32_t tmax = *max_element(rhs.offset.begin(), rhs.offset.end());
        this->offset_left = min(tmin, offset_left);
        this->offset_right= max(tmax, offset_right);
        if (words.find(word) == words.end()) {
            words[word] = rhs.offset;
        }
    }
    void set_text() {
#ifndef OPTIMIZE
        int maxLength = 0;
        for (auto it = words.begin(); it != words.end(); it++) {

            for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
                text_list.push_back(load_text(docID, it->first, *it2));
                text_offset.push_back(*it2);
            }
            if (maxLength < it->first.size()) {
                maxLength = it->first.size();
                text = text_list.back();
            }
        }
#endif
        //cout << "offset_left: " << offset_left << " offset_right: " << offset_right << endl
        //    << text << endl;
        int l = offset_left, r = offset_right;
        if (r - l < 512) {
            r = r - l;
            l = l - 200 > 0 ? l - 200 : 0;
            r = l + 512 + r;
        }
        text = load_text(docID, l, r);
    }
    bool operator < (const ResultDocItem& rhs) const {
        return !(*this > rhs || *this == rhs);
    }
    bool operator > (const ResultDocItem& rhs) const {
        double l = pagerank + 0.0001 * (double)count;
        double r = rhs.pagerank + 0.0001 * (double)rhs.count;
        if (isTitle == true)
            l *= 1.8;
        if (rhs.isTitle == true)
            r *= 1.8;
        if (l == r) {
            return url < rhs.url;
        }
        else
            return l < r;
    }
    bool operator == (const ResultDocItem& rhs) const {
        if (docID == rhs.docID)
            return true;
        else
            return false;
    }
    ResultDocItem operator + (const ResultDocItem& rhs) const {
        assert(docID == rhs.docID);
        ResultDocItem res(rhs);
        res.offset_left = min(offset_left, rhs.offset_left);
        res.offset_right= max(offset_left, rhs.offset_right);
        for (auto it = rhs.words.begin(); it != rhs.words.end(); it++) {
            if (res.words.find(it->first) != words.end()) {
                res.words[it->first].insert(it->second.begin(), it->second.end());
            }
            else {
                res.words[it->first] = it->second;
            }
        }
        return res;
    }
    ResultDocItem& operator = (const ResultDocItem& rhs) {
        if (this != &rhs) {
            this->count = rhs.count;
            this->docID = rhs.docID;
            this->words = rhs.words;
            this->pagerank = rhs.pagerank;
            this->url = rhs.url;
            this->title = rhs.title;
            this->isTitle = rhs.isTitle;
            this->offset_left = rhs.offset_left;
            this->offset_right = rhs.offset_right;
#ifndef OPTIMIZE
            this->text_list = rhs.text_list;
            this->text_offset = rhs.text_offset;
#endif
        }
        return *this;
    }
};

struct Results {
    vector<ResultDocItem> data;
    void insert(const ResultDocItem& rhs) {
        for (uint32_t i = 0; i < data.size(); i ++) {
            if (data[i].docID == rhs.docID) {
                data[i] = data[i] + rhs;
                return;
            }
        }
        data.push_back(rhs);
    }
    void insert(const string &word, const IndicesItem& rhs) {
        for (auto it = data.begin(); it != data.end(); it ++) {
            if (it->docID == rhs.docID) {
                it->insert(word, rhs);
                return;
            }
        }
        data.push_back(ResultDocItem(word, rhs));
    }
    void remove(uint32_t docid) {
        for (auto it = data.begin(); it != data.end(); ) {
            if (it->docID == docid) {
                it = data.erase(it);
            }
            else
                it++;
        }
    }
    void before_use() {
        sort(data.begin(), data.end());
        for (auto it = data.begin(); it != data.end(); it++) {
            it->set_text();
        }
    }
    void output_debug() {
        before_use();

        for (auto it = data.begin(); it != data.end(); it++) {
            cout << it->url << " "
                 << it->count << " "
                 << it->pagerank  << endl;
            cout << it->words << endl;

#ifndef OPTIMIZE
            assert(it->text_list.size() == it->text_offset.size());
            for (uint32_t i = 0; i < it->text_list.size(); i++) {
                cout << "off:" << it->text_offset[i] << " " << it->text_list[i] << " ";
            }
            cout << endl;
#endif
        }

    }

    void output(int maxNum = 50) {
        before_use();
        int count = 0;
        for (auto it = data.begin(); it != data.end(); it++) {
            count++;
            if (count > maxNum) break;
            cout << it->url << endl
                 << it->title << endl
                 << it->pagerank  << endl
                 //<< "offset " << it->offset_left << " " << it->offset_right << endl
                 << it->text  << endl
                 << "#" << endl;
        }

    }

    size_t size() {
        return data.size();
    }

    bool empty() {
        return data.empty();
    }

    Results operation_not(const Results& rhs) const {
        Results res(*this);
        for (auto it = rhs.data.begin(); it != rhs.data.end(); it ++) {
            res.remove(it->docID);
        }
        return res;
    }
    // and operation
    Results operator * (const Results& rhs) const {
        Results res;
        for (auto it = data.begin(); it != data.end(); it ++) {
            for (auto it2 = rhs.data.begin(); it2 != rhs.data.end(); it2++) {
                if (it->docID == it2->docID)
                    res.insert(*it + *it2);
            }
        }
        return res;
    }
    // or operation
    Results operator + (const Results& rhs) const {
        Results res;
        for (auto it = data.begin(); it != data.end(); it ++) {
            res.insert(*it);
        }
        for (auto it = rhs.data.begin(); it != rhs.data.end(); it ++) {
            res.insert(*it);
        }
        return res;
    }
    // or operation
    Results& operator = (const Results& rhs) {
        if (this != &rhs) {
            data = rhs.data;
        }
        return *this;
    }
};

void load_index() {
    ifstream infile(indicesFilePath);
    string word, docID, offset, count;
    infile >> word >> docID >> count;
    auto item = IndicesItem(atoi(docID.c_str()), atoi(count.c_str()));
    while (infile >> offset) {
        if (offset == "$") {
            auto it = indices.find(word);
            if (it != indices.end()) {
                it->second.push_back(item);
            }
            else {
                indices[word] = vector<IndicesItem>{item};
            }
            infile >> offset;
            if (offset == "#") {
                infile >> word >> docID >> count;
                if (infile) {
                    item.update(atoi(docID.c_str()), atoi(count.c_str()));
                }
                else
                    break;
            }
            else {
                docID = offset;
                infile >> count;
                item.update(atoi(docID.c_str()), atoi(count.c_str()));
            }
        }
        else {
            item.insert(atoi(offset.c_str()));
        }
    }
    /*
    for (auto it = indices.begin(); it != indices.end(); it++) {
        vector<IndicesItem> vec = it->second;
        sort(vec.begin(), vec.end());
        cout << it->first;
        for (auto it2 = vec.begin(); it2 != vec.end(); it2++) {
            cout << " " << it2->docID << " " << it2->count;
            for (auto it3 = it2->offset.begin(); it3 != it2->offset.end(); it3++) {
                cout << " " << *it3;
            }
            cout << " $";
        }
        cout << " #" << endl;
    }
    */
    cout << "Load " << indices.size() << " indices" << endl;
    infile.close();
}

void load_pagerank() {
    ifstream infile(pagerankPath);
    string str;
    uint32_t docID;
    double pagerank;
    char line[256];
    infile.getline(line, 256);
    while (true) {
        infile >> str;
        if (str == "===" || str == "$")
            break;
        infile >> docID >> pagerank;
        map_doc_rank[docID] = pagerank;
        //cout << str << " " << docID << " " << pagerank << endl;
    }
    infile.close();
    cout << "Load " << map_doc_rank.size() << " pagerank" << endl;
}


Results get_single_result(string& word) {
    vector<string> words;
    jieba.CutForSearch(word, words);
    Results res;
    for (auto it = words.begin(); it != words.end(); it++) {
        if (filter(*it))
            continue;
        auto it2 = indices.find(*it);
        if (it2 != indices.end()) {
            vector<IndicesItem> &vec = it2->second;
            for (auto it3 = vec.begin(); it3 != vec.end(); it3++) {
                res.insert(*it, *it3);
            }
        }
    }
    return res;
}

struct Token {
    int type;
    string word;
    Results value;
    Token(int type):type(type){}
};

enum {
	NOTYPE = 256, WORD, AND, OR, NOT, NOP
};

static Results v_st[64];
static int op_st[64]; // value_stack, operator_stack
static int v_top = -1, op_top = -1;   // v_st_top, op_st_top

string expression;
Token get_next_token(bool refresh = false) {
    static uint32_t i;
    if (refresh) {
        i = 0;
        while (expression[i] != '\0' && isblank(expression[i]))
            i++;
    }
    while (i < expression.size()) {
        if (expression[i] == '(') {
            i++;
            while (expression[i] != '\0' && isblank(expression[i]))
                i++;
            return Token('(');
        }
        else if (expression[i] == ')') {
            i++;
            if (expression[i] == '\0' || expression[i] == ')' || expression[i] == '-' || expression[i] == '|') {
                return Token(')');
            }
            else if (isblank(expression[i])) {
                while (expression[i] != '\0' && isblank(expression[i]))
                    i++;
                if (expression[i] != '\0' && expression[i] != '-' &&
                    expression[i] != ')' && expression[i] != '|') {
                        i--;
                        expression[i] = ' '; // add extra and operation
                        return Token(')');
                    }
                else {
                    return Token(')');
                }
            }
            else {
                i--;
                expression[i] = ' '; // add extra and operation
                return Token(')');
            }
        }
        else if (isblank(expression[i])) {
            while (expression[i] != '\0' && isblank(expression[i]))
                i++;
            return Token(AND);
        }
        else if (expression[i] == '|') {
            i++;
            while (expression[i] != '\0' && isblank(expression[i]))
                i++;
            return Token(OR);
        }
        else if (expression[i] == '-') {
            i++;
            while (expression[i] != '\0' && isblank(expression[i]))
                i++;
            return Token(NOT);
        }
        else {
            int start = i;
            while (expression[i] != '\0' && expression[i] != '(' && expression[i] != ')' &&
                 !isblank(expression[i]) &&expression[i] != '|' && expression[i] != '-') {
                       i++;
                   }
            Token tk(WORD);
            tk.word = expression.substr(start, i - start);
            tk.value = get_single_result(tk.word);

            if (isblank(expression[i])) {
                while (expression[i] != '\0' && isblank(expression[i]))
                    i++;
                if (expression[i] != '\0' && expression[i] != '-' &&
                    expression[i] != ')' && expression[i] != '|') {
                        i--;
                        expression[i] = ' '; // add extra and operation
                    }
            }
            return tk;
        }
    }
    return Token(NOTYPE);
}

void debug_tokens() {
    Token tk = get_next_token(true);
    switch (tk.type) {
        case WORD: cout << tk.word << " "; break;
        case AND:  cout << "AND "; break;
        case OR:   cout << "OR "; break;
        case NOT: cout << "NOT"; break;
        case '(': cout << "("; break;
        case ')': cout << ")"; break;
        default: cout << endl;
    }
    while (tk.type != NOTYPE) {
        tk = get_next_token();
        switch (tk.type) {
            case WORD: cout << tk.word << " "; break;
            case AND:  cout << "AND "; break;
            case OR:   cout << "OR "; break;
            case NOT: cout << "NOT"; break;
            case '(': cout << "("; break;
            case ')': cout << ")"; break;
            default: cout << endl;
        }
    }
}

static int priority(int tk) {
    switch(tk) {
        case NOP: case NOT:
            return 11;
        case OR: return 5;
        case AND: return 3;
        case NOTYPE: return -1;
        default:  return -1;
    }
}

static int calc_once(int op) {
    switch(op) {
        case NOTYPE:
            break;
        case NOT:
            if(v_top < 1) {
                return 0;
            }
            #ifdef DEBUG
            cout << "NOT" << endl;
            cout << "left word: "  << " " << v_st[v_top-1].size() << endl;
            cout << "right word: "  << " " << v_st[v_top].size() << endl;
            #endif
            v_st[v_top-1] = v_st[v_top-1].operation_not(v_st[v_top]);
            #ifdef DEBUG
            cout << "result word: "  << " " << v_st[v_top-1].size() << endl;
            #endif
            v_top--;
            break;
        case AND:
            if(v_top < 1) {
                return 0;
            }
            #ifdef DEBUG
            cout << "AND" << endl;
            cout << "left word: "  << " " << v_st[v_top-1].size() << endl;
            cout << "right word: "  << " " << v_st[v_top].size() << endl;
            #endif
            v_st[v_top-1] = v_st[v_top-1] * v_st[v_top];
            #ifdef DEBUG
            cout << "result word: "  << " " << v_st[v_top-1].size() << endl;
            #endif
            v_top--;
            break;
        case OR:
            if(v_top < 1) {
                return 0;
            }
            #ifdef DEBUG
            cout << "OR" << endl;
            cout << "left word: "  << " " << v_st[v_top-1].size() << endl;
            cout << "right word: "  << " " << v_st[v_top].size() << endl;
            #endif
            v_st[v_top-1] = v_st[v_top-1] + v_st[v_top];
            #ifdef DEBUG
            cout << "result word: "  << " " << v_st[v_top-1].size() << endl;
            #endif
            v_top--;
            break;
        default: return 0;
    }
    return 1;
}

Results calc() {
    v_top = op_top = -1;
    op_st[++op_top] = NOTYPE;
    Token token = get_next_token(true);
    while (token.type != NOTYPE) {
        if(token.type == WORD) {
            v_st[++v_top] = token.value;
        }
        else if(token.type == '(') {
            op_st[++op_top] = '(';
        }
        else if(token.type == ')') {
            while(op_top > 0 && op_st[op_top] != '(') {
                if(calc_once(op_st[op_top--]) == 0) {
                    break; //ERROR
                }
            }
            if(op_top == 0) { // miss left (
                break; // ERROR Missing left parenthesis
            }
            if(op_top >= 0 && op_st[op_top] == '(') {
                op_top--;
            }
        }
        else {
            if(priority(token.type) > priority(op_st[op_top])) {
                op_st[++op_top] = token.type;
            }
            else {
                while(op_top > 0 && priority(op_st[op_top]) >= priority(token.type)) {
                    if(calc_once(op_st[op_top--]) == 0) {
                        break; //ERROR
                    }
                }
                op_st[++op_top] = token.type;
            }
        }
        token = get_next_token();
    }
    while(op_top > 0) {
        if(calc_once(op_st[op_top--]) == 0) {
            break; //ERROR
        }
    }
    return v_st[0];
}

void search_loop() {
    string str;
    while (getline(cin, str)) {
        expression = str;
        Results results = calc();

        if (results.empty()) {
            cout << 0 << endl << "$" << endl;
        }
        else {
            cout << results.size() << endl;
            cout << "#" << endl;
            //results.before_use();
            #ifndef DEBUG
            results.output();
            #endif
            cout << "$" << endl;
        }
    }
}

int main() {

    load_doc_id();
    load_index();
    load_filter_file();
    load_pagerank();
    cout << "$" << endl;

    search_loop();

    return EXIT_SUCCESS;
}
