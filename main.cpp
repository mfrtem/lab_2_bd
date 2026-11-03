#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QByteArray>
#include <QStringList>
#include <QAbstractItemView>

#include <vector>
#include <cstring>
#include <filesystem>
#include <fstream>

#include "database.h"

namespace fs = std::filesystem;

bool askYesNo(QWidget* parent, const QString& title, const QString& message) {
    QMessageBox box(parent);
    box.setWindowTitle(title);
    box.setText(message);
    box.setIcon(QMessageBox::Question);

    QPushButton* yesButton = box.addButton("Да", QMessageBox::AcceptRole);
    box.addButton("Нет", QMessageBox::RejectRole);

    box.setDefaultButton(qobject_cast<QPushButton*>(box.button(QMessageBox::No)));
    box.exec();

    return box.clickedButton() == yesButton;
}

bool copyText(char* destination, size_t capacity, const QString& text) {
    QByteArray bytes = text.toUtf8();

    if (bytes.size() >= static_cast<int>(capacity)) {
        return false;
    }

    std::memset(destination, 0, capacity);
    std::memcpy(destination, bytes.constData(), bytes.size());

    return true;
}

bool bookDialog(QWidget* parent, Book& book, bool editing) {
    QDialog dialog(parent);
    dialog.setWindowTitle(editing ? "Редактировать книгу" : "Добавить книгу");

    QFormLayout form(&dialog);

    QSpinBox idInput;
    idInput.setRange(1, 2147483647);
    idInput.setValue(book.id);

    QLineEdit titleInput;
    titleInput.setMaxLength(99);
    titleInput.setText(QString::fromUtf8(book.title));

    QLineEdit authorInput;
    authorInput.setMaxLength(99);
    authorInput.setText(QString::fromUtf8(book.author));

    QSpinBox yearInput;
    yearInput.setRange(0, 3000);
    yearInput.setValue(book.year);

    QLineEdit genreInput;
    genreInput.setMaxLength(49);
    genreInput.setText(QString::fromUtf8(book.genre));

    QSpinBox pagesInput;
    pagesInput.setRange(1, 100000);
    pagesInput.setValue(book.pages > 0 ? book.pages : 1);

    if (editing) {
        idInput.setEnabled(false);
    }

    form.addRow("ID:", &idInput);
    form.addRow("Название:", &titleInput);
    form.addRow("Автор:", &authorInput);
    form.addRow("Год:", &yearInput);
    form.addRow("Жанр:", &genreInput);
    form.addRow("Страницы:", &pagesInput);

    QDialogButtonBox buttons(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel
    );

    buttons.button(QDialogButtonBox::Ok)->setText("Сохранить");
    buttons.button(QDialogButtonBox::Cancel)->setText("Отмена");

    form.addRow(&buttons);

    QObject::connect(&buttons, &QDialogButtonBox::accepted,
                     &dialog, &QDialog::accept);

    QObject::connect(&buttons, &QDialogButtonBox::rejected,
                     &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    if (titleInput.text().trimmed().isEmpty() ||
        authorInput.text().trimmed().isEmpty() ||
        genreInput.text().trimmed().isEmpty()) {

        QMessageBox::warning(
            parent,
            "Ошибка",
            "Заполните все текстовые поля."
        );

        return false;
    }

    Book updated{};

    updated.id = idInput.value();
    updated.year = yearInput.value();
    updated.pages = pagesInput.value();
    updated.isDeleted = false;

    if (!copyText(updated.title, sizeof(updated.title), titleInput.text()) ||
        !copyText(updated.author, sizeof(updated.author), authorInput.text()) ||
        !copyText(updated.genre, sizeof(updated.genre), genreInput.text())) {

        QMessageBox::warning(
            parent,
            "Ошибка",
            "Слишком длинный текст в одном из полей."
        );

        return false;
    }

    book = updated;
    return true;
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("LibraryDB — Библиотека");
    window.resize(1100, 650);

    Database db;
    bool dbOpened = false;
    std::string currentFilename;

    QWidget* centralWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    QLabel* titleLabel = new QLabel("База данных библиотеки");
    titleLabel->setStyleSheet("font-size: 22px; font-weight: bold;");
    mainLayout->addWidget(titleLabel);

    QLabel* statusLabel = new QLabel("База данных не открыта");
    mainLayout->addWidget(statusLabel);

    QHBoxLayout* fileLayout = new QHBoxLayout();

    QPushButton* createButton = new QPushButton("Создать БД");
    QPushButton* openButton = new QPushButton("Открыть БД");
    QPushButton* closeButton = new QPushButton("Закрыть БД");
    QPushButton* backupButton = new QPushButton("Резервная копия");
    QPushButton* restoreButton = new QPushButton("Восстановить");

    fileLayout->addWidget(createButton);
    fileLayout->addWidget(openButton);
    fileLayout->addWidget(closeButton);
    fileLayout->addStretch();
    fileLayout->addWidget(backupButton);
    fileLayout->addWidget(restoreButton);

    mainLayout->addLayout(fileLayout);

    QTableWidget* table = new QTableWidget(0, 6);

    table->setHorizontalHeaderLabels({
        "ID", "Название", "Автор", "Год", "Жанр", "Страницы"
    });

    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    mainLayout->addWidget(table);

    QHBoxLayout* actionsLayout = new QHBoxLayout();

    QPushButton* addButton = new QPushButton("Добавить книгу");
    QPushButton* editButton = new QPushButton("Редактировать");
    QPushButton* deleteButton = new QPushButton("Удалить");
    QPushButton* searchButton = new QPushButton("Поиск");
    QPushButton* showAllButton = new QPushButton("Показать все книги");

    actionsLayout->addWidget(addButton);
    actionsLayout->addWidget(editButton);
    actionsLayout->addWidget(deleteButton);
    actionsLayout->addWidget(searchButton);
    actionsLayout->addWidget(showAllButton);

    mainLayout->addLayout(actionsLayout);

    window.setCentralWidget(centralWidget);

    auto refreshTable = [&]() {
        if (!dbOpened) {
            return;
        }

        std::vector<Book> books;

        if (!db.getAllBooks(books)) {
            QMessageBox::warning(
                &window,
                "Ошибка",
                "Не удалось прочитать базу данных."
            );
            return;
        }

        table->setRowCount(0);

        for (const Book& book : books) {
            int row = table->rowCount();
            table->insertRow(row);

            table->setItem(row, 0,
                new QTableWidgetItem(QString::number(book.id)));

            table->setItem(row, 1,
                new QTableWidgetItem(QString::fromUtf8(book.title)));

            table->setItem(row, 2,
                new QTableWidgetItem(QString::fromUtf8(book.author)));

            table->setItem(row, 3,
                new QTableWidgetItem(QString::number(book.year)));

            table->setItem(row, 4,
                new QTableWidgetItem(QString::fromUtf8(book.genre)));

            table->setItem(row, 5,
                new QTableWidgetItem(QString::number(book.pages)));
        }
    };

    QObject::connect(showAllButton, &QPushButton::clicked, [&]() {
        if (!dbOpened) {
            QMessageBox::warning(
                &window,
                "Ошибка",
                "Сначала откройте базу данных."
            );
            return;
        }

        refreshTable();
        statusLabel->setText("Показаны все книги");
    });

    auto selectedBookId = [&]() -> int {
        int row = table->currentRow();

        if (row < 0) {
            return -1;
        }

        QTableWidgetItem* item = table->item(row, 0);

        if (!item) {
            return -1;
        }

        bool ok = false;
        int id = item->text().toInt(&ok);

        return ok ? id : -1;
    };

    auto openDatabase = [&](const QString& filename) {
        if (filename.isEmpty()) {
            return false;
        }

        if (dbOpened) {
            db.close();
            dbOpened = false;
        }

        if (!db.open(filename.toStdString())) {
            QMessageBox::critical(
                &window,
                "Ошибка",
                "Не удалось открыть базу данных."
            );

            statusLabel->setText("Ошибка открытия базы данных");
            return false;
        }

        currentFilename = filename.toStdString();
        dbOpened = true;

        refreshTable();
        statusLabel->setText("Открыта база: " + filename);

        return true;
    };

    QObject::connect(createButton, &QPushButton::clicked, [&]() {
        QString filename = QFileDialog::getSaveFileName(
            &window,
            "Создать базу данных",
            "",
            "Файлы базы данных (*.dat)"
        );

        if (filename.isEmpty()) {
            return;
        }

        if (!filename.endsWith(".dat")) {
            filename += ".dat";
        }

        if (fs::exists(filename.toStdString())) {
            bool answer = askYesNo(
                &window,
                "Подтверждение",
                "Файл уже существует. Создать новую пустую базу вместо него?"
            );

            if (!answer) {
                return;
            }
        }

        if (dbOpened) {
            db.close();
            dbOpened = false;
        }

        std::error_code error;

        fs::remove(filename.toStdString() + ".idx", error);
        error.clear();

        fs::remove(filename.toStdString() + ".author.idx", error);
        error.clear();

        std::ofstream newFile(
            filename.toStdString(),
            std::ios::binary | std::ios::trunc
        );

        if (!newFile.is_open()) {
            QMessageBox::critical(
                &window,
                "Ошибка",
                "Не удалось создать файл базы данных."
            );
            return;
        }

        newFile.close();
        openDatabase(filename);
    });

    QObject::connect(openButton, &QPushButton::clicked, [&]() {
        QString filename = QFileDialog::getOpenFileName(
            &window,
            "Открыть базу данных",
            "",
            "Файлы базы данных (*.dat);;Все файлы (*)"
        );

        if (!filename.isEmpty()) {
            openDatabase(filename);
        }
    });

    QObject::connect(closeButton, &QPushButton::clicked, [&]() {
        if (dbOpened) {
            db.close();
            dbOpened = false;
            currentFilename.clear();
        }

        table->setRowCount(0);
        statusLabel->setText("База данных закрыта");
    });

    QObject::connect(addButton, &QPushButton::clicked, [&]() {
        if (!dbOpened) {
            QMessageBox::warning(
                &window,
                "Ошибка",
                "Сначала откройте базу данных."
            );
            return;
        }

        Book book{};

        if (!bookDialog(&window, book, false)) {
            return;
        }

        if (!db.addBook(book)) {
            QMessageBox::warning(
                &window,
                "Ошибка",
                "Не удалось добавить книгу. Возможно, такой ID уже существует."
            );
            return;
        }

        refreshTable();
        statusLabel->setText("Книга успешно добавлена");
    });

    QObject::connect(editButton, &QPushButton::clicked, [&]() {
        if (!dbOpened) {
            QMessageBox::warning(
                &window,
                "Ошибка",
                "Сначала откройте базу данных."
            );
            return;
        }

        int id = selectedBookId();

        if (id < 0) {
            QMessageBox::information(
                &window,
                "Редактирование",
                "Сначала выберите книгу в таблице."
            );
            return;
        }

        Book book{};

        if (!db.findById(id, book)) {
            QMessageBox::warning(
                &window,
                "Ошибка",
                "Книга не найдена."
            );
            return;
        }

        if (!bookDialog(&window, book, true)) {
            return;
        }

        if (!db.editBook(id, book)) {
            QMessageBox::warning(
                &window,
                "Ошибка",
                "Не удалось изменить книгу."
            );
            return;
        }

        refreshTable();
        statusLabel->setText("Книга успешно изменена");
    });

    QObject::connect(deleteButton, &QPushButton::clicked, [&]() {
        if (!dbOpened) {
            QMessageBox::warning(
                &window,
                "Ошибка",
                "Сначала откройте базу данных."
            );
            return;
        }

        int id = selectedBookId();

        if (id < 0) {
            QMessageBox::information(
                &window,
                "Удаление",
                "Сначала выберите книгу в таблице."
            );
            return;
        }

        Book book{};

        if (!db.findById(id, book)) {
            QMessageBox::warning(
                &window,
                "Ошибка",
                "Не удалось найти выбранную книгу."
            );
            return;
        }

        QString message =
            "Вы действительно хотите удалить эту книгу?\n\n" +
            QString("Название: ") + QString::fromUtf8(book.title) + "\n" +
            "Автор: " + QString::fromUtf8(book.author) + "\n" +
            "ID: " + QString::number(book.id);

        bool answer = askYesNo(
            &window,
            "Подтверждение удаления",
            message
        );

        if (!answer) {
            return;
        }

        if (!db.removeBook(id)) {
            QMessageBox::warning(
                &window,
                "Ошибка",
                "Не удалось удалить книгу."
            );
            return;
        }

        refreshTable();
        statusLabel->setText("Книга удалена");
    });

    QObject::connect(searchButton, &QPushButton::clicked, [&]() {
        if (!dbOpened) {
            QMessageBox::warning(
                &window,
                "Ошибка",
                "Сначала откройте базу данных."
            );
            return;
        }

        QStringList options;
        options << "По ID" << "По автору" << "По названию";

        bool ok = false;

        QString choice = QInputDialog::getItem(
            &window,
            "Поиск",
            "Выберите способ поиска:",
            options,
            0,
            false,
            &ok
        );

        if (!ok) {
            return;
        }

        std::vector<Book> results;

        if (choice == "По ID") {
            int id = QInputDialog::getInt(
                &window,
                "Поиск книги",
                "Введите ID:",
                1,
                1,
                2147483647,
                1,
                &ok
            );

            if (!ok) {
                return;
            }

            Book book{};

            if (db.findById(id, book)) {
                results.push_back(book);
            }
        }
        else if (choice == "По автору") {
            QString author = QInputDialog::getText(
                &window,
                "Поиск книг",
                "Введите фамилию, имя или часть имени автора:",
                QLineEdit::Normal,
                "",
                &ok
            );

            if (!ok || author.trimmed().isEmpty()) {
                return;
            }

            std::vector<Book> allBooks;
            db.getAllBooks(allBooks);

            QString query = author.trimmed();

            for (const Book& book : allBooks) {
                QString fullAuthor = QString::fromUtf8(book.author);

                if (fullAuthor.contains(query, Qt::CaseInsensitive)) {
                    results.push_back(book);
                }
            }
        }
        else {
            QString title = QInputDialog::getText(
                &window,
                "Поиск книг",
                "Введите название книги или его часть:",
                QLineEdit::Normal,
                "",
                &ok
            );

            if (!ok || title.trimmed().isEmpty()) {
                return;
            }

            std::vector<Book> allBooks;
            db.getAllBooks(allBooks);

            QString query = title.trimmed();

            for (const Book& book : allBooks) {
                QString bookTitle = QString::fromUtf8(book.title);

                if (bookTitle.contains(query, Qt::CaseInsensitive)) {
                    results.push_back(book);
                }
            }
        }

        table->setRowCount(0);

        for (const Book& book : results) {
            int row = table->rowCount();
            table->insertRow(row);

            table->setItem(row, 0,
                new QTableWidgetItem(QString::number(book.id)));

            table->setItem(row, 1,
                new QTableWidgetItem(QString::fromUtf8(book.title)));

            table->setItem(row, 2,
                new QTableWidgetItem(QString::fromUtf8(book.author)));

            table->setItem(row, 3,
                new QTableWidgetItem(QString::number(book.year)));

            table->setItem(row, 4,
                new QTableWidgetItem(QString::fromUtf8(book.genre)));

            table->setItem(row, 5,
                new QTableWidgetItem(QString::number(book.pages)));
        }

        if (results.empty()) {
            QMessageBox::information(
                &window,
                "Поиск",
                "Ничего не найдено."
            );
        }
        else {
            statusLabel->setText(
                "Найдено книг: " + QString::number(results.size())
            );
        }
    });

    QObject::connect(backupButton, &QPushButton::clicked, [&]() {
        if (!dbOpened) {
            QMessageBox::warning(
                &window,
                "Ошибка",
                "Сначала откройте базу данных."
            );
            return;
        }

        QString backupName = QFileDialog::getSaveFileName(
            &window,
            "Создать резервную копию",
            "",
            "Резервная копия (*.bak)"
        );

        if (backupName.isEmpty()) {
            return;
        }

        if (!backupName.endsWith(".bak")) {
            backupName += ".bak";
        }

        db.close();
        dbOpened = false;

        std::error_code error;

        fs::copy_file(
            currentFilename,
            backupName.toStdString(),
            fs::copy_options::overwrite_existing,
            error
        );

        if (!error) {
            fs::copy_file(
                currentFilename + ".idx",
                backupName.toStdString() + ".idx",
                fs::copy_options::overwrite_existing,
                error
            );
        }

        if (!error) {
            fs::copy_file(
                currentFilename + ".author.idx",
                backupName.toStdString() + ".author.idx",
                fs::copy_options::overwrite_existing,
                error
            );
        }

        bool reopened = db.open(currentFilename);
        dbOpened = reopened;

        if (!reopened) {
            QMessageBox::critical(
                &window,
                "Ошибка",
                "Не удалось повторно открыть базу данных."
            );
            return;
        }

        if (error) {
            QMessageBox::critical(
                &window,
                "Ошибка",
                "Не удалось создать резервную копию."
            );
            return;
        }

        refreshTable();
        statusLabel->setText("Резервная копия создана");
    });

    QObject::connect(restoreButton, &QPushButton::clicked, [&]() {
        if (!dbOpened) {
            QMessageBox::warning(
                &window,
                "Ошибка",
                "Сначала откройте базу данных, которую нужно восстановить."
            );
            return;
        }

        QString backupName = QFileDialog::getOpenFileName(
            &window,
            "Выбрать резервную копию",
            "",
            "Резервная копия (*.bak)"
        );

        if (backupName.isEmpty()) {
            return;
        }

        bool answer = askYesNo(
            &window,
            "Подтверждение",
            "Текущая база данных будет заменена резервной копией. Продолжить?"
        );

        if (!answer) {
            return;
        }

        db.close();
        dbOpened = false;

        std::error_code error;

        fs::copy_file(
            backupName.toStdString(),
            currentFilename,
            fs::copy_options::overwrite_existing,
            error
        );

        if (!error) {
            fs::copy_file(
                backupName.toStdString() + ".idx",
                currentFilename + ".idx",
                fs::copy_options::overwrite_existing,
                error
            );
        }

        if (!error) {
            fs::copy_file(
                backupName.toStdString() + ".author.idx",
                currentFilename + ".author.idx",
                fs::copy_options::overwrite_existing,
                error
            );
        }

        if (error) {
            QMessageBox::critical(
                &window,
                "Ошибка",
                "Не удалось восстановить резервную копию."
            );

            openDatabase(QString::fromStdString(currentFilename));
            return;
        }

        if (!openDatabase(QString::fromStdString(currentFilename))) {
            QMessageBox::critical(
                &window,
                "Ошибка",
                "Не удалось открыть восстановленную базу данных."
            );
            return;
        }

        statusLabel->setText("База данных восстановлена");
    });

    window.show();

    return app.exec();
}