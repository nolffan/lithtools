// Opens a DLL in the string table editor and saves a PNG of the window.
// With a third argument (a scratch copy path), also edits string 1 through
// the table, saves with File > Save and checks the written DLL.
#include "stringtable.h"

#include <lithtech/pe_strings.hpp>

#include <QAction>
#include <QApplication>
#include <QFile>
#include <QPixmap>
#include <QTableWidget>
#include <QTimer>
#include <cstdio>
#include <fstream>
#include <iterator>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    if (argc < 3) return 2;
    int rc = 0;
    StringTableWindow w;
    QString dll = QString::fromLocal8Bit(argv[1]);
    if (argc > 3) {
        QFile::remove(QString::fromLocal8Bit(argv[3]));
        QFile::copy(dll, QString::fromLocal8Bit(argv[3]));
        dll = QString::fromLocal8Bit(argv[3]);
    }
    if (!w.openFile(dll)) return 1;
    w.resize(900, 600);
    w.show();
    QTimer::singleShot(300, [&] {
        if (argc > 3) {
            auto* t = w.findChild<QTableWidget*>();
            t->item(1, 1)->setText(QStringLiteral("EDITED IN THE GUI\\nsecond line"));
            for (QAction* a : w.findChildren<QAction*>())
                if (a->text() == QStringLiteral("&Save")) {
                    a->trigger();
                    break;
                }
            std::ifstream f(argv[3], std::ios::binary);
            std::vector<uint8_t> b((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            auto img = lithtech::pe::Image::parse(b);
            auto s = img.strings(img.string_languages().front());
            bool ok = s.count(1) && s[1] == u"EDITED IN THE GUI\nsecond line";
            std::printf("gui edit+save: %s, %s\n", ok ? "string 1 written" : "FAILED", w.isWindowModified() ? "still modified" : "clean");
            rc = ok && !w.isWindowModified() ? 0 : 1;
        }
        w.grab().save(QString::fromLocal8Bit(argv[2]));
        app.quit();
    });
    app.exec();
    return rc;
}
