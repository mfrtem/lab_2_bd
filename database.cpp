#include "database.h"
#include <iostream>
#include <filesystem>
#include <cstring>

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

    // проверяем, существовал ли хеш-индекс
    std::string indexName = filename + ".idx";
    bool hashIndexExists = std::filesystem::exists(indexName);

    // открываем файл хеш-индекса
    if (!index.open(indexName.c_str())) {
        file.close();
        return false;
    }

    // восстанавливаем хеш-индекс, если его не было
    if (!hashIndexExists) {
        file.clear();
        file.seekg(0, std::ios::beg);

        Book book{};
        long long offset = 0;

        while (file.read(reinterpret_cast<char*>(&book), sizeof(Book))) {
            if (!book.isDeleted) {
                if (!index.insert(book.id, offset)) {
                    close();
                    return false;
                }
            }

            offset += sizeof(Book);
        }

        file.clear();
    }

    // проверяем, существовал ли индекс авторов
    std::string authorIndexName = filename + ".author.idx";
    bool authorIndexExists = std::filesystem::exists(authorIndexName);

    // открываем индекс авторов
    if (!authorIndex.open(authorIndexName)) {
        file.close();
        index.close();
        return false;
    }

    // восстанавливаем индекс авторов, если его не было
    if (!authorIndexExists) {
        file.clear();
        file.seekg(0, std::ios::beg);

        Book book{};

        while (file.read(reinterpret_cast<char*>(&book), sizeof(Book))) {
            if (!book.isDeleted) {
                if (!authorIndex.insert(book.author, book.id)) {
                    close();
                    return false;
                }
            }
        }

        file.clear();
    }

    return true;
}

// закрываем файлы
void Database::close() {
    if (file.is_open()) {
        file.close();
    }

    index.close();
    authorIndex.close();
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

    // проверяем наличие пары автор + ID в B+ дереве
    std::vector<int> authorIds;
    bool pairExists = false;

    if (authorIndex.findByAuthor(book.author, authorIds)) {
        for (int id : authorIds) {
            if (id == book.id) {
                pairExists = true;
                break;
            }
        }
    }

    // определяем конец файла
    file.clear();
    file.seekp(0, std::ios::end);

    long long offset = file.tellp();

    // откатываем добавление книги
    auto rollbackAppend = [&]() {
        file.flush();
        file.close();

        std::error_code error;
        std::filesystem::resize_file(filename, offset, error);

        file.clear();
        file.open(filename, std::ios::in | std::ios::out | std::ios::binary);

        return !error && file.is_open();
    };

    // записываем книгу
    file.write(reinterpret_cast<const char*>(&book), sizeof(Book));
    file.flush();

    if (!file.good()) {
        return false;
    }

    // добавляем ID и смещение в хеш-индекс
    if (!index.insert(book.id, offset)) {
        std::cerr << "Ошибка добавления в хеш-индекс" << std::endl;
        rollbackAppend();
        return false;
    }

    // добавляем автора и ID, если такой пары еще нет
    if (!pairExists && !authorIndex.insert(book.author, book.id)) {
        std::cerr << "Ошибка добавления в индекс авторов" << std::endl;
        index.remove(book.id);
        rollbackAppend();
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

// редактируем книгу по ID
bool Database::editBook(int id, const Book& updatedBook) {
    if (!file.is_open() || id <= 0 || updatedBook.id != id) {
        return false;
    }

    long long offset;

    // получаем смещение книги
    if (!index.find(id, offset)) {
        return false;
    }

    Book oldBook{};

    // считываем старую запись
    file.clear();
    file.seekg(offset, std::ios::beg);
    file.read(reinterpret_cast<char*>(&oldBook), sizeof(Book));

    if (!file.good() || oldBook.id != id || oldBook.isDeleted) {
        return false;
    }

    Book newBook = updatedBook;
    newBook.isDeleted = false;

    // проверяем, изменился ли автор
    bool authorChanged =
        strcmp(oldBook.author, newBook.author) != 0;

    // записываем обновлённую книгу
    file.clear();
    file.seekp(offset, std::ios::beg);
    file.write(reinterpret_cast<const char*>(&newBook), sizeof(Book));
    file.flush();

    if (!file.good()) {
        return false;
    }

    // если автор не изменился, перестраивать дерево не нужно
    if (!authorChanged) {
        return true;
    }

    // перестраиваем индекс авторов
    auto rebuildAuthorIndex = [&]() {
        if (!authorIndex.clear()) {
            return false;
        }

        file.clear();
        file.seekg(0, std::ios::beg);

        Book book{};

        while (file.read(reinterpret_cast<char*>(&book), sizeof(Book))) {
            if (!book.isDeleted) {
                if (!authorIndex.insert(book.author, book.id)) {
                    return false;
                }
            }
        }

        file.clear();
        return true;
    };

    // если перестроение не удалось, возвращаем старую запись
    if (!rebuildAuthorIndex()) {
        file.clear();
        file.seekp(offset, std::ios::beg);
        file.write(reinterpret_cast<const char*>(&oldBook), sizeof(Book));
        file.flush();

        rebuildAuthorIndex();
        return false;
    }

    return true;
}

// ищем книги по автору
bool Database::findByAuthor(const std::string& author, std::vector<Book>& results) {
    results.clear();

    if (!file.is_open()) {
        return false;
    }

    std::vector<int> ids;

    // получаем ID книг из B+ дерева
    if (!authorIndex.findByAuthor(author.c_str(), ids)) {
        return false;
    }

    // проверяем найденные записи в основной базе
    for (int id : ids) {
        Book book{};

        if (findById(id, book) && author == book.author) {
            results.push_back(book);
        }
    }

    return !results.empty();
}

// получаем все книги из базы данных
bool Database::getAllBooks(std::vector<Book>& results) {
    if (!file.is_open()) {
        return false;
    }

    results.clear();

    file.clear();
    file.seekg(0, std::ios::beg);

    Book book{};

    while (file.read(reinterpret_cast<char*>(&book), sizeof(Book))) {
        if (!book.isDeleted) {
            results.push_back(book);
        }
    }

    file.clear();

    return true;
}

// ищем книги по названию
bool Database::findByTitle(const std::string& title, std::vector<Book>& results) {
    results.clear();

    if (!file.is_open() || title.empty()) {
        return false;
    }

    file.clear();
    file.seekg(0, std::ios::beg);

    Book book{};

    while (file.read(reinterpret_cast<char*>(&book), sizeof(Book))) {
        if (!book.isDeleted) {
            std::string bookTitle = book.title;

            if (bookTitle.find(title) != std::string::npos) {
                results.push_back(book);
            }
        }
    }

    file.clear();

    return !results.empty();
}