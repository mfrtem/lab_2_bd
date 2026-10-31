#ifndef HASHINDEX_H
#define HASHINDEX_H

#include <fstream>
#include <string>
using namespace std;

// состояния ячейки
enum EntryStatus {
    EMPTY = 0,
    OCCUPIED = 1,
    DELETED = 2
};

// ячейка хеш-таблицы
struct HashEntry {
    int id;
    long long offset;
    int status;
};

// класс для работы с хеш-таблицей
class HashIndex {
private:
    fstream file;
    int capacity;
    int count;
    string filename;

    int hashFunction(int id);
    bool rehash();

public:
    bool open(const char* filename);
    void close();

    bool insert(int id, long long offset);
    bool find(int id, long long& offset);
    bool remove(int id);
};

#endif