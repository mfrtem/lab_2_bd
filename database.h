#ifndef DATABASE_H
#define DATABASE_H

#include "book.h"
#include "hashindex.h"
#include <fstream>
#include <string>

class Database {
private:
    std::fstream file;
    HashIndex index;
    std::string filename;

public:
    bool open(const std::string& filename);
    void close();

    bool addBook(const Book& book);
    bool findById(int id, Book& result);
    bool removeBook(int id);
};

#endif