#pragma once
// DTX texture viewing: the preview pane widget, thumbnails for the icon
// views, and the standalone viewer window (WinRez LT Studio's DTXView).
// The preview pane shows PCX, TGA and BMP images in it too.

#include <QImage>
#include <QMainWindow>
#include <QStringList>
#include <QWidget>

class QAction;
class QMenu;

class TextureView : public QWidget {
    Q_OBJECT
public:
    enum class AlphaMode { Automatic, Blend, Ignore, AlphaOnly };

    explicit TextureView(QWidget* parent = nullptr);
    ~TextureView() override;
    // Shows a .dtx; false (and *error) if it can't be read.
    bool loadDtx(const QByteArray& data, const QString& name, QString* error = nullptr);
    // Shows a .pcx, .tga or .bmp (by name's extension; no mips or faces);
    // false (and *error) if it can't be read.
    bool loadImage(const QByteArray& data, const QString& name, QString* error = nullptr);

    void clear();

    bool hasTexture() const;
    QString name() const;
    // The current level/face as decoded (straight RGBA), null if none.
    QImage currentImage() const;
    // 0 means fit to the view.
    double zoom() const;

public slots:
    void setZoom(double factor);   // 0 = fit
    void zoomIn();
    void zoomOut();
    void setMip(int mip);
    void setFace(int face);
    void setAlphaMode(TextureView::AlphaMode mode);
    // Palette alpha as the PS2 draws it (0x80 opaque; see lithtech::dtx::
    // PaletteAlpha) rather than as stored. For textures from a PS2 archive.
    void setPs2Alpha(bool on);
    bool ps2Alpha() const;
    void showInfo(bool show);
    // Asks for a file name (unless path is given) and writes the current
    // level/face; false on failure or cancel.
    bool exportPng(const QString& path = QString());
    void copyImage();

signals:
    void zoomChanged(double factor);
    void textureChanged();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    struct Impl;
    Impl* d;
    void refresh();
    void applyZoom();
};

// A thumbnail of a .dtx (null image if it can't be read), at most size px.
QImage dtxThumbnail(const QByteArray& data, int size, bool ps2Alpha = false);
// The same for a .pcx, .tga or .bmp (type: its extension).
QImage imageThumbnail(const QByteArray& data, const QString& type, int size);

// The standalone viewer: File > Open, zoom, mip/face, alpha, export PNG.
class TextureWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit TextureWindow(QWidget* parent = nullptr);
    bool openFile(const QString& path);
    bool openData(const QByteArray& data, const QString& name);
    TextureView* view() const { return view_; }

protected:
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dropEvent(QDropEvent* e) override;

private:
    TextureView* view_;
    QMenu* recentMenu_ = nullptr;
    void openDialog();
    void addRecent(const QString& path);
    void rebuildRecent();
};
