#include "hashindex.h"

#include <iostream>

using namespace std;

int main() {

    HashIndex index;

    if (!index.open("data/test_hash.idx")) {

        cout << "Ошибка открытия файла" << endl;

        return 1;

    }

    // добавляем записи

    cout << "Добавление записей:" << endl;

    for (int i = 1; i <= 710; i++) {

        if (!index.insert(i, i * 296)) {

            cout << "Ошибка добавления ID: " << i << endl;

            index.close();

            return 1;

        }

    }

    cout << "Добавлено: 710" << endl;

    // проверяем поиск

    cout << "\nПроверка поиска:" << endl;

    long long offset;

    for (int i = 1; i <= 710; i++) {

        if (!index.find(i, offset) || offset != i * 296) {

            cout << "Ошибка поиска ID: " << i << endl;

            index.close();

            return 1;

        }

    }

    cout << "Все записи найдены" << endl;

    // проверяем повторное добавление

    cout << "\nПроверка уникальности:" << endl;

    cout << index.insert(100, 99999) << endl;

    index.close();

    // проверяем повторное открытие

    cout << "\nПовторное открытие:" << endl;

    if (!index.open("data/test_hash.idx")) {

        cout << "Ошибка повторного открытия" << endl;

        return 1;

    }

    // проверяем сохранность записей

    for (int i = 1; i <= 710; i++) {

        if (!index.find(i, offset) || offset != i * 296) {

            cout << "Ошибка после открытия, ID: " << i << endl;

            index.close();

            return 1;

        }

    }

    cout << "Все записи сохранены" << endl;

    index.close();

    return 0;

}