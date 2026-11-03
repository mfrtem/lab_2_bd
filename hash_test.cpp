#include "hashindex.h"

#include <iostream>
#include <filesystem>

using namespace std;

int main() {

    filesystem::remove("data/test_hash.idx");
    filesystem::remove("data/test_hash.idx.tmp");
    filesystem::remove("data/test_hash.idx.bak");

    HashIndex index;

    if (!index.open("data/test_hash.idx")) {
        cout << "Ошибка открытия файла" << endl;
        return 1;
    }

    cout << "Добавление записей:" << endl;

    for (int i = 1; i <= 710; i++) {
        if (!index.insert(i, i * 296)) {
            cout << "Ошибка добавления ID: " << i << endl;
            index.close();
            return 1;
        }
    }

    cout << "Добавлено: 710" << endl;

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

    cout << "\nПроверка уникальности:" << endl;

    if (index.insert(100, 99999)) {
        cout << "Ошибка: повторный ID добавлен" << endl;
        index.close();
        return 1;
    }

    cout << "Повторная вставка отклонена" << endl;

    index.close();

    cout << "\nПовторное открытие:" << endl;

    if (!index.open("data/test_hash.idx")) {
        cout << "Ошибка повторного открытия" << endl;
        return 1;
    }

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