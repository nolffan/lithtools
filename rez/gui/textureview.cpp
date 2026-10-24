#include "textureview.h"

#include <lithtech/dtx.hpp>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QTextBrowser>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>

namespace {

using lithtech::dtx::Texture;

const double kZooms[] = {0.125, 0.25, 0.5, 1, 2, 3, 4, 8, 16};

// RGBA bytes (top row first) as a QImage that owns its pixels.
QImage toQImage(const lithtech::dtx::Image& img) {
    if (img.width <= 0 || img.height <= 0) return {};
    QImage q(img.width, img.height, QImage::Format_RGBA8888);
    for (int y = 0; y < img.height; ++y)
        std::memcpy(q.scanLine(y), img.rgba.data() + std::size_t(y) * img.width * 4, std::size_t(img.width) * 4);
    return q;
}

void paintChecker(QPainter& p, const QRect& r, int cell, QColor a, QColor b) {
    p.fillRect(r, a);
    for (int y = r.top(); y < r.bottom() + 1; y += cell)
        for (int x = r.left() + (((y - r.top()) / cell) % 2) * cell; x < r.right() + 1; x += 2 * cell)
            p.fillRect(QRect(x, y, cell, cell).intersected(r), b);
}

QString flagNames(uint32_t f) {
    using namespace lithtech::dtx;
    struct { uint32_t bit; const char* name; } names[] = {
        {DTX_FULLBRITE, "FULLBRITE"}, {DTX_PREFER16BIT, "PREFER16BIT"}, {1u << 2, "MIPSALLOCED"},
        {DTX_SECTIONSFIXED, "SECTIONSFIXED"}, {DTX_NOSYSCACHE, "NOSYSCACHE"}, {DTX_PREFER4444, "PREFER4444"},
        {DTX_PREFER5551, "PREFER5551"}, {DTX_32BITSYSCOPY, "32BITSYSCOPY"}, {DTX_CUBEMAP, "CUBEMAP"},
        {DTX_BUMPMAP, "BUMPMAP"}, {DTX_LUMBUMPMAP, "LUMBUMPMAP"},
    };
    QStringList out;
    uint32_t known = 0;
    for (auto& n : names) {
        known |= n.bit;
        if (f & n.bit) out << QString::fromLatin1(n.name);
    }
    if (f & ~known) out << QStringLiteral("0x%1").arg(f & ~known, 0, 16);
    return out.isEmpty() ? QStringLiteral("none") : out.join(QStringLiteral(" | "));
}

// The canvas: the image scaled (nearest neighbour when enlarged) over a
// checkerboard.
class Canvas : public QWidget {
public:
    explicit Canvas(QWidget* parent) : QWidget(parent) { setAttribute(Qt::WA_OpaquePaintEvent, false); }
    void setImage(const QImage& img) { image_ = img; resizeToZoom(); }
    void setFactor(double f) { factor_ = f; resizeToZoom(); }
    void setMessage(const QString& m) { message_ = m; update(); }
    double factor() const { return factor_; }
    const QImage& image() const { return image_; }
    void resizeToZoom() {
        if (image_.isNull()) {
            setFixedSize(QSize(1, 1));
        } else {
            QSize s(std::max(1, int(std::lround(image_.width() * factor_))),
                    std::max(1, int(std::lround(image_.height() * factor_))));
            setFixedSize(s);
        }
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        if (image_.isNull()) return;
        paintChecker(p, rect(), 8, QColor(204, 204, 204), QColor(255, 255, 255));
        p.setRenderHint(QPainter::SmoothPixmapTransform, factor_ < 1.0);
        p.drawImage(rect(), image_);
    }

private:
    QImage image_;
    double factor_ = 1.0;
    QString message_;
};

}  // namespace

struct TextureView::Impl {
    Texture tex;
    bool loaded = false;
    QString name;
    int mip = 0, face = 0;
    AlphaMode alpha = AlphaMode::Blend;
    double zoom = 0;      // 0 = fit
    QImage decoded;       // straight RGBA of the current level/face

    QComboBox* zoomBox = nullptr;
    QComboBox* mipBox = nullptr;
    QComboBox* faceBox = nullptr;
    QComboBox* alphaBox = nullptr;
    QLabel* faceLabel = nullptr;
    QToolButton* exportBtn = nullptr;
    QToolButton* copyBtn = nullptr;
    QToolButton* infoBtn = nullptr;
    QScrollArea* scroll = nullptr;
    Canvas* canvas = nullptr;
    QLabel* message = nullptr;
    QTextBrowser* info = nullptr;
    QSplitter* split = nullptr;
    bool updating = false;
};

TextureView::TextureView(QWidget* parent) : QWidget(parent), d(new Impl) {
    auto* top = new QHBoxLayout;
    top->setContentsMargins(4, 4, 4, 0);
    top->setSpacing(6);

    d->zoomBox = new QComboBox(this);
    d->zoomBox->addItem(tr("Fit"), 0.0);
    for (double z : kZooms) d->zoomBox->addItem(QStringLiteral("%1%").arg(z * 100), z);
    d->zoomBox->setToolTip(tr("Zoom (Ctrl+wheel)"));
    d->mipBox = new QComboBox(this);
    d->mipBox->setToolTip(tr("Mip level"));
    d->faceLabel = new QLabel(tr("Face"), this);
    d->faceBox = new QComboBox(this);
    for (const char* f : {"+X", "-X", "+Y", "-Y", "+Z", "-Z"}) d->faceBox->addItem(QString::fromLatin1(f));
    d->faceBox->setToolTip(tr("Cube map face"));
    d->alphaBox = new QComboBox(this);
    d->alphaBox->addItem(tr("Alpha blended"));
    d->alphaBox->addItem(tr("Ignore alpha"));
    d->alphaBox->addItem(tr("Alpha only"));
    d->exportBtn = new QToolButton(this);
    d->exportBtn->setText(tr("Export PNG…"));
    d->copyBtn = new QToolButton(this);
    d->copyBtn->setText(tr("Copy"));
    d->infoBtn = new QToolButton(this);
    d->infoBtn->setText(tr("Info"));
    d->infoBtn->setCheckable(true);
    d->infoBtn->setChecked(true);

    top->addWidget(new QLabel(tr("Zoom"), this));
    top->addWidget(d->zoomBox);
    top->addWidget(new QLabel(tr("Mip"), this));
    top->addWidget(d->mipBox);
    top->addWidget(d->faceLabel);
    top->addWidget(d->faceBox);
    top->addWidget(d->alphaBox);
    top->addStretch(1);
    top->addWidget(d->copyBtn);
    top->addWidget(d->exportBtn);
    top->addWidget(d->infoBtn);

    d->scroll = new QScrollArea(this);
    d->scroll->setAlignment(Qt::AlignCenter);
    d->scroll->setBackgroundRole(QPalette::Dark);
    d->canvas = new Canvas(d->scroll);
    d->scroll->setWidget(d->canvas);
    d->scroll->setWidgetResizable(false);
    d->scroll->viewport()->installEventFilter(this);

    d->message = new QLabel(this);
    d->message->setAlignment(Qt::AlignCenter);
    d->message->setWordWrap(true);
    d->message->hide();

    auto* left = new QWidget(this);
    auto* ll = new QVBoxLayout(left);
    ll->setContentsMargins(0, 0, 0, 0);
    ll->addWidget(d->scroll, 1);
    ll->addWidget(d->message, 1);

    d->info = new QTextBrowser(this);
    d->info->setOpenLinks(false);
    d->info->setMinimumWidth(180);

    d->split = new QSplitter(Qt::Horizontal, this);
    d->split->addWidget(left);
    d->split->addWidget(d->info);
    d->split->setStretchFactor(0, 3);
    d->split->setStretchFactor(1, 1);
    d->split->setSizes({600, 240});

    auto* l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->addLayout(top);
    l->addWidget(d->split, 1);

    connect(d->zoomBox, QOverload<int>::of(&QComboBox::activated), this,
            [this](int i) { setZoom(d->zoomBox->itemData(i).toDouble()); });
    connect(d->mipBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int i) {
        if (!d->updating && i >= 0) setMip(i);
    });
    connect(d->faceBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int i) {
        if (!d->updating && i >= 0) setFace(i);
    });
    connect(d->alphaBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int i) { setAlphaMode(AlphaMode(i)); });
    connect(d->exportBtn, &QToolButton::clicked, this, [this] { exportPng(); });
    connect(d->copyBtn, &QToolButton::clicked, this, &TextureView::copyImage);
    connect(d->infoBtn, &QToolButton::toggled, this, &TextureView::showInfo);
    clear();
}

TextureView::~TextureView() { delete d; }

bool TextureView::hasTexture() const { return d->loaded; }
QString TextureView::name() const { return d->name; }
QImage TextureView::currentImage() const { return d->decoded; }
double TextureView::zoom() const { return d->zoom; }

void TextureView::clear() {
    d->loaded = false;
    d->tex = Texture{};
    d->name.clear();
    d->decoded = QImage();
    d->canvas->setImage(QImage());
    d->info->clear();
    d->updating = true;
    d->mipBox->clear();
    d->updating = false;
    d->faceBox->setVisible(false);
    d->faceLabel->setVisible(false);
    d->exportBtn->setEnabled(false);
    d->copyBtn->setEnabled(false);
    d->message->hide();
    d->scroll->show();
}

bool TextureView::loadDtx(const QByteArray& data, const QString& name, QString* error) {
    clear();
    d->name = name;
    try {
        std::vector<uint8_t> v(data.begin(), data.end());
        d->tex = lithtech::dtx::read(v, true);
    } catch (const std::exception& e) {
        QString msg = tr("Can't read %1: %2").arg(name, QString::fromUtf8(e.what()));
        if (error) *error = msg;
        d->scroll->hide();
        d->message->setText(msg);
        d->message->show();
        d->info->setPlainText(msg);
        return false;
    }
    d->loaded = true;
    d->mip = 0;
    d->face = 0;
    d->updating = true;
    for (int i = 0; i < int(d->tex.mips.size()); ++i)
        d->mipBox->addItem(QStringLiteral("%1: %2×%3").arg(i).arg(std::max(d->tex.width >> i, 0)).arg(std::max(d->tex.height >> i, 0)));
    d->faceBox->setCurrentIndex(0);
    d->updating = false;
    bool cube = d->tex.is_cubemap() && d->tex.find_section("CUBEMAPDATA");
    d->faceBox->setVisible(cube);
    d->faceLabel->setVisible(cube);
    refresh();
    emit textureChanged();
    return true;
}

void TextureView::setMip(int mip) {
    if (!d->loaded || mip < 0 || mip >= int(d->tex.mips.size())) return;
    d->mip = mip;
    if (d->mipBox->currentIndex() != mip) {
        d->updating = true;
        d->mipBox->setCurrentIndex(mip);
        d->updating = false;
    }
    refresh();
}

void TextureView::setFace(int face) {
    if (!d->loaded || face < 0 || face > 5) return;
    d->face = face;
    if (d->faceBox->currentIndex() != face) {
        d->updating = true;
        d->faceBox->setCurrentIndex(face);
        d->updating = false;
    }
    refresh();
}

void TextureView::setAlphaMode(AlphaMode mode) {
    d->alpha = mode;
    if (d->alphaBox->currentIndex() != int(mode)) d->alphaBox->setCurrentIndex(int(mode));
    refresh();
}

void TextureView::showInfo(bool show) {
    d->info->setVisible(show);
    if (d->infoBtn->isChecked() != show) d->infoBtn->setChecked(show);
}

void TextureView::refresh() {
    if (!d->loaded) return;
    QString err;
    QImage img;
    try {
        img = toQImage(d->tex.decode(d->mip, d->face));
    } catch (const std::exception& e) {
        err = QString::fromUtf8(e.what());
    }
    d->decoded = img;
    QImage shown;
    if (!img.isNull()) {
        if (d->alpha == AlphaMode::Blend) {
            shown = img;
        } else {
            shown = QImage(img.size(), QImage::Format_RGBA8888);
            for (int y = 0; y < img.height(); ++y) {
                const uchar* s = img.constScanLine(y);
                uchar* o = shown.scanLine(y);
                for (int x = 0; x < img.width(); ++x, s += 4, o += 4) {
                    if (d->alpha == AlphaMode::Ignore) {
                        o[0] = s[0]; o[1] = s[1]; o[2] = s[2];
                    } else {
                        o[0] = o[1] = o[2] = s[3];
                    }
                    o[3] = 255;
                }
            }
        }
    }
    if (shown.isNull()) {
        d->scroll->hide();
        d->message->setText(err.isEmpty() ? tr("This level is empty.") : tr("Can't decode %1: %2").arg(d->name, err));
        d->message->show();
    } else {
        d->message->hide();
        d->scroll->show();
    }
    d->canvas->setImage(shown);
    d->exportBtn->setEnabled(!img.isNull());
    d->copyBtn->setEnabled(!img.isNull());
    applyZoom();

    // The info panel.
    const Texture& t = d->tex;
    auto row = [](const QString& k, const QString& v) {
        return QStringLiteral("<tr><td style='color:gray;padding-right:8px'>%1</td><td>%2</td></tr>").arg(k, v);
    };
    QString h = QStringLiteral("<b>%1</b><table>").arg(d->name.toHtmlEscaped());
    h += row(tr("Size"), QStringLiteral("%1 × %2").arg(t.width).arg(t.height));
    h += row(tr("Format"), QString::fromLatin1(lithtech::dtx::bpp_name(t.bpp())));
    h += row(tr("Mips"), QString::number(t.mip_count));
    h += row(tr("Version"), QString::number(t.version));
    h += row(tr("Flags"), flagNames(t.iflags).toHtmlEscaped());
    h += row(tr("User flags"), QStringLiteral("0x%1").arg(t.user_flags, 8, 16, QLatin1Char('0')));
    h += row(tr("Texture group"), QString::number(t.texture_group()));
    h += row(tr("Mipmaps to use"), QString::number(t.mipmaps_to_use()));
    h += row(tr("Non-S3TC mip offset"), QString::number(t.non_s3tc_mip_offset()));
    h += row(tr("UI mip offset"), QString::number(t.ui_mip_offset()));
    h += row(tr("Priority"), QString::number(t.priority()));
    h += row(tr("Detail scale"), QString::number(double(t.detail_scale()), 'g', 6));
    h += row(tr("Detail angle"), QString::number(t.detail_angle()));
    h += row(tr("Command"), t.command.empty() ? QStringLiteral("—") : QString::fromLatin1(t.command.c_str()).toHtmlEscaped());
    if (t.truncated) h += row(tr("Warning"), tr("the file ends inside its pixel data"));
    if (!err.isEmpty()) h += row(tr("Error"), err.toHtmlEscaped());
    h += QStringLiteral("</table>");
    if (!t.sections.empty()) {
        h += QStringLiteral("<p><b>%1</b></p><table>").arg(tr("Sections"));
        for (auto& s : t.sections)
            h += row(QString::fromLatin1(s.type.c_str()).toHtmlEscaped(),
                     QStringLiteral("%1 (%2 bytes)").arg(QString::fromLatin1(s.name.c_str()).toHtmlEscaped()).arg(s.data.size()));
        h += QStringLiteral("</table>");
        if (const auto* ld = t.find_section("LIGHTDEFS")) {
            QString text = QString::fromLatin1(reinterpret_cast<const char*>(ld->data.data()), int(ld->data.size()));
            int z = text.indexOf(QChar(0));
            if (z >= 0) text.truncate(z);
            h += QStringLiteral("<p><b>LIGHTDEFS</b></p><pre>%1</pre>").arg(text.toHtmlEscaped());
        }
    }
    d->info->setHtml(h);
}

void TextureView::applyZoom() {
    const QImage& img = d->canvas->image();
    double f = d->zoom;
    if (f <= 0 && !img.isNull()) {
        QSize vs = d->scroll->viewport()->size() - QSize(4, 4);
        f = std::min(double(vs.width()) / img.width(), double(vs.height()) / img.height());
        if (f > 1) f = std::floor(f);   // whole steps when enlarging: crisp pixels
        f = std::max(f, 1.0 / 64);
    }
    if (f <= 0) f = 1;
    d->canvas->setFactor(f);
    int idx = d->zoomBox->findData(d->zoom);
    if (idx >= 0) d->zoomBox->setCurrentIndex(idx);
    else d->zoomBox->setEditText(QStringLiteral("%1%").arg(d->zoom * 100));
}

void TextureView::setZoom(double factor) {
    d->zoom = std::max(0.0, factor);
    applyZoom();
    emit zoomChanged(d->zoom);
}

void TextureView::zoomIn() {
    double cur = d->zoom > 0 ? d->zoom : d->canvas->factor();
    for (double z : kZooms)
        if (z > cur + 1e-9) { setZoom(z); return; }
}

void TextureView::zoomOut() {
    double cur = d->zoom > 0 ? d->zoom : d->canvas->factor();
    for (int i = int(std::size(kZooms)) - 1; i >= 0; --i)
        if (kZooms[i] < cur - 1e-9) { setZoom(kZooms[i]); return; }
}

bool TextureView::eventFilter(QObject* watched, QEvent* event) {
    if (watched == d->scroll->viewport()) {
        if (event->type() == QEvent::Wheel) {
            auto* w = static_cast<QWheelEvent*>(event);
            if (w->modifiers() & Qt::ControlModifier) {
                if (w->angleDelta().y() > 0) zoomIn();
                else if (w->angleDelta().y() < 0) zoomOut();
                return true;
            }
        } else if (event->type() == QEvent::Resize && d->zoom <= 0) {
            applyZoom();
        }
    }
    return QWidget::eventFilter(watched, event);
}

bool TextureView::exportPng(const QString& path) {
    if (d->decoded.isNull()) return false;
    QString p = path;
    if (p.isEmpty()) {
        QString base = QFileInfo(d->name).completeBaseName();
        if (base.isEmpty()) base = QStringLiteral("texture");
        if (d->mip) base += QStringLiteral("_mip%1").arg(d->mip);
        if (d->face) base += QStringLiteral("_face%1").arg(d->face);
        p = QFileDialog::getSaveFileName(this, tr("Export PNG"), base + QStringLiteral(".png"), tr("PNG images (*.png)"));
        if (p.isEmpty()) return false;
    }
    if (!d->decoded.save(p, "PNG")) {
        if (path.isEmpty()) QMessageBox::warning(this, tr("Export PNG"), tr("Can't write %1").arg(p));
        return false;
    }
    return true;
}

void TextureView::copyImage() {
    if (!d->decoded.isNull()) QApplication::clipboard()->setImage(d->decoded);
}

// ---------------------------------------------------------------------------

QImage dtxThumbnail(const QByteArray& data, int size) {
    if (size <= 0) return {};
    try {
        std::vector<uint8_t> v(data.begin(), data.end());
        Texture t = lithtech::dtx::read(v, true);
        if (t.mips.empty() || t.width <= 0 || t.height <= 0) return {};
        // The smallest level that still covers the thumbnail.
        int mip = 0;
        for (int i = int(t.mips.size()) - 1; i >= 0; --i) {
            int w = t.width >> i, h = t.height >> i;
            if (std::max(w, h) >= size && t.mips[std::size_t(i)].size() >= lithtech::dtx::image_size(t.bpp(), w, h)) {
                mip = i;
                break;
            }
        }
        QImage img = toQImage(t.decode(mip));
        if (img.isNull()) return {};
        QImage scaled = img.scaled(size, size, Qt::KeepAspectRatio,
                                   std::max(img.width(), img.height()) > size ? Qt::SmoothTransformation : Qt::FastTransformation);
        QImage out(scaled.size(), QImage::Format_ARGB32_Premultiplied);
        QPainter p(&out);
        paintChecker(p, out.rect(), std::max(4, size / 16), QColor(235, 235, 235), QColor(255, 255, 255));
        p.drawImage(0, 0, scaled);
        p.end();
        return out;
    } catch (...) {
        return {};
    }
}

// ---------------------------------------------------------------------------

TextureWindow::TextureWindow(QWidget* parent) : QMainWindow(parent), view_(new TextureView(this)) {
    setCentralWidget(view_);
    setAcceptDrops(true);
    setWindowTitle(tr("DTX Viewer"));
    resize(900, 640);

    QMenu* file = menuBar()->addMenu(tr("&File"));
    file->addAction(tr("&Open…"), QKeySequence::Open, this, &TextureWindow::openDialog);
    recentMenu_ = file->addMenu(tr("Open &Recent"));
    QAction* exp = file->addAction(tr("&Export PNG…"), QKeySequence(tr("Ctrl+E")), view_, [this] { view_->exportPng(); });
    QAction* cp = file->addAction(tr("&Copy Image"), QKeySequence::Copy, view_, &TextureView::copyImage);
    file->addSeparator();
    file->addAction(tr("E&xit"), QKeySequence::Quit, this, &QWidget::close);

    QMenu* view = menuBar()->addMenu(tr("&View"));
    auto* group = new QActionGroup(this);
    auto zoomAct = [&](const QString& text, double f, const QKeySequence& key) {
        QAction* a = view->addAction(text, view_, [this, f] { view_->setZoom(f); });
        a->setShortcut(key);
        a->setCheckable(true);
        a->setData(f);
        group->addAction(a);
        return a;
    };
    zoomAct(tr("&Fit"), 0, QKeySequence(tr("Ctrl+0")))->setChecked(true);
    zoomAct(tr("&1x (100%)"), 1, QKeySequence(tr("Ctrl+1")));
    zoomAct(tr("&2x (200%)"), 2, QKeySequence(tr("Ctrl+2")));
    zoomAct(tr("&3x (300%)"), 3, QKeySequence(tr("Ctrl+3")));
    zoomAct(tr("&4x (400%)"), 4, QKeySequence(tr("Ctrl+4")));
    zoomAct(tr("&8x (800%)"), 8, QKeySequence(tr("Ctrl+8")));
    view->addSeparator();
    view->addAction(tr("Zoom &In"), QKeySequence::ZoomIn, view_, &TextureView::zoomIn);
    view->addAction(tr("Zoom &Out"), QKeySequence::ZoomOut, view_, &TextureView::zoomOut);
    view->addSeparator();
    QAction* info = view->addAction(tr("&Information Panel"));
    info->setCheckable(true);
    info->setChecked(true);
    connect(info, &QAction::toggled, view_, &TextureView::showInfo);
    connect(view_, &TextureView::zoomChanged, this, [group](double f) {
        for (QAction* a : group->actions()) a->setChecked(std::abs(a->data().toDouble() - f) < 1e-9);
    });
    auto enable = [this, exp, cp] {
        exp->setEnabled(!view_->currentImage().isNull());
        cp->setEnabled(!view_->currentImage().isNull());
    };
    connect(view_, &TextureView::textureChanged, this, enable);
    enable();
    rebuildRecent();
    statusBar()->showMessage(tr("Open a .dtx file, or drop one here"));
}

bool TextureWindow::openFile(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("DTX Viewer"), tr("Can't open %1").arg(path));
        return false;
    }
    bool ok = openData(f.readAll(), QFileInfo(path).fileName());
    if (ok) addRecent(QFileInfo(path).absoluteFilePath());
    return ok;
}

bool TextureWindow::openData(const QByteArray& data, const QString& name) {
    QString err;
    bool ok = view_->loadDtx(data, name, &err);
    setWindowTitle(name.isEmpty() ? tr("DTX Viewer") : tr("%1 — DTX Viewer").arg(name));
    QImage img = view_->currentImage();
    statusBar()->showMessage(ok ? tr("%1×%2").arg(img.width()).arg(img.height()) : err);
    return ok;
}

void TextureWindow::openDialog() {
    QSettings s;
    QString dir = s.value(QStringLiteral("dtxview/lastDir")).toString();
    QString p = QFileDialog::getOpenFileName(this, tr("Open Texture"), dir, tr("LithTech textures (*.dtx *.DTX);;All files (*)"));
    if (p.isEmpty()) return;
    s.setValue(QStringLiteral("dtxview/lastDir"), QFileInfo(p).absolutePath());
    openFile(p);
}

void TextureWindow::addRecent(const QString& path) {
    QSettings s;
    QStringList r = s.value(QStringLiteral("dtxview/recent")).toStringList();
    r.removeAll(path);
    r.prepend(path);
    while (r.size() > 8) r.removeLast();
    s.setValue(QStringLiteral("dtxview/recent"), r);
    rebuildRecent();
}

void TextureWindow::rebuildRecent() {
    recentMenu_->clear();
    QStringList r = QSettings().value(QStringLiteral("dtxview/recent")).toStringList();
    int n = 1;
    for (const QString& p : r)
        recentMenu_->addAction(QStringLiteral("&%1 %2").arg(n++).arg(QFileInfo(p).fileName()), this, [this, p] { openFile(p); })
            ->setToolTip(p);
    recentMenu_->setEnabled(!r.isEmpty());
}

void TextureWindow::dragEnterEvent(QDragEnterEvent* e) {
    if (e->mimeData()->hasUrls()) e->acceptProposedAction();
}

void TextureWindow::dropEvent(QDropEvent* e) {
    for (const QUrl& u : e->mimeData()->urls())
        if (u.isLocalFile()) {
            openFile(u.toLocalFile());
            break;
        }
}
