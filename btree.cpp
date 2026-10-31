#include <cstring>
#include "btree.h"
#include <iostream>

// открываем или создаём файл дерева
bool BPlusTree::open(const string& filename) {
    close();

    this->filename = filename;

    file.open(filename, ios::in | ios::out | ios::binary);

    // если файла нет, создаём его
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

    // если файл пустой, создаём корень дерева
    if (fileSize == 0) {
        header.rootOffset = sizeof(BTreeHeader);
        header.nextOffset = sizeof(BTreeHeader) + sizeof(BTreeNode);

        BTreeNode root{};
        root.isLeaf = 1;
        root.keyCount = 0;
        root.nextLeaf = -1;

        // записываем заголовок
        file.clear();
        file.seekp(0, ios::beg);
        file.write(reinterpret_cast<const char*>(&header),
                   sizeof(BTreeHeader));

        // записываем корневой узел
        file.write(reinterpret_cast<const char*>(&root),
                   sizeof(BTreeNode));

        file.flush();

        if (!file.good()) {
            close();
            return false;
        }
    } else {
        // читаем заголовок существующего дерева
        file.clear();
        file.seekg(0, ios::beg);
        file.read(reinterpret_cast<char*>(&header),
                  sizeof(BTreeHeader));

        if (!file.good() ||
            header.rootOffset < static_cast<long long>(sizeof(BTreeHeader)) ||
            header.nextOffset < header.rootOffset + static_cast<long long>(sizeof(BTreeNode)) ||
            header.nextOffset > fileSize) {
            close();
            return false;
        }
    }

    return true;
}

// закрываем файл дерева
void BPlusTree::close() {
    if (file.is_open()) {
        file.close();
    }
}

// читаем узел по смещению
bool BPlusTree::readNode(long long offset, BTreeNode& node) {
    if (!file.is_open() || offset < static_cast<long long>(sizeof(BTreeHeader))) {
        return false;
    }

    file.clear();
    file.seekg(offset, ios::beg);
    file.read(reinterpret_cast<char*>(&node), sizeof(BTreeNode));

    return file.good();
}

// записываем узел по смещению
bool BPlusTree::writeNode(long long offset, const BTreeNode& node) {
    if (!file.is_open() || offset < static_cast<long long>(sizeof(BTreeHeader))) {
        return false;
    }

    file.clear();
    file.seekp(offset, ios::beg);
    file.write(reinterpret_cast<const char*>(&node), sizeof(BTreeNode));
    file.flush();

    return file.good();
}

// выделяем место для нового узла
long long BPlusTree::allocateNode(bool isLeaf) {
    if (!file.is_open()) {
        return -1;
    }

    long long offset = header.nextOffset;

    BTreeNode node{};
    node.isLeaf = isLeaf;
    node.keyCount = 0;
    node.nextLeaf = -1;

    // записываем новый узел
    if (!writeNode(offset, node)) {
        return -1;
    }

    // обновляем смещение следующего свободного места
    header.nextOffset += sizeof(BTreeNode);

    file.clear();
    file.seekp(0, ios::beg);
    file.write(reinterpret_cast<const char*>(&header),
               sizeof(BTreeHeader));
    file.flush();

    if (!file.good()) {
        return -1;
    }

    return offset;
}

// сравниваем два ключа
int BPlusTree::compareKeys(const BTreeKey& first, const BTreeKey& second) {
    int result = strcmp(first.author, second.author);

    // если авторы совпадают, сравниваем ID
    if (result == 0) {
        if (first.id < second.id) {
            return -1;
        }

        if (first.id > second.id) {
            return 1;
        }

        return 0;
    }

    return result < 0 ? -1 : 1;
}

// вставляем ключ в дерево
bool BPlusTree::insertRecursive(long long nodeOffset, const BTreeKey& key,
                                BTreeKey& promotedKey, long long& newChild) {
    BTreeNode node{};

    if (!readNode(nodeOffset, node)) {
        return false;
    }

    newChild = -1;

    // если узел является листом
    if (node.isLeaf) {
        int pos = 0;

        // ищем место для нового ключа
        while (pos < node.keyCount &&
               compareKeys(node.keys[pos], key) < 0) {
            pos++;
        }

        // проверяем уникальность ключа
        if (pos < node.keyCount &&
            compareKeys(node.keys[pos], key) == 0) {
            return false;
        }

        // если есть свободное место
        if (node.keyCount < MAX_KEYS) {
            for (int i = node.keyCount; i > pos; i--) {
                node.keys[i] = node.keys[i - 1];
            }

            node.keys[pos] = key;
            node.keyCount++;

            return writeNode(nodeOffset, node);
        }

        // временно размещаем четыре ключа
        BTreeKey temp[MAX_KEYS + 1];

        for (int i = 0, j = 0; i < MAX_KEYS + 1; i++) {
            if (i == pos) {
                temp[i] = key;
            } else {
                temp[i] = node.keys[j++];
            }
        }

        // создаём новый лист
        long long newOffset = allocateNode(true);

        if (newOffset == -1) {
            return false;
        }

        BTreeNode newNode{};
        newNode.isLeaf = 1;
        newNode.keyCount = 2;
        newNode.nextLeaf = node.nextLeaf;

        // распределяем ключи между листьями
        node.keyCount = 2;

        for (int i = 0; i < 2; i++) {
            node.keys[i] = temp[i];
            newNode.keys[i] = temp[i + 2];
        }

        node.nextLeaf = newOffset;

        if (!writeNode(nodeOffset, node) ||
            !writeNode(newOffset, newNode)) {
            return false;
        }

        // передаём родителю первый ключ нового листа
        promotedKey = newNode.keys[0];
        newChild = newOffset;

        return true;
    }

    // если узел внутренний, выбираем дочерний узел
    int pos = 0;

    while (pos < node.keyCount &&
           compareKeys(key, node.keys[pos]) >= 0) {
        pos++;
    }

    BTreeKey childPromoted{};
    long long childNewOffset = -1;

    if (!insertRecursive(node.children[pos], key,
                         childPromoted, childNewOffset)) {
        return false;
    }

    // если дочерний узел не разделился
    if (childNewOffset == -1) {
        return true;
    }

    // временно добавляем новый ключ и указатель
    BTreeKey tempKeys[MAX_KEYS + 1];
    long long tempChildren[MAX_KEYS + 2];

    for (int i = 0; i < pos; i++) {
        tempKeys[i] = node.keys[i];
    }

    tempKeys[pos] = childPromoted;

    for (int i = pos; i < node.keyCount; i++) {
        tempKeys[i + 1] = node.keys[i];
    }

    for (int i = 0; i <= pos; i++) {
        tempChildren[i] = node.children[i];
    }

    tempChildren[pos + 1] = childNewOffset;

    for (int i = pos + 1; i <= node.keyCount; i++) {
        tempChildren[i + 1] = node.children[i];
    }

    // если есть место, сохраняем изменения
    if (node.keyCount < MAX_KEYS) {
        node.keyCount++;

        for (int i = 0; i < node.keyCount; i++) {
            node.keys[i] = tempKeys[i];
        }

        for (int i = 0; i <= node.keyCount; i++) {
            node.children[i] = tempChildren[i];
        }

        return writeNode(nodeOffset, node);
    }

    // разделяем переполненный внутренний узел
    long long newOffset = allocateNode(false);

    if (newOffset == -1) {
        return false;
    }

    BTreeNode newNode{};
    newNode.isLeaf = 0;
    newNode.keyCount = 1;

    promotedKey = tempKeys[2];

    node.keyCount = 2;

    for (int i = 0; i < 2; i++) {
        node.keys[i] = tempKeys[i];
        node.children[i] = tempChildren[i];
    }

    node.children[2] = tempChildren[2];

    newNode.keys[0] = tempKeys[3];
    newNode.children[0] = tempChildren[3];
    newNode.children[1] = tempChildren[4];

    if (!writeNode(nodeOffset, node) ||
        !writeNode(newOffset, newNode)) {
        return false;
    }

    newChild = newOffset;

    return true;
}

// добавляем автора и ID книги в дерево
bool BPlusTree::insert(const char* author, int id) {
    BTreeKey key{};

    if (!file.is_open() || author == nullptr || id <= 0 ||
        strlen(author) >= sizeof(key.author)) {
        return false;
    }
    strcpy(key.author, author);
    key.id = id;

    BTreeKey promotedKey{};
    long long newChild = -1;

    // запускаем рекурсивную вставку
    if (!insertRecursive(header.rootOffset, key,
                         promotedKey, newChild)) {
        return false;
    }

    // если корень не разделился, вставка завершена
    if (newChild == -1) {
        return true;
    }

    // создаём новый корень
    long long newRootOffset = allocateNode(false);

    if (newRootOffset == -1) {
        return false;
    }

    BTreeNode newRoot{};
    newRoot.isLeaf = 0;
    newRoot.keyCount = 1;
    newRoot.keys[0] = promotedKey;
    newRoot.children[0] = header.rootOffset;
    newRoot.children[1] = newChild;

    if (!writeNode(newRootOffset, newRoot)) {
        return false;
    }

    // обновляем адрес корня в заголовке
    header.rootOffset = newRootOffset;

    file.clear();
    file.seekp(0, ios::beg);
    file.write(reinterpret_cast<const char*>(&header),
               sizeof(BTreeHeader));
    file.flush();

    return file.good();
}

// ищем все книги указанного автора
bool BPlusTree::findByAuthor(const char* author, vector<int>& ids) {
    if (!file.is_open() || author == nullptr ||
        strlen(author) >= sizeof(BTreeKey{}.author)) {
        return false;
    }

    ids.clear();

    // формируем ключ для поиска
    BTreeKey key{};
    strcpy(key.author, author);
    key.id = 0;

    // начинаем с корня
    long long currentOffset = header.rootOffset;
    BTreeNode node{};

    // спускаемся до нужного листа
    while (true) {
        if (!readNode(currentOffset, node)) {
            return false;
        }

        if (node.isLeaf) {
            break;
        }

        int pos = 0;

        while (pos < node.keyCount &&
               compareKeys(key, node.keys[pos]) >= 0) {
            pos++;
        }

        currentOffset = node.children[pos];
    }

    // просматриваем листья
    while (currentOffset != -1) {
        if (!readNode(currentOffset, node)) {
            return false;
        }

        for (int i = 0; i < node.keyCount; i++) {
            int result = strcmp(node.keys[i].author, author);

            if (result == 0) {
                ids.push_back(node.keys[i].id);
            } else if (result > 0) {
                return !ids.empty();
            }
        }

        currentOffset = node.nextLeaf;
    }

    return !ids.empty();
}