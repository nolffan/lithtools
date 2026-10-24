// Renders the model preview to PNGs (run under xvfb-run):
//   abc_preview_shot <out.png> <model.abc> [skin0.dtx [skin1.dtx...]]
//                    [--anim N --time MS] [--yaw DEG --pitch DEG] [--skeleton]
#include "modelview.h"

#include <QApplication>
#include <QCheckBox>
#include <QFile>
#include <QThread>
#include <cstdio>

static QByteArray slurp(const QString& p) {
    QFile f(p);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStringList a = app.arguments().mid(1);
    if (a.size() < 2) { std::printf("usage\n"); return 2; }
    QString out = a.takeFirst(), model = a.takeFirst();
    int anim = -2;
    float t = 0, yaw = 180, pitch = 10;
    bool turn = false, skel = false;
    QStringList skins;
    while (!a.isEmpty()) {
        QString s = a.takeFirst();
        if (s == "--anim") anim = a.takeFirst().toInt();
        else if (s == "--time") t = a.takeFirst().toFloat();
        else if (s == "--yaw") { yaw = a.takeFirst().toFloat(); turn = true; }
        else if (s == "--pitch") { pitch = a.takeFirst().toFloat(); turn = true; }
        else if (s == "--skeleton") skel = true;
        else skins << s;
    }
    ModelView v;
    v.resize(900, 700);
    v.show();
    app.processEvents();
    QString err;
    bool ok = v.loadAbc(slurp(model), model, [&](int i, int) {
        return i < skins.size() ? slurp(skins[i]) : QByteArray();
    }, &err);
    if (!ok) { std::printf("load failed: %s\n", qPrintable(err)); return 1; }
    if (anim != -2) v.setPose(anim, t);
    if (turn) v.setViewAngles(yaw, pitch);
    if (skel)
        for (auto* c : v.findChildren<QCheckBox*>())
            if (c->text() == "Skeleton") c->setChecked(true);
    for (int i = 0; i < 10; ++i) { app.processEvents(); QThread::msleep(20); }
    QImage img = v.grabFrame();
    img.save(out);
    v.grab().save(QString(out).replace(".png", "-widget.png"));
    std::printf("saved %s (%dx%d)\n", qPrintable(out), img.width(), img.height());
    return 0;
}
