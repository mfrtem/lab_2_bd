#include <QApplication>
#include <QLabel>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QLabel window("LibraryDB: наша библиотека!");
    window.resize(400, 200);
    window.setWindowTitle("LibraryDB");

    window.show();

    return app.exec();
}