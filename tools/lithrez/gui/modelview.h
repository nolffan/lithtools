#pragma once
// A simple model preview: an .abc (NOLF, AvP2) or .ltb (NOLF2, Contract
// J.A.C.K., Tron 2.0) drawn with OpenGL, textured with skins
// found through a callback, with its animations playable.

#include <QByteArray>
#include <QImage>
#include <QString>
#include <QWidget>
#include <functional>

class ModelView : public QWidget {
    Q_OBJECT
public:
    // Returns a skin's .dtx bytes and sets its path, or returns an empty array.
    using SkinLookup = std::function<QByteArray(int textureIndex, int textureCount, QString& path)>;

    explicit ModelView(QWidget* parent = nullptr);
    ~ModelView() override;
    // Either format: loadAbc tells .abc and .ltb apart by their first bytes.
    bool loadAbc(const QByteArray& data, const QString& name, SkinLookup skins, QString* error = nullptr);
    bool loadLtb(const QByteArray& data, const QString& name, SkinLookup skins, QString* error = nullptr);
    void clear();

    // For tests and scripting: pick an animation (-1 for the bind pose) and
    // a time in ms, turn the camera (degrees), and grab what is drawn.
    void setPose(int animation, float timeMs);
    void setViewAngles(float yawDegrees, float pitchDegrees);
    QImage grabFrame();

signals:
    void skinActivated(const QString& path);

private:
    struct Impl;
    Impl* d;
};
