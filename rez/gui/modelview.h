#pragma once
// A simple model preview: an .abc drawn with OpenGL, textured with skins
// found through a callback, with its animations playable.

#include <QByteArray>
#include <QImage>
#include <QString>
#include <QWidget>
#include <functional>

class ModelView : public QWidget {
    Q_OBJECT
public:
    // Returns a skin's .dtx bytes for a texture slot, or an empty array.
    using SkinLookup = std::function<QByteArray(int textureIndex, int textureCount)>;

    explicit ModelView(QWidget* parent = nullptr);
    ~ModelView() override;
    bool loadAbc(const QByteArray& data, const QString& name, SkinLookup skins, QString* error = nullptr);
    void clear();

    // For tests and scripting: pick an animation (-1 for the bind pose) and
    // a time in ms, turn the camera (degrees), and grab what is drawn.
    void setPose(int animation, float timeMs);
    void setViewAngles(float yawDegrees, float pitchDegrees);
    QImage grabFrame();

private:
    struct Impl;
    Impl* d;
};
