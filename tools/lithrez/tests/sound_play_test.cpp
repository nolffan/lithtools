// Drives the sound preview on a real output device (in CI and docker, a
// PulseAudio null sink): playback advances in real time, pause holds the
// position, seeking moves it, looping wraps, stop returns to the start and
// a sound played to the end stops by itself. Optionally saves a screenshot.
//   sound_play_test <file.rez> <path in rez> [screenshot.png]
//   sound_play_test <file.wav> - [screenshot.png]
//   sound_play_test <file.rez|file.wav> <path|-> --through
// --through only plays the sound from start to end (for recording what the
// device receives).
#include "soundview.h"

#include <lithtech/rez.hpp>

#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QTimer>

#include <cmath>
#include <cstdio>
#include <functional>

using namespace lithtech;

static void wait(int ms) {
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

// Waits (up to 3 s) for the device to be playing steadily: 0.15 s on from
// where it started. (A PulseAudio null sink plays a few ms, then stalls for
// most of a second before running in real time.)
static bool waitForStart(SoundView& v) {
    qint64 p = v.position(), want = qint64(v.sampleRate() * 0.15);
    for (int i = 0; i < 300; ++i) {
        wait(10);
        qint64 q = v.position();
        if (q - p >= want || q < p) return true;   // (or it wrapped round a loop)
    }
    return false;
}

static int failures = 0;
static void check(bool ok, const char* what, double got = 0, double want = 0) {
    std::printf("%s %s (got %.3f, want %.3f)\n", ok ? "ok  " : "FAIL", what, got, want);
    if (!ok) ++failures;
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    if (argc < 3) return 2;
    QByteArray bytes;
    if (QString(argv[2]) == "-") {
        QFile f(argv[1]);
        if (!f.open(QIODevice::ReadOnly)) return 2;
        bytes = f.readAll();
    } else {
        auto a = rez::Archive::open(argv[1]);
        std::function<const rez::Resource*(const rez::Directory&)> find = [&](const rez::Directory& d) -> const rez::Resource* {
            for (auto& r : d.resources())
                if (r->path('/') == argv[2]) return r.get();
            for (auto& s : d.dirs())
                if (auto* r = find(*s)) return r;
            return nullptr;
        };
        const rez::Resource* r = find(a->root());
        if (!r) return 2;
        auto v = r->read();
        bytes = QByteArray(reinterpret_cast<const char*>(v.data()), int(v.size()));
    }

    SoundView view;
    view.resize(640, 720);
    view.show();
    QString err;
    if (!view.loadWav(bytes, QString::fromUtf8(argv[2]), &err)) {
        std::printf("FAIL load: %s\n", qPrintable(err));
        return 1;
    }
    if (argc > 3 && QString(argv[3]) == "--through") {
        view.play();
        QElapsedTimer t;
        t.start();
        const qint64 limit = qint64(double(view.frames()) / view.sampleRate() * 1000) + 5000;
        while (view.isPlaying() && t.elapsed() < limit) wait(20);
        if (view.isPlaying()) {
            std::printf("FAIL still playing %lld ms in, at frame %lld of %lld\n", (long long)t.elapsed(),
                        (long long)view.position(), (long long)view.frames());
            return 1;
        }
        return 0;
    }
    view.play();
    check(view.isPlaying(), "playing after play()", view.isPlaying(), 1);
    check(waitForStart(view), "the device starts", 1, 1);
    qint64 p0 = view.position();
    QElapsedTimer clock;
    clock.start();
    wait(1000);
    qint64 p1 = view.position();
    double secs = clock.elapsed() / 1000.0;
    const double played = double(p1 - p0) / secs;   // frames per second, as played
    const double fps = view.sampleRate();
    std::printf("played %lld frames in %.3f s\n", (long long)(p1 - p0), secs);
    check(std::abs(played - fps) < 0.1 * fps, "advances in real time (frames/s)", played, fps);

    view.pause();
    qint64 held = view.position();
    wait(400);
    check(!view.isPlaying() && view.position() == held, "pause holds the position", double(view.position()), double(held));

    view.play();
    wait(300);
    check(view.position() >= held && view.position() < held + qint64(fps * 0.5), "resumes where it paused",
          double(view.position()), double(held));
    view.pause();

    qint64 target = view.frames() / 2;
    view.seek(target);
    check(view.position() == target, "seek while paused", double(view.position()), double(target));
    view.play();
    waitForStart(view);
    wait(500);
    qint64 after = view.position();
    check(after >= target + qint64(fps * 0.4) && after < target + qint64(fps * 0.8), "plays on from the seek",
          double(after), double(target + qint64(fps * 0.5)));
    view.seek(view.frames() / 4);
    wait(300);
    check(view.isPlaying() && view.position() >= view.frames() / 4 + qint64(fps * 0.2) &&
              view.position() < view.frames() / 4 + qint64(fps * 0.5),
          "seek while playing, without a restart", double(view.position()), double(view.frames() / 4 + qint64(fps * 0.3)));

    view.stop();
    check(!view.isPlaying() && view.position() == 0, "stop returns to the start", double(view.position()), 0);

    // To the end: start near it and wait for it to stop by itself.
    double tail = 0.8;
    view.seek(std::max<qint64>(0, view.frames() - qint64(fps * tail)));
    view.play();
    waitForStart(view);
    wait(int(tail * 1000) + 400);
    check(!view.isPlaying() && view.position() == 0, "stops by itself at the end", double(view.isPlaying()), 0);

    // Looping: past the loop's end (the sound's loop region, else its end)
    // it carries on from the loop's start.
    qint64 la = 0, lb = view.frames(), ra, rb;
    bool region = view.loopRegion(ra, rb);
    if (region) la = ra, lb = rb;
    view.setLoop(true);
    view.play();
    waitForStart(view);
    view.seek(std::max<qint64>(la, lb - qint64(fps * 0.3)));
    wait(800);
    check(view.isPlaying() && view.position() >= la && view.position() < la + qint64(fps * 0.8),
          region ? "loops round the loop region" : "loops past the end", double(view.position()), double(la) + fps * 0.5);
    view.stop();
    view.setLoop(false);

    if (argc > 3) {
        view.seek(view.frames() / 3);
        view.grab().save(argv[3]);
    }
    std::printf("%s\n", failures ? "FAILED" : "all passed");
    return failures ? 1 : 0;
}
