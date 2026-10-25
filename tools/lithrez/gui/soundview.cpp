#include "soundview.h"

#include <lithtech/sound.hpp>

#include <QAudioDevice>
#include <QAudioSink>
#include <QCheckBox>
#include <QCoreApplication>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QIODevice>
#include <QLabel>
#include <QMediaDevices>
#include <QMouseEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QSettings>
#include <QShortcut>
#include <QSlider>
#include <QSplitter>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <functional>

using namespace lithtech;

namespace {

QString timeText(double s) {
    s = std::max(0.0, s);
    int m = int(s / 60);
    return QStringLiteral("%1:%2").arg(m).arg(s - m * 60, 5, 'f', 2, QChar('0'));
}

// The "Levels" lines of the details: peak and RMS over all channels.
QString levelsText(const sound::Pcm& p) {
    int peak = 0;
    double sum = 0;
    for (int16_t v : p.samples) {
        peak = std::max(peak, std::abs(int(v)));
        sum += double(v) * v;
    }
    auto dbfs = [](double v) {
        return v > 0 ? QStringLiteral("%1 dBFS").arg(20 * std::log10(v / 32768.0), 0, 'f', 1) : QStringLiteral("silent");
    };
    const double rms = p.samples.empty() ? 0 : std::sqrt(sum / double(p.samples.size()));
    return QStringLiteral("\n%1\n  %2 %3\n  %4 %5\n")
        .arg(QCoreApplication::translate("SoundView", "Levels"))
        .arg(QCoreApplication::translate("SoundView", "Peak"), -14).arg(dbfs(peak))
        .arg(QCoreApplication::translate("SoundView", "RMS"), -14).arg(dbfs(rms));
}

// The sound as the output device takes it, read by QAudioSink in pull
// mode. Wraps around when looping; otherwise ends, and the sink goes idle.
class PcmSource : public QIODevice {
public:
    QByteArray bytes;
    std::atomic<qint64> at{0};
    std::atomic<bool> loop{false};
    qint64 loopFrom = 0, loopTo = 0;   // bytes; looping goes back to loopFrom at loopTo

    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override {
        return (loop ? bytes.size() : bytes.size() - at) + QIODevice::bytesAvailable();
    }
    // Qt 6.8's sink tries to rewind the source; a sequential one can't, and
    // QIODevice would warn about it.
    bool seek(qint64) override { return false;
    }

protected:
    qint64 readData(char* out, qint64 max) override {
        qint64 done = 0;
        while (done < max && !bytes.isEmpty()) {
            qint64 p = at, end = loop ? loopTo : bytes.size();
            if (p >= end) {
                if (!loop) break;
                p = loopFrom;
            }
            qint64 n = std::min(max - done, end - p);
            std::memcpy(out + done, bytes.constData() + p, size_t(n));
            done += n;
            at = p + n;
        }
        return done;
    }
    qint64 writeData(const char*, qint64) override { return -1; }
};

// Min/max of every 256 frames per channel, for drawing long sounds quickly.
struct Peaks {
    static constexpr int kBlock = 256;
    std::vector<std::vector<std::pair<int16_t, int16_t>>> blocks;   // [channel][block]
};

class Waveform : public QWidget {
public:
    const sound::Pcm* pcm = nullptr;
    const Peaks* peaks = nullptr;
    qint64 playhead = 0;
    qint64 loopStart = -1, loopEnd = -1;
    std::function<void(qint64)> onPress, onDrag, onRelease;

    explicit Waveform(QWidget* parent) : QWidget(parent) {
        setMinimumSize(160, 80);
        setCursor(Qt::IBeamCursor);
        setToolTip(tr("Click or drag to move through the sound"));
    }

protected:
    qint64 frameAt(int x) const {
        if (!pcm || !pcm->frames() || width() <= 1) return 0;
        return std::clamp<qint64>(qint64(double(x) / (width() - 1) * double(pcm->frames())), 0, qint64(pcm->frames()));
    }
    int xAt(qint64 f) const {
        return pcm && pcm->frames() ? int(double(f) / double(pcm->frames()) * (width() - 1)) : 0;
    }
    void mousePressEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton && onPress) onPress(frameAt(int(e->position().x())));
    }
    void mouseMoveEvent(QMouseEvent* e) override {
        if ((e->buttons() & Qt::LeftButton) && onDrag) onDrag(frameAt(int(e->position().x())));
    }
    void mouseReleaseEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton && onRelease) onRelease(frameAt(int(e->position().x())));
    }

    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        const QPalette& pal = palette();
        p.fillRect(rect(), pal.base());
        if (!pcm || !pcm->frames() || !peaks) return;
        const int ch = pcm->channels, w = width();
        const double lane = double(height()) / ch;
        const double per = double(pcm->frames()) / w;   // frames per column
        const int head = xAt(playhead);
        QColor played = pal.highlight().color(), rest = pal.text().color();
        rest.setAlphaF(0.55f);
        if (loopStart >= 0 && loopEnd > loopStart) {
            QColor shade = pal.highlight().color();
            shade.setAlphaF(0.12f);
            p.fillRect(QRect(QPoint(xAt(loopStart), 0), QPoint(xAt(loopEnd), height())), shade);
        }
        for (int c = 0; c < ch; ++c) {
            const double mid = lane * (c + 0.5), half = lane * 0.45;
            QColor axis = pal.mid().color();
            p.setPen(axis);
            p.drawLine(0, int(mid), w, int(mid));
            const auto& blk = peaks->blocks[size_t(c)];
            for (int x = 0; x < w; ++x) {
                qint64 a = qint64(x * per), b = std::max(a + 1, qint64((x + 1) * per));
                b = std::min<qint64>(b, qint64(pcm->frames()));
                int lo = 32767, hi = -32768;
                if (b - a >= 2 * Peaks::kBlock) {   // whole blocks, then the ends sample by sample
                    qint64 ba = (a + Peaks::kBlock - 1) / Peaks::kBlock, bb = b / Peaks::kBlock;
                    for (qint64 k = ba; k < bb; ++k) {
                        lo = std::min<int>(lo, blk[size_t(k)].first);
                        hi = std::max<int>(hi, blk[size_t(k)].second);
                    }
                    for (qint64 f = a; f < ba * Peaks::kBlock; ++f) {
                        int v = pcm->samples[size_t(f * ch + c)];
                        lo = std::min(lo, v), hi = std::max(hi, v);
                    }
                    for (qint64 f = bb * Peaks::kBlock; f < b; ++f) {
                        int v = pcm->samples[size_t(f * ch + c)];
                        lo = std::min(lo, v), hi = std::max(hi, v);
                    }
                } else {
                    for (qint64 f = a; f < b; ++f) {
                        int v = pcm->samples[size_t(f * ch + c)];
                        lo = std::min(lo, v), hi = std::max(hi, v);
                    }
                }
                if (lo > hi) continue;
                p.setPen(x <= head ? played : rest);
                p.drawLine(x, int(mid - hi / 32768.0 * half), x, int(mid - lo / 32768.0 * half));
            }
        }
        p.setPen(QPen(pal.highlight().color(), 2));
        p.drawLine(head, 0, head, height());
    }
};

// Linear resampling and channel mapping to 16-bit or float output.
QByteArray convert(const sound::Pcm& in, const QAudioFormat& f) {
    const int oc = f.channelCount(), ic = in.channels;
    const double ratio = double(in.sample_rate) / f.sampleRate();
    const qint64 frames = qint64(std::floor(double(in.frames()) / ratio));
    const bool fl = f.sampleFormat() == QAudioFormat::Float;
    QByteArray out(int(frames * oc * (fl ? 4 : 2)), Qt::Uninitialized);
    auto* o16 = reinterpret_cast<int16_t*>(out.data());
    auto* of = reinterpret_cast<float*>(out.data());
    const qint64 last = qint64(in.frames()) - 1;
    for (qint64 k = 0; k < frames; ++k) {
        double pos = k * ratio;
        qint64 i = qint64(pos);
        double t = pos - double(i);
        qint64 j = std::min(i + 1, last);
        for (int c = 0; c < oc; ++c) {
            double v;
            if (ic == 1 || oc > 1) {
                int sc = c % ic;
                v = in.samples[size_t(i * ic + sc)] * (1 - t) + in.samples[size_t(j * ic + sc)] * t;
            } else {   // several channels to mono: their average
                v = 0;
                for (int sc = 0; sc < ic; ++sc)
                    v += in.samples[size_t(i * ic + sc)] * (1 - t) + in.samples[size_t(j * ic + sc)] * t;
                v /= ic;
            }
            if (fl) of[k * oc + c] = float(v / 32768.0);
            else o16[k * oc + c] = int16_t(std::lround(std::clamp(v, -32768.0, 32767.0)));
        }
    }
    return out;
}

}  // namespace

struct SoundView::Impl {
    sound::Pcm pcm;
    sound::Info info;
    Peaks peaks;
    QString name;

    QToolButton *play = nullptr, *stop = nullptr;
    QLabel* time = nullptr;
    QCheckBox *loop = nullptr, *autoplay = nullptr;
    QSlider* volume = nullptr;
    Waveform* wave = nullptr;
    QPlainTextEdit* text = nullptr;
    QLabel* status = nullptr;

    QAudioSink* sink = nullptr;
    PcmSource source;
    QAudioFormat outFormat;
    QString outNote;            // how the device takes it, if converted
    qint64 pos = 0;             // in source frames, while not playing
    qint64 loopStart = -1, loopEnd = -1;   // the loop region, in source frames, if any
    qint64 anchorOut = 0;       // output frame at the last start or seek
    qint64 anchorUs = 0;        // the device's clock then
    bool seekWhilePaused = false;
    QTimer timer;

    qint64 outFrames() const {
        int bpf = outFormat.bytesPerFrame();
        return bpf ? source.bytes.size() / bpf : 0;
    }
    qint64 toOut(qint64 f) const { return pcm.frames() ? qint64(double(f) * outFrames() / double(pcm.frames())) : 0; }
    qint64 fromOut(qint64 f) const { return outFrames() ? qint64(double(f) * double(pcm.frames()) / outFrames()) : 0; }

    // Position follows the sink's processed audio time. The output device
    // may add latency before those samples reach the speakers.
    qint64 position() const {
        if (!sink || sink->state() == QAudio::StoppedState || sink->state() == QAudio::SuspendedState) return pos;
        qint64 out = anchorOut + qint64(double(sink->processedUSecs() - anchorUs) * outFormat.sampleRate() / 1e6);
        const qint64 bpf = outFormat.bytesPerFrame(), a = source.loopFrom / bpf, b = source.loopTo / bpf;
        if (source.loop && b > a && out >= b) out = a + (out - a) % (b - a);
        return std::clamp<qint64>(fromOut(out), 0, qint64(pcm.frames()));
    }
    void anchor(qint64 frame) {
        anchorOut = toOut(frame);
        anchorUs = sink ? sink->processedUSecs() : 0;
        source.at = anchorOut * outFormat.bytesPerFrame();
    }
    void restart(qint64 frame) {
        sink->reset();
        source.close();
        // The PCM is already in memory; read-ahead would survive offset changes.
        source.open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        anchor(frame);
        anchorUs = 0;
        sink->start(&source);
    }
};

SoundView::SoundView(QWidget* parent) : QWidget(parent), d(new Impl) {
    auto* top = new QVBoxLayout(this);
    top->setContentsMargins(0, 0, 0, 0);

    auto* bar = new QHBoxLayout();
    d->play = new QToolButton(this);
    d->play->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    d->play->setToolTip(tr("Play / pause (Space)"));
    d->stop = new QToolButton(this);
    d->stop->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    d->stop->setToolTip(tr("Stop"));
    d->time = new QLabel(this);
    d->time->setMinimumWidth(130);
    d->loop = new QCheckBox(tr("Loop"), this);
    d->loop->setToolTip(tr("Loop as the game does: the sound's loop region (shaded) if it has one, else all of it"));
    d->autoplay = new QCheckBox(tr("Auto-play"), this);
    d->autoplay->setToolTip(tr("Start each sound as soon as it's selected"));
    d->autoplay->setChecked(QSettings().value("soundAutoPlay", false).toBool());
    auto* vol = new QLabel(this);
    vol->setPixmap(style()->standardIcon(QStyle::SP_MediaVolume).pixmap(16, 16));
    d->volume = new QSlider(Qt::Horizontal, this);
    d->volume->setRange(0, 100);
    d->volume->setValue(QSettings().value("soundVolume", 80).toInt());
    d->volume->setMaximumWidth(90);
    d->volume->setToolTip(tr("Volume"));
    bar->addWidget(d->play);
    bar->addWidget(d->stop);
    bar->addWidget(d->time);
    bar->addStretch(1);
    bar->addWidget(d->loop);
    bar->addWidget(d->autoplay);
    bar->addWidget(vol);
    bar->addWidget(d->volume);
    top->addLayout(bar);

    auto* split = new QSplitter(Qt::Vertical, this);
    auto* waveBox = new QWidget(split);
    auto* wl = new QVBoxLayout(waveBox);
    wl->setContentsMargins(0, 0, 0, 0);
    d->wave = new Waveform(waveBox);
    d->status = new QLabel(waveBox);
    d->status->setWordWrap(true);
    d->status->hide();
    wl->addWidget(d->wave, 1);
    wl->addWidget(d->status);
    d->text = new QPlainTextEdit(split);
    d->text->setReadOnly(true);
    d->text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    split->addWidget(waveBox);
    split->addWidget(d->text);
    split->setStretchFactor(0, 2);
    split->setStretchFactor(1, 1);
    top->addWidget(split, 1);

    d->wave->pcm = &d->pcm;
    d->wave->peaks = &d->peaks;

    auto* space = new QShortcut(QKeySequence(Qt::Key_Space), this, nullptr, nullptr, Qt::WidgetWithChildrenShortcut);
    connect(space, &QShortcut::activated, this, [this] { isPlaying() ? pause() : play(); });
    connect(d->play, &QToolButton::clicked, this, [this] { isPlaying() ? pause() : play(); });
    connect(d->stop, &QToolButton::clicked, this, &SoundView::stop);
    connect(d->loop, &QCheckBox::toggled, this, [this](bool on) { d->source.loop = on; });
    connect(d->autoplay, &QCheckBox::toggled, this, [](bool on) { QSettings().setValue("soundAutoPlay", on); });
    connect(d->volume, &QSlider::valueChanged, this, [this](int v) {
        QSettings().setValue("soundVolume", v);
        if (d->sink)
            d->sink->setVolume(QAudio::convertVolume(v / 100.0, QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale));
    });

    // Scrubbing: the sound follows the mouse, playing or not.
    d->wave->onPress = [this](qint64 f) { seek(f); };
    d->wave->onDrag = [this](qint64 f) { seek(f); };
    d->wave->onRelease = [this](qint64 f) { seek(f); };

    connect(&d->timer, &QTimer::timeout, this, [this] {
        qint64 p = d->position();
        // played to the end (the device's clock, in microseconds, can stop
        // a frame or so short of it)
        if (!d->source.loop && qint64(d->pcm.frames()) - p <= d->pcm.sample_rate / 50) {
            // Finish queued audio at natural completion; an explicit Stop discards it.
            if (d->sink) d->sink->stop();
            return stop();
        }
        d->wave->playhead = p;
        d->wave->update();
        d->time->setText(timeText(double(p) / std::max(1, d->pcm.sample_rate)) + " / " + timeText(d->pcm.seconds()));
        emit positionChanged(p);
    });

    clear();
}

SoundView::~SoundView() {
    stop();
    delete d;
}

void SoundView::clear() {
    stop();
    d->pcm = {};
    d->info = {};
    d->peaks = {};
    d->source.bytes.clear();
    d->wave->loopStart = d->wave->loopEnd = -1;
    d->loopStart = d->loopEnd = -1;
    d->text->clear();
    d->status->hide();
    d->play->setEnabled(false);
    d->stop->setEnabled(false);
    d->time->setText(timeText(0) + " / " + timeText(0));
    d->wave->update();
}

bool SoundView::loadWav(const QByteArray& data, const QString& name, QString* error) {
    clear();
    d->name = name;
    try {
        d->pcm = sound::decode(reinterpret_cast<const uint8_t*>(data.constData()), size_t(data.size()), &d->info);
    } catch (const std::exception& e) {
        if (error) *error = tr("Can't play this sound: %1").arg(QString::fromUtf8(e.what()));
        return false;
    }
    const sound::Info& i = d->info;
    const sound::Pcm& p = d->pcm;
    // The loop: the engine's (a cue point and its length), else a "smpl"
    // loop; looping plays it as the game does, else the whole sound.
    uint32_t ls = 0, le = 0;
    if (!(i.engine_loop(ls, le) || (i.has_loop && i.loop_end > i.loop_start && ((ls = i.loop_start), (le = i.loop_end), true))))
        ls = le = 0;

    QString t;
    auto head = [&t](const QString& h) { t += (t.isEmpty() ? "" : "\n") + h + "\n"; };
    auto row = [&t](const QString& k, const QString& v) { t += QStringLiteral("  %1 %2\n").arg(k, -14).arg(v); };
    t += name + "\n";
    head(tr("Format"));
    QString codec = QString::fromLatin1(sound::codec_name(i.format_tag));
    if (i.format_tag == sound::WAVE_MPEGLAYER3 && i.mpeg_version)
        codec = tr("MPEG-%1 layer %2").arg(i.mpeg_version == 25 ? QStringLiteral("2.5") : QString::number(i.mpeg_version))
                    .arg(i.mpeg_layer);
    row(tr("Codec"), codec + QStringLiteral(" (format 0x%1)").arg(i.format_tag, 4, 16, QChar('0')));
    QString chans = p.channels == 1 ? tr("mono") : p.channels == 2 ? tr("stereo") : tr("%1 channels").arg(p.channels);
    if (i.mpeg_mode >= 0) {
        static const char* modes[] = {"stereo", "joint stereo", "dual channel", "mono"};
        QString m = QString::fromLatin1(modes[i.mpeg_mode]);
        if (i.mpeg_mode == 1) {
            QStringList ext;
            if (i.mpeg_mode_ext & 2) ext << tr("mid/side");
            if (i.mpeg_mode_ext & 1) ext << tr("intensity");
            if (!ext.isEmpty()) m += " (" + ext.join(", ") + ")";
        }
        if (m != chans) chans += ", " + m;
    }
    row(tr("Channels"), chans);
    row(tr("Sample rate"), tr("%1 Hz").arg(p.sample_rate));
    if (i.format_tag != sound::WAVE_MPEGLAYER3) row(tr("Bits"), QString::number(i.bits_per_sample));
    if (i.bitrate_max)
        row(tr("Bit rate"), i.bitrate_min == i.bitrate_max ? tr("%1 kbit/s (constant)").arg(i.bitrate_max)
                                                           : tr("%1 to %2 kbit/s (variable)").arg(i.bitrate_min).arg(i.bitrate_max));
    else if (i.avg_bytes_per_sec)
        row(tr("Bit rate"), tr("%1 kbit/s").arg(i.avg_bytes_per_sec * 8 / 1000.0, 0, 'f', 0));
    if (i.samples_per_block) row(tr("Blocks"), tr("%1 bytes, %2 samples each").arg(i.block_align).arg(i.samples_per_block));
    if (i.mpeg_frames) row(tr("MPEG frames"), QString::number(i.mpeg_frames));
    row(tr("Duration"), timeText(p.seconds()) + tr(" (%1 frames)").arg(p.frames()));
    row(tr("Data"), tr("%1 bytes%2").arg(i.data_bytes).arg(i.data_truncated ? tr(" (the file ends early)") : QString()));
    if (i.data_bytes && p.frames() && i.format_tag != sound::WAVE_PCM)
        row(tr("Compression"), tr("%1 : 1 against 16-bit PCM").arg(double(p.samples.size()) * 2 / i.data_bytes, 0, 'f', 1));
    if (i.has_fact) row(tr("fact"), tr("%1 samples").arg(i.fact_samples));
    t += levelsText(p);

    if (i.has_lith || i.has_cue) {
        head(tr("LithTech"));
        if (i.has_lith) {
            QStringList f;
            if (i.lith_flags & sound::LITH_STREAM) f << tr("streamed");
            if (i.lith_flags & sound::LITH_DECOMPRESS_ON_LOAD) f << tr("decompressed on load");
            if (i.lith_flags & sound::LITH_DECOMPRESS_AT_START) f << tr("decompressed when first played");
            if (uint32_t other = i.lith_flags & ~7u) f << QStringLiteral("0x%1").arg(other, 0, 16);
            row(tr("Buffer"), f.isEmpty() ? tr("loaded whole (no flags)") : f.join(", "));
            if (i.lith_pitch_mod != 0) row(tr("Pitch"), tr("varies by up to ±%1% each play").arg(double(i.lith_pitch_mod) * 100, 0, 'g', 3));
        }
        uint32_t a2, b2;
        if (i.engine_loop(a2, b2))
            row(tr("Loop"), tr("%1 to %2 (frames %3 to %4)").arg(timeText(double(a2) / p.sample_rate), timeText(double(b2) / p.sample_rate))
                               .arg(a2).arg(b2));
        else if (i.has_cue)
            row(tr("Cue point"), tr("frame %1 (no length, so no loop)").arg(i.cue_position));
        if (!i.cue_label.empty()) row(tr("Cue label"), QString::fromStdString(i.cue_label));
    }
    if (i.has_loop) {
        head(tr("Sampler loop"));
        row(tr("Loop"), tr("frames %1 to %2").arg(i.loop_start).arg(i.loop_end));
    }
    if (!i.tags.empty()) {
        head(tr("Tags"));
        for (auto& [k, v] : i.tags) row(QString::fromStdString(sound::tag_name(k)), QString::fromLatin1(v.c_str()));
    }
    head(tr("Chunks"));
    t += "  " + QString::fromStdString([&] {
             std::string s;
             for (auto c : i.chunks) {
                 while (!c.empty() && c.back() == ' ') c.pop_back();   // "fmt "
                 s += (s.empty() ? "" : " ") + c;
             }
             return s;
         }()) + "\n";
    return finishLoad(ls, le, t);
}

// Shared by loadWav and loadPcm: the waveform's peaks, the loop, the
// details, and the sound as the output device takes it.
bool SoundView::finishLoad(qint64 loopStart, qint64 loopEnd, const QString& text) {
    const sound::Pcm& p = d->pcm;
    for (int c = 0; c < p.channels; ++c) {
        auto& blk = d->peaks.blocks.emplace_back();
        for (size_t f = 0; f < p.frames(); f += Peaks::kBlock) {
            int16_t lo = 32767, hi = -32768;
            for (size_t g = f; g < std::min(p.frames(), f + Peaks::kBlock); ++g) {
                int16_t v = p.samples[g * size_t(p.channels) + size_t(c)];
                lo = std::min(lo, v), hi = std::max(hi, v);
            }
            blk.emplace_back(lo, hi);
        }
    }
    loopEnd = std::min<qint64>(loopEnd, qint64(p.frames()));
    if (loopEnd > loopStart && loopStart >= 0) {
        d->loopStart = d->wave->loopStart = loopStart;
        d->loopEnd = d->wave->loopEnd = loopEnd;
    }
    d->text->setPlainText(text);

    if (!p.frames()) {
        d->status->setText(tr("This sound is empty: it has no samples."));
        d->status->show();
        d->wave->update();
        return true;
    }
    const QAudioDevice dev = QMediaDevices::defaultAudioOutput();
    if (dev.isNull()) {
        d->status->setText(tr("There's no sound output device, so this can't be played."));
        d->status->show();
    } else {
        QAudioFormat f;
        f.setSampleRate(p.sample_rate);
        f.setChannelCount(p.channels);
        f.setSampleFormat(QAudioFormat::Int16);
        if (!dev.isFormatSupported(f)) {
            // the device's own rate and channels, in 16 bits if it takes them
            QAudioFormat pref = dev.preferredFormat();
            f.setSampleRate(pref.sampleRate());
            f.setChannelCount(pref.channelCount());
            if (!dev.isFormatSupported(f)) f = pref;
            d->outNote = tr("Played at %1 Hz, %2 channels (the device's format)").arg(f.sampleRate()).arg(f.channelCount());
            d->text->appendPlainText("\n" + d->outNote);
        }
        d->outFormat = f;
        d->source.bytes = f.sampleRate() == p.sample_rate && f.channelCount() == p.channels &&
                                  f.sampleFormat() == QAudioFormat::Int16
                              ? QByteArray(reinterpret_cast<const char*>(p.samples.data()), int(p.samples.size() * 2))
                              : convert(p, f);
        d->play->setEnabled(true);
        d->stop->setEnabled(true);
    }
    d->time->setText(timeText(0) + " / " + timeText(p.seconds()));
    d->wave->update();
    if (d->autoplay->isChecked()) play();
    return true;
}

bool SoundView::loadPcm(sound::Pcm pcm, const QString& name, const QString& details, qint64 loopStart,
                        qint64 loopEnd) {
    clear();
    d->name = name;
    d->pcm = std::move(pcm);
    return finishLoad(loopStart, loopEnd, details + levelsText(d->pcm));
}

void SoundView::play() {
    if (d->source.bytes.isEmpty() || isPlaying()) return;
    if (d->pos >= qint64(d->pcm.frames())) {
        d->pos = 0;
        d->seekWhilePaused = true;
    }
    d->source.loop = d->loop->isChecked();
    const qint64 bpf = d->outFormat.bytesPerFrame();
    d->source.loopFrom = d->loopEnd > d->loopStart && d->loopStart >= 0 ? d->toOut(d->loopStart) * bpf : 0;
    d->source.loopTo = d->loopEnd > d->loopStart && d->loopStart >= 0 ? d->toOut(d->loopEnd) * bpf : d->source.bytes.size();
    if (d->sink && d->sink->state() == QAudio::SuspendedState) {
        if (d->seekWhilePaused) d->restart(d->pos);
        else d->sink->resume();
        d->seekWhilePaused = false;
    } else {
        delete d->sink;
        d->sink = new QAudioSink(QMediaDevices::defaultAudioOutput(), d->outFormat, this);
        // a short buffer, so seeking answers quickly
        d->sink->setBufferSize(int(d->outFormat.bytesForDuration(150000)));
        d->sink->setVolume(QAudio::convertVolume(d->volume->value() / 100.0, QAudio::LogarithmicVolumeScale,
                                                 QAudio::LinearVolumeScale));
        connect(d->sink, &QAudioSink::stateChanged, this, [this](QAudio::State s) {
            if (d->sink && s == QAudio::StoppedState && d->sink->error() != QAudio::NoError) {
                d->status->setText(tr("Couldn't play this sound on the output device (error %1).").arg(int(d->sink->error())));
                d->status->show();
            }
        });
        d->seekWhilePaused = false;
        d->restart(d->pos);
        if (d->sink->error() != QAudio::NoError) {
            d->status->setText(tr("Couldn't open the sound output device."));
            d->status->show();
            return;
        }
    }
    d->play->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    d->timer.start(30);
}

void SoundView::pause() {
    if (!isPlaying()) return;
    d->pos = d->position();
    d->sink->suspend();
    d->timer.stop();
    d->play->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
}

void SoundView::stop() {
    if (d->sink) {
        QAudioSink* s = d->sink;
        d->sink = nullptr;
        disconnect(s, nullptr, this, nullptr);
        s->reset();
        s->deleteLater();
    }
    d->source.close();
    d->timer.stop();
    d->pos = 0;
    d->play->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    d->wave->playhead = 0;
    d->wave->update();
    d->time->setText(timeText(0) + " / " + timeText(d->pcm.seconds()));
    emit positionChanged(0);
}

bool SoundView::isPlaying() const {
    return d->sink && (d->sink->state() == QAudio::ActiveState || d->sink->state() == QAudio::IdleState);
}

qint64 SoundView::position() const { return d->position(); }
qint64 SoundView::frames() const { return qint64(d->pcm.frames()); }
int SoundView::sampleRate() const { return d->pcm.sample_rate; }
void SoundView::setLoop(bool on) { d->loop->setChecked(on); }
bool SoundView::loopRegion(qint64& start, qint64& end) const {
    start = d->loopStart;
    end = d->loopEnd;
    return end > start && start >= 0;
}

void SoundView::seek(qint64 frame) {
    frame = std::clamp<qint64>(frame, 0, qint64(d->pcm.frames()));
    d->pos = frame;
    if (isPlaying()) d->restart(frame);
    else d->seekWhilePaused = true;
    d->wave->playhead = frame;
    d->wave->update();
    d->time->setText(timeText(double(frame) / std::max(1, d->pcm.sample_rate)) + " / " + timeText(d->pcm.seconds()));
    emit positionChanged(frame);
}
