#include "hashindex.h"
#include <iostream>
#include <filesystem>

const int INITIAL_CAPACITY = 1009;

// проверяем, является ли число простым
bool isPrime(int number) {
    if (number < 2) {
        return false;
    }

    for (int i = 2; i * i <= number; i++) {
        if (number % i == 0) {
            return false;
        }
    }

    return true;
}

// находим следующее простое число
int nextPrime(int number) {
    while (!isPrime(number)) {
        number++;
    }

    return number;
}

// открываем или создаем файл
bool HashIndex::open(const char* filename) {
    close();

    this->filename = filename;
    count = 0;

    file.open(filename, ios::in | ios::out | ios::binary);

    // если файла нет, создаем его
    if (!file.is_open()) {
        ofstream createFile(filename, ios::binary);
        createFile.close();

        file.clear();
        file.open(filename, ios::in | ios::out | ios::binary);
    }

    if (!file.is_open()) {
        return false;
    }

    // проверяем размер файла
    file.seekg(0, ios::end);
    streampos fileSize = file.tellg();

    // создаем новую таблицу
    if (fileSize == 0) {
        capacity = INITIAL_CAPACITY;

        file.clear();
        file.seekp(0, ios::beg);
        file.write(reinterpret_cast<const char*>(&capacity), sizeof(int));

        HashEntry entry{};
        entry.status = EMPTY;

        // заполняем все ячейки
        for (int i = 0; i < capacity; i++) {
            file.write(reinterpret_cast<const char*>(&entry), sizeof(HashEntry));
        }

        file.flush();

        if (!file.good()) {
            close();
            return false;
        }
    } else {
        // считываем размер таблицы
        file.clear();
        file.seekg(0, ios::beg);
        file.read(reinterpret_cast<char*>(&capacity), sizeof(int));

        if (!file.good() || capacity <= 0 ||
            fileSize != static_cast<streamoff>(
                sizeof(int) + capacity * sizeof(HashEntry))) {
            close();
            return false;
        }

        // подсчитываем занятые ячейки
        HashEntry entry{};

        for (int i = 0; i < capacity; i++) {
            file.clear();
            file.seekg(sizeof(int) + i * sizeof(HashEntry), ios::beg);
            file.read(reinterpret_cast<char*>(&entry), sizeof(HashEntry));

            if (!file.good()) {
                close();
                return false;
            }

            if (entry.status == OCCUPIED) {
                count++;
            }
        }
    }

    return true;
}

// закрываем файл
void HashIndex::close() {
    if (file.is_open()) {
        file.close();
    }
}

// вычисляем номер ячейки
int HashIndex::hashFunction(int id) {
    return (id % capacity + capacity) % capacity;
}

// увеличиваем размер таблицы
bool HashIndex::rehash() {
    int newCapacity = nextPrime(capacity * 2);

    string tempName = filename + ".tmp";
    string backupName = filename + ".bak";

    fstream tempFile(tempName, ios::in | ios::out | ios::binary | ios::trunc);

    if (!tempFile.is_open()) {
        return false;
    }

    // записываем новый размер таблицы
    tempFile.write(reinterpret_cast<const char*>(&newCapacity), sizeof(int));

    HashEntry emptyEntry{};
    emptyEntry.status = EMPTY;

    // инициализируем новые ячейки
    for (int i = 0; i < newCapacity; i++) {
        tempFile.write(reinterpret_cast<const char*>(&emptyEntry), sizeof(HashEntry));
    }

    // переносим занятые элементы в новую таблицу
    for (int i = 0; i < capacity; i++) {
        HashEntry entry{};

        file.clear();
        file.seekg(sizeof(int) + i * sizeof(HashEntry), ios::beg);
        file.read(reinterpret_cast<char*>(&entry), sizeof(HashEntry));

        if (!file.good()) {
            tempFile.close();
            std::filesystem::remove(tempName);
            return false;
        }

        if (entry.status != OCCUPIED) {
            continue;
        }

        // вычисляем новую позицию
        int pos = entry.id % newCapacity;

        while (true) {
            HashEntry current{};

            tempFile.clear();
            tempFile.seekg(sizeof(int) + pos * sizeof(HashEntry), ios::beg);
            tempFile.read(reinterpret_cast<char*>(&current), sizeof(HashEntry));

            if (!tempFile.good()) {
                tempFile.close();
                std::filesystem::remove(tempName);
                return false;
            }

            if (current.status == EMPTY) {
                break;
            }

            pos = (pos + 1) % newCapacity;
        }

        // записываем элемент
        tempFile.clear();
        tempFile.seekp(sizeof(int) + pos * sizeof(HashEntry), ios::beg);
        tempFile.write(reinterpret_cast<const char*>(&entry), sizeof(HashEntry));

        if (!tempFile.good()) {
            tempFile.close();
            std::filesystem::remove(tempName);
            return false;
        }
    }

    tempFile.flush();

    if (!tempFile.good()) {
        tempFile.close();
        std::filesystem::remove(tempName);
        return false;
    }

    tempFile.close();

    // заменяем старый индекс новым
    close();

    std::error_code error;
    std::filesystem::remove(backupName, error);

    error.clear();
    std::filesystem::rename(filename, backupName, error);

    if (error) {
        file.open(filename, ios::in | ios::out | ios::binary);
        std::filesystem::remove(tempName);
        return false;
    }

    error.clear();
    std::filesystem::rename(tempName, filename, error);

    if (error) {
        std::error_code restoreError;
        std::filesystem::rename(backupName, filename, restoreError);

        file.clear();
        file.open(filename, ios::in | ios::out | ios::binary);

        return false;
    }

    // открываем обновленный индекс
    capacity = newCapacity;

    file.clear();
    file.open(filename, ios::in | ios::out | ios::binary);

    if (!file.is_open()) {
        return false;
    }

    return true;
}

// добавляем элемент в таблицу
bool HashIndex::insert(int id, long long offset) {
    if (!file.is_open() || id <= 0) {
        return false;
    }

    // проверяем уникальность ID
    long long existingOffset;

    if (find(id, existingOffset)) {
        return false;
    }

    // увеличиваем таблицу при заполнении более 70%
    if (static_cast<double>(count + 1) / capacity > 0.7) {
        if (!rehash()) {
            return false;
        }
    }

    int start = hashFunction(id);
    int firstDeleted = -1;

    // ищем свободную ячейку
    for (int i = 0; i < capacity; i++) {
        int pos = (start + i) % capacity;

        HashEntry entry{};

        file.clear();
        file.seekg(sizeof(int) + pos * sizeof(HashEntry), ios::beg);
        file.read(reinterpret_cast<char*>(&entry), sizeof(HashEntry));

        if (!file.good()) {
            return false;
        }

        // запоминаем удаленную ячейку
        if (entry.status == DELETED && firstDeleted == -1) {
            firstDeleted = pos;
        }

        // нашли пустую ячейку
        if (entry.status == EMPTY) {
            int target = (firstDeleted != -1) ? firstDeleted : pos;

            HashEntry newEntry{};
            newEntry.id = id;
            newEntry.offset = offset;
            newEntry.status = OCCUPIED;

            file.clear();
            file.seekp(sizeof(int) + target * sizeof(HashEntry), ios::beg);
            file.write(reinterpret_cast<const char*>(&newEntry), sizeof(HashEntry));
            file.flush();

            if (!file.good()) {
                return false;
            }

            count++;
            return true;
        }
    }

    // используем удаленную ячейку
    if (firstDeleted != -1) {
        HashEntry newEntry{};
        newEntry.id = id;
        newEntry.offset = offset;
        newEntry.status = OCCUPIED;

        file.clear();
        file.seekp(sizeof(int) + firstDeleted * sizeof(HashEntry), ios::beg);
        file.write(reinterpret_cast<const char*>(&newEntry), sizeof(HashEntry));
        file.flush();

        if (!file.good()) {
            return false;
        }

        count++;
        return true;
    }

    return false;
}

// ищем элемент по ID
bool HashIndex::find(int id, long long& offset) {
    if (!file.is_open() || id <= 0) {
        return false;
    }

    int start = hashFunction(id);

    // проверяем ячейки таблицы
    for (int i = 0; i < capacity; i++) {
        int pos = (start + i) % capacity;

        HashEntry entry{};

        file.clear();
        file.seekg(sizeof(int) + pos * sizeof(HashEntry), ios::beg);
        file.read(reinterpret_cast<char*>(&entry), sizeof(HashEntry));

        if (!file.good()) {
            return false;
        }

        // прекращаем поиск
        if (entry.status == EMPTY) {
            return false;
        }

        // проверяем ID
        if (entry.status == OCCUPIED && entry.id == id) {
            offset = entry.offset;
            return true;
        }
    }

    return false;
}

// удаляем элемент по ID
bool HashIndex::remove(int id) {
    if (!file.is_open() || id <= 0) {
        return false;
    }

    int start = hashFunction(id);

    // ищем нужную ячейку
    for (int i = 0; i < capacity; i++) {
        int pos = (start + i) % capacity;

        HashEntry entry{};

        file.clear();
        file.seekg(sizeof(int) + pos * sizeof(HashEntry), ios::beg);
        file.read(reinterpret_cast<char*>(&entry), sizeof(HashEntry));

        if (!file.good()) {
            return false;
        }

        // прекращаем поиск
        if (entry.status == EMPTY) {
            return false;
        }

        // помечаем ячейку как удаленную
        if (entry.status == OCCUPIED && entry.id == id) {
            entry.status = DELETED;

            file.clear();
            file.seekp(sizeof(int) + pos * sizeof(HashEntry), ios::beg);
            file.write(reinterpret_cast<const char*>(&entry), sizeof(HashEntry));
            file.flush();

            if (!file.good()) {
                return false;
            }

            count--;
            return true;
        }
    }

    return false;
} 