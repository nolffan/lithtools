// Renders TextureView, TextureWindow and thumbnails for textures taken from
// a rez to PNGs, for looking at (run with QT_QPA_PLATFORM=offscreen).
//   dtx_view_shots <out dir> <file.rez> <path in rez>...
#include "../gui/textureview.h"

#include <lithtech/rez.hpp>

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QPainter>

using namespace lithtech;

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    if (argc < 4) return 2;
    QDir out(QString::fromLocal8Bit(argv[1]));
    out.mkpath(QStringLiteral("."));
    auto a = rez::Archive::open(argv[2]);
    QList<QImage> thumbs;
    for (int i = 3; i < argc; ++i) {
        auto* r = a->root().find_path(argv[i]);
        if (!r) { std::fprintf(stderr, "%s not found\n", argv[i]); continue; }
        auto v = r->read();
        QByteArray data(reinterpret_cast<const char*>(v.data()), int(v.size()));
        QString name = QString::fromStdString(r->file_name());
        QString stem = QFileInfo(name).completeBaseName();

        TextureView view;
        view.resize(900, 560);
        QString err;
        bool ok = view.loadDtx(data, name, &err);
        view.show();
        app.processEvents();
        view.grab().save(out.filePath(stem + QStringLiteral("_view.png")));
        if (ok) {
            view.setZoom(2);
            view.setAlphaMode(TextureView::AlphaMode::AlphaOnly);
            app.processEvents();
            view.grab().save(out.filePath(stem + QStringLiteral("_view_alpha_2x.png")));
            view.setAlphaMode(TextureView::AlphaMode::Blend);
            view.setFace(2);
            view.setMip(1);
            app.processEvents();
            view.grab().save(out.filePath(stem + QStringLiteral("_view_face2_mip1.png")));
        } else {
            std::fprintf(stderr, "%s: %s\n", argv[i], qPrintable(err));
        }

        TextureWindow w;
        w.resize(900, 600);
        w.openData(data, name);
        w.show();
        app.processEvents();
        w.grab().save(out.filePath(stem + QStringLiteral("_window.png")));

        thumbs << dtxThumbnail(data, 96);
    }
    // A sheet of thumbnails.
    QImage sheet(104 * std::max<qsizetype>(1, thumbs.size()), 104, QImage::Format_ARGB32);
    sheet.fill(Qt::darkGray);
    QPainter p(&sheet);
    for (int i = 0; i < thumbs.size(); ++i)
        if (!thumbs[i].isNull()) p.drawImage(104 * i + 4, 4, thumbs[i]);
    p.end();
    sheet.save(out.filePath(QStringLiteral("thumbnails.png")));
    return 0;
}
