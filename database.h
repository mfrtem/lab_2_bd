#ifndef DATABASE_H
#define DATABASE_H

#include "book.h"
#include "hashindex.h"
#include <fstream>
#include <string>
#include "btree.h"
#include <vector>

class Database {
private:
    std::fstream file;
    HashIndex index;
    std::string filename;
    BPlusTree authorIndex;

public:
    bool open(const std::string& filename);
    void close();

    bool addBook(const Book& book);
    bool findById(int id, Book& result);
    bool removeBook(int id);
    bool editBook(int id, const Book& updatedBook);
    bool findByAuthor(const std::string& author, std::vector<Book>& results);
    bool findByTitle(const std::string& title, std::vector<Book>& results);
    bool getAllBooks(std::vector<Book>& results);
};

#endif