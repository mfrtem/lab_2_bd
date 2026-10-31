#include "database.h"
#include <iostream>

// открываем или создаем базу данных
bool Database::open(const std::string& filename) {
    close();

    this->filename = filename;

    // открываем основной файл
    file.open(filename, std::ios::in | std::ios::out | std::ios::binary);

    // если файла нет, создаем его
    if (!file.is_open()) {
        std::ofstream createFile(filename, std::ios::binary);
        createFile.close();

        file.clear();
        file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
    }

    if (!file.is_open()) {
        return false;
    }

    // открываем файл хеш-индекса
    std::string indexName = filename + ".idx";

    if (!index.open(indexName.c_str())) {
        file.close();
        return false;
    }

    return true;
}

// закрываем файлы
void Database::close() {
    if (file.is_open()) {
        file.close();
    }

    index.close();
}

// ищем книгу по ID
bool Database::findById(int id, Book& result) {
    if (!file.is_open()) {
        return false;
    }

    long long offset;

    // получаем смещение из хеш-индекса
    if (!index.find(id, offset)) {
        return false;
    }

    // переходим к записи в основном файле
    file.clear();
    file.seekg(offset, std::ios::beg);

    Book current{};

    file.read(reinterpret_cast<char*>(&current), sizeof(Book));

    if (!file.good() || current.isDeleted || current.id != id) {
        return false;
    }

    result = current;

    return true;
}

// добавляем книгу
bool Database::addBook(const Book& book) {
    if (!file.is_open() || book.id <= 0) {
        return false;
    }

    // проверяем уникальность ID
    Book existing;

    if (findById(book.id, existing)) {
        return false;
    }

    // определяем конец файла
    file.clear();
    file.seekp(0, std::ios::end);

    long long offset = file.tellp();

    // записываем книгу
    file.write(reinterpret_cast<const char*>(&book), sizeof(Book));
    file.flush();

    if (!file.good()) {
        return false;
    }

    // добавляем ID и смещение в индекс
    if (!index.insert(book.id, offset)) {
        std::cerr << "Ошибка добавления в хеш-индекс" << std::endl;
        return false;
    }

    return true;
}

// удаляем книгу по ID
bool Database::removeBook(int id) {
    if (!file.is_open()) {
        return false;
    }

    long long offset;

    // ищем смещение книги
    if (!index.find(id, offset)) {
        return false;
    }

    Book book{};

    // считываем книгу
    file.clear();
    file.seekg(offset, std::ios::beg);
    file.read(reinterpret_cast<char*>(&book), sizeof(Book));

    if (!file.good() || book.id != id || book.isDeleted) {
        return false;
    }

    // помечаем книгу как удаленную
    book.isDeleted = true;

    file.clear();
    file.seekp(offset, std::ios::beg);
    file.write(reinterpret_cast<const char*>(&book), sizeof(Book));
    file.flush();

    if (!file.good()) {
        return false;
    }

    // удаляем ID из индекса
    if (!index.remove(id)) {
        // восстанавливаем запись при ошибке индекса
        book.isDeleted = false;

        file.clear();
        file.seekp(offset, std::ios::beg);
        file.write(reinterpret_cast<const char*>(&book), sizeof(Book));
        file.flush();

        return false;
    }

    return true;
}