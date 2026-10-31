#ifndef BOOK_H
#define BOOK_H

struct Book {
    int id; // уникальный идентификатор книги
    char title[100]; // название книги
    char author[100]; // автор книги
    int year; // год издания
    char genre[50]; // жанр книги
    int pages; // количество страниц
    bool isDeleted; // признак удаления книги 
};

#endif 