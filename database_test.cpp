#include "database.h"

#include <iostream>
#include <cstring>
#include <filesystem>

using namespace std;

int main() {

    filesystem::create_directories("data");

    const string filename = "data/test_library.dat";

    // Очищаем предыдущую тестовую базу и индексы
    filesystem::remove(filename);
    filesystem::remove(filename + ".idx");
    filesystem::remove(filename + ".author.idx");

    Database db;

    if (!db.open(filename)) {
        cout << "Ошибка открытия БД" << endl;
        return 1;
    }

    vector<Book> books;
    Book result{};

    cout << "Проверка пустой БД:" << endl;

    cout << "Поиск автора Rowling: "
         << db.findByAuthor("Rowling", books) << endl;

    cout << "Найдено книг: " << books.size() << endl;

    // Создаем книгу
    Book book{};
    book.id = 101;
    strcpy(book.title, "Harry Potter");
    strcpy(book.author, "Rowling");
    book.year = 1997;
    strcpy(book.genre, "Fantasy");
    book.pages = 400;
    book.isDeleted = false;

    cout << "\nДобавление книги:" << endl;

    cout << "Добавление: "
         << db.addBook(book) << endl;

    cout << "Поиск по ID: "
         << db.findById(101, result) << endl;

    cout << "Поиск по автору: "
         << db.findByAuthor("Rowling", books) << endl;

    cout << "Найдено книг: " << books.size() << endl;

    // Проверяем повторное добавление
    cout << "\nПроверка уникальности ID:" << endl;

    cout << "Повторное добавление: "
         << db.addBook(book) << endl;

    // Удаляем книгу
    cout << "\nУдаление книги:" << endl;

    cout << "Удаление: "
         << db.removeBook(101) << endl;

    cout << "Поиск удаленной книги: "
         << db.findById(101, result) << endl;

    cout << "Повторное удаление: "
         << db.removeBook(101) << endl;

    // Повторно добавляем книгу
    cout << "\nПовторное добавление:" << endl;

    cout << "Добавление ID 101: "
         << db.addBook(book) << endl;

    // Редактируем название
    cout << "\nРедактирование названия:" << endl;

    Book updatedBook = book;
    strcpy(updatedBook.title, "Harry Potter Updated");

    cout << "Редактирование: "
         << db.editBook(101, updatedBook) << endl;

    db.findById(101, result);

    cout << "Новое название: "
         << result.title << endl;

    // Изменяем автора
    cout << "\nРедактирование автора:" << endl;

    strcpy(updatedBook.author, "J.K. Rowling");

    cout << "Редактирование: "
         << db.editBook(101, updatedBook) << endl;

    // Проверяем новый индекс автора
    cout << "\nПроверка индекса авторов:" << endl;

    cout << "Поиск по новому автору: "
         << db.findByAuthor("J.K. Rowling", books) << endl;

    cout << "Найдено книг: " << books.size() << endl;

    bool oldAuthorHasBook = db.findByAuthor("Rowling", books);

    bool oldRecordFound = false;

    for (const Book& b : books) {
        if (b.id == 101) {
            oldRecordFound = true;
        }
    }

    cout << "Старая запись осталась в индексе: "
         << oldRecordFound << endl;

    // Проверяем повторное открытие
    cout << "\nПовторное открытие БД:" << endl;

    db.close();

    cout << "Открытие: "
         << db.open(filename) << endl;

    cout << "Поиск книги после открытия: "
         << db.findById(101, result) << endl;

    cout << "Название после открытия: "
         << result.title << endl;

    cout << "Автор после открытия: "
         << result.author << endl;

    cout << "Поиск по новому автору: "
         << db.findByAuthor("J.K. Rowling", books) << endl;

    cout << "Найдено книг: "
         << books.size() << endl;

    db.close();

    cout << "\nТестирование завершено." << endl;

    return 0;
}