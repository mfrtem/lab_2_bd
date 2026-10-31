#include "btree.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <cstdio>

using namespace std;

int main() {
    BPlusTree tree;

    const string filename = "data/test_author.idx";

    // удаляем старый тестовый файл
    remove(filename.c_str());

    // открываем дерево
    if (!tree.open(filename)) {
        cout << "Ошибка открытия дерева" << endl;
        return 1;
    }

    // добавляем 1000 записей
    cout << "Добавление записей:" << endl;

    for (int i = 1; i <= 1000; i++) {
        int id = (i * 37) % 1000 + 1;

        string author = "Author_" + to_string((id - 1) % 20);

        if (!tree.insert(author.c_str(), id)) {
            cout << "Ошибка добавления ID: " << id << endl;
            tree.close();
            return 1;
        }
    }

    cout << "Добавлено: 1000" << endl;

    // проверяем поиск по всем авторам
    cout << "\nПроверка поиска:" << endl;

    vector<int> ids;

    for (int authorNumber = 0; authorNumber < 20; authorNumber++) {
        string author = "Author_" + to_string(authorNumber);

        if (!tree.findByAuthor(author.c_str(), ids)) {
            cout << "Ошибка поиска: " << author << endl;
            tree.close();
            return 1;
        }

        sort(ids.begin(), ids.end());

        vector<int> expected;

        for (int id = 1; id <= 1000; id++) {
            if ((id - 1) % 20 == authorNumber) {
                expected.push_back(id);
            }
        }

        if (ids != expected) {
            cout << "Ошибка: неверные результаты для " << author << endl;
            tree.close();
            return 1;
        }
    }

    cout << "Поиск по всем 20 авторам пройден" << endl;

    // проверяем отсутствие неизвестного автора
    if (tree.findByAuthor("Unknown", ids)) {
        cout << "Ошибка: найден неизвестный автор" << endl;
        tree.close();
        return 1;
    }

    cout << "Проверка отсутствующего автора пройдена" << endl;

    // проверяем повторную вставку
    cout << "\nПроверка уникальности:" << endl;

    cout << "Повторная вставка: "
         << tree.insert("Author_7", 8) << endl;

    tree.close();

    // проверяем повторное открытие
    cout << "\nПовторное открытие:" << endl;

    if (!tree.open(filename)) {
        cout << "Ошибка повторного открытия" << endl;
        return 1;
    }

    if (!tree.findByAuthor("Author_7", ids) || ids.size() != 50) {
        cout << "Ошибка после повторного открытия" << endl;
        tree.close();
        return 1;
    }

    cout << "Данные сохранены" << endl;

    tree.close();

    return 0;
}