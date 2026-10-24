// Sound preview: plays a .wav (any codec lithtech::sound decodes) with
// play/pause/stop, a waveform timeline to scrub on, loop and volume, and
// the file's details beside it.

#pragma once

#include <QWidget>

#include <lithtech/sound.hpp>

class SoundView : public QWidget {
    Q_OBJECT
public:
    explicit SoundView(QWidget* parent = nullptr);
    ~SoundView() override;

    bool loadWav(const QByteArray& data, const QString& name, QString* error = nullptr);
    // A sound decoded elsewhere (a PS2 bank's), with its details text (the
    // levels are added) and loop region in frames, if any.
    bool loadPcm(lithtech::sound::Pcm pcm, const QString& name, const QString& details, qint64 loopStart = -1,
                 qint64 loopEnd = -1);
    void clear();

    void play();
    void pause();
    void stop();   // and back to the start
    bool isPlaying() const;

    // For tests and scripting: the playback position in sample frames of
    // the sound, and moving it.
    qint64 position() const;
    void seek(qint64 frame);
    qint64 frames() const;
    int sampleRate() const;
    void setLoop(bool on);
    // The loop region (the engine's, else a sampler loop), in frames.
    bool loopRegion(qint64& start, qint64& end) const;

signals:
    void positionChanged(qint64 frame);

private:
    bool finishLoad(qint64 loopStart, qint64 loopEnd, const QString& text);
    struct Impl;
    Impl* d;
};
