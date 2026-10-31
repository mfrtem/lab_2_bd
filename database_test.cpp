#include "database.h"
#include <iostream>
#include <cstring>
using namespace std;

int main() {
    Database db;

    // открываем тестовую БД
    if (!db.open("data/test_library.dat")) {
        cout << "Ошибка открытия БД" << endl;
        return 1;
    }

    // создаем книгу
    Book book{};
    book.id = 101;
    strcpy(book.title, "Harry Potter");
    strcpy(book.author, "Rowling");
    book.year = 1997;
    strcpy(book.genre, "Fantasy");
    book.pages = 400;
    book.isDeleted = false;

    // добавляем книгу
    cout << "Добавление: " << db.addBook(book) << endl;

    // удаляем книгу
    cout << "Удаление: " << db.removeBook(101) << endl;

    // проверяем поиск удаленной книги
    Book result{};

    cout << "Поиск удаленной книги: "
         << db.findById(101, result) << endl;

    // проверяем повторное удаление
    cout << "Повторное удаление: "
         << db.removeBook(101) << endl;

    // повторно используем ID
    cout << "Повторное добавление ID 101: "
         << db.addBook(book) << endl;

    // проверяем поиск новой записи
    if (db.findById(101, result)) {
        cout << "Найдена книга: " << result.title << endl;
    } else {
        cout << "Книга не найдена" << endl;
    }

    db.close();

    return 0;
}