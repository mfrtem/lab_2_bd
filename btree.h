#ifndef BTREE_H
#define BTREE_H

#include <fstream>
#include <string>
#include <vector>

using namespace std;

// максимальное количество ключей в узле
const int MAX_KEYS = 3;

// ключ дерева: автор и ID книги
struct BTreeKey {
    char author[100];
    int id;
};

// узел B+ дерева
struct BTreeNode {
    int isLeaf;
    int keyCount;

    BTreeKey keys[MAX_KEYS];
    long long children[MAX_KEYS + 1];

    long long nextLeaf;
};

// заголовок файла дерева
struct BTreeHeader {
    long long rootOffset;
    long long nextOffset;
};

// класс для работы с B+ деревом
class BPlusTree {
private:
    fstream file;
    string filename;

    BTreeHeader header;

    bool readNode(long long offset, BTreeNode& node);
    bool writeNode(long long offset, const BTreeNode& node);
    long long allocateNode(bool isLeaf);

    int compareKeys(const BTreeKey& first, const BTreeKey& second);
    bool insertRecursive(long long nodeOffset, const BTreeKey& key,
                     BTreeKey& promotedKey, long long& newChild);

public:
    bool open(const string& filename);
    void close();

    bool insert(const char* author, int id);
    bool findByAuthor(const char* author, vector<int>& ids);
};

#endif