#include "bankview.h"

#include "soundview.h"

#include <lithtech/sound.hpp>

#include <QDir>

#include <algorithm>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>

using namespace lithtech;

namespace {

QString seconds(double s) {
    int m = int(s / 60);
    return QStringLiteral("%1:%2").arg(m).arg(s - m * 60, 5, 'f', 2, QChar('0'));
}

QString bytes(qint64 n) { return QLocale().formattedDataSize(n, 1, QLocale::DataSizeTraditionalFormat); }

// Rows sort by their numbers where they have them.
class Row : public QTreeWidgetItem {
public:
    using QTreeWidgetItem::QTreeWidgetItem;
    bool operator<(const QTreeWidgetItem& o) const override {
        int c = treeWidget() ? treeWidget()->sortColumn() : 0;
        QVariant a = data(c, Qt::UserRole), b = o.data(c, Qt::UserRole);
        if (a.isValid() && b.isValid()) return a.toDouble() < b.toDouble();
        return text(c).compare(o.text(c), Qt::CaseInsensitive) < 0;
    }
};

}  // namespace

struct BankView::Impl {
    sound::Bank bank;
    QString name;
    QLabel* summary = nullptr;
    QTreeWidget* list = nullptr;
    QPushButton *save = nullptr, *saveAll = nullptr;
    SoundView* player = nullptr;
    int current = -1;
    std::vector<double> lengths;   // seconds, as decoded (to the END frame)
};

BankView::BankView(QWidget* parent) : QWidget(parent), d(new Impl) {
    auto* top = new QVBoxLayout(this);
    top->setContentsMargins(0, 0, 0, 0);
    d->summary = new QLabel(this);
    d->summary->setWordWrap(true);
    d->summary->setTextInteractionFlags(Qt::TextSelectableByMouse);
    top->addWidget(d->summary);

    auto* bar = new QHBoxLayout();
    d->save = new QPushButton(tr("Save as WAV..."), this);
    d->save->setToolTip(tr("Save the selected sounds as 16-bit PCM WAVs, with their loops"));
    d->saveAll = new QPushButton(tr("Save All..."), this);
    d->saveAll->setToolTip(tr("Save every sound in the bank as a WAV, in folders by group"));
    bar->addWidget(d->save);
    bar->addWidget(d->saveAll);
    bar->addStretch(1);
    top->addLayout(bar);

    auto* split = new QSplitter(Qt::Vertical, this);
    d->list = new QTreeWidget(split);
    d->list->setRootIsDecorated(false);
    d->list->setUniformRowHeights(true);
    d->list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    d->list->setSortingEnabled(true);
    d->list->setHeaderLabels({tr("#"), tr("Name"), tr("Group"), tr("Rate"), tr("Length"), tr("Loop"), tr("Lip-sync")});
    d->list->header()->setStretchLastSection(false);
    d->list->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    d->player = new SoundView(split);
    split->addWidget(d->list);
    split->addWidget(d->player);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 2);
    top->addWidget(split, 1);

    connect(d->list, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* it) {
        if (it) selectSound(it->data(0, Qt::UserRole).toInt());
    });
    connect(d->save, &QPushButton::clicked, this, [this] { saveSelected(false); });
    connect(d->saveAll, &QPushButton::clicked, this, [this] { saveSelected(true); });
    clear();
}

BankView::~BankView() { delete d; }

void BankView::clear() {
    d->player->clear();
    d->list->clear();
    d->bank = {};
    d->lengths.clear();
    d->current = -1;
    d->summary->clear();
    d->save->setEnabled(false);
    d->saveAll->setEnabled(false);
}

void BankView::stop() { d->player->stop(); }
int BankView::soundCount() const { return int(d->bank.sounds.size()); }
SoundView* BankView::player() const { return d->player; }

bool BankView::loadBank(const QByteArray& data, const QString& name, QString* error) {
    clear();
    try {
        d->bank = sound::read_bank(reinterpret_cast<const uint8_t*>(data.constData()), size_t(data.size()));
    } catch (const std::exception& e) {
        if (error) *error = tr("Can't read this sound bank: %1").arg(QString::fromUtf8(e.what()));
        return false;
    }
    d->name = name;
    const sound::Bank& b = d->bank;
    int loops = 0, lips = 0;
    double total = 0;
    for (auto& s : b.sounds) {
        loops += s.looping();
        lips += s.lip_count > 0;
        double len = s.rate ? double(sound::decode_ps_adpcm(b.adpcm(s), s.size).size()) / s.rate : 0;
        d->lengths.push_back(len);
        total += len;
    }
    QString kind = b.spu_image.empty() ? tr("effect bank") : tr("voice bank");
    QStringList parts;
    parts << tr("<b>%1</b>: PS2 %2, \"%3\"").arg(name.toHtmlEscaped(), kind, QString::fromStdString(b.version));
    parts << tr("%1 sounds (%2) in %3 groups").arg(b.sounds.size()).arg(seconds(total)).arg(b.groups.size());
    if (loops) parts << tr("%1 looping").arg(loops);
    parts << tr("PS-ADPCM %1").arg(bytes(qint64(b.data.size())));
    if (!b.lip.empty()) parts << tr("lip-sync for %1 sounds (%2)").arg(lips).arg(bytes(qint64(b.lip.size())));
    if (!b.spu_image.empty()) parts << tr("SPU preload image %1").arg(bytes(qint64(b.spu_image.size())));
    d->summary->setText(parts.join(QStringLiteral(" · ")));

    d->list->setSortingEnabled(false);
    for (std::size_t i = 0; i < b.sounds.size(); ++i) {
        const auto& s = b.sounds[i];
        auto* it = new Row(d->list);
        it->setText(0, QString::number(i));
        it->setData(0, Qt::UserRole, int(i));
        it->setText(1, QString::fromStdString(s.name));
        it->setText(2, QString::fromStdString(s.group));
        it->setText(3, tr("%1 Hz").arg(s.rate));
        it->setData(3, Qt::UserRole, s.rate);
        it->setText(4, seconds(d->lengths[i]));
        it->setData(4, Qt::UserRole, d->lengths[i]);
        it->setText(5, s.looping() ? tr("loop") : QString());
        it->setText(6, s.lip_count ? tr("%1 bytes").arg(s.lip_count) : QString());
        it->setData(6, Qt::UserRole, s.lip_count);
        it->setToolTip(1, QString::fromStdString(s.path));
    }
    d->list->setSortingEnabled(true);
    d->list->sortByColumn(0, Qt::AscendingOrder);
    d->save->setEnabled(!b.sounds.empty());
    d->saveAll->setEnabled(!b.sounds.empty());
    if (!b.sounds.empty()) d->list->setCurrentItem(d->list->topLevelItem(0));
    return true;
}

void BankView::selectSound(int index) {
    if (index < 0 || index >= soundCount()) return;
    if (index != d->current) {
        for (int r = 0; r < d->list->topLevelItemCount(); ++r)
            if (d->list->topLevelItem(r)->data(0, Qt::UserRole).toInt() == index && d->list->currentItem() != d->list->topLevelItem(r)) {
                d->list->setCurrentItem(d->list->topLevelItem(r));
                return;   // currentItemChanged comes back here
            }
    }
    d->current = index;
    const sound::BankSound& s = d->bank.sounds[std::size_t(index)];
    uint32_t ls = 0, le = 0;
    sound::Pcm pcm;
    try {
        pcm = sound::decode_bank_sound(d->bank, std::size_t(index), &ls, &le);
    } catch (const std::exception&) {
        d->player->clear();
        return;
    }
    QString t;
    auto head = [&t](const QString& h) { t += (t.isEmpty() ? "" : "\n") + h + "\n"; };
    auto row = [&t](const QString& k, const QString& v) { t += QStringLiteral("  %1 %2\n").arg(k, -14).arg(v); };
    t += QString::fromStdString(s.path) + "\n";
    head(tr("Format"));
    row(tr("Codec"), tr("PS-ADPCM (PS2 SPU)"));
    row(tr("Channels"), tr("mono"));
    row(tr("Sample rate"), tr("%1 Hz").arg(s.rate));
    row(tr("Duration"), seconds(pcm.seconds()) + tr(" (%1 frames)").arg(pcm.frames()));
    row(tr("Data"), tr("%1 bytes (%2 ADPCM blocks)").arg(s.size).arg(s.size / 16));
    if (s.size) row(tr("Compression"), tr("%1 : 1 against 16-bit PCM").arg(double(pcm.frames()) * 2 / s.size, 0, 'f', 1));
    head(tr("PS2 bank"));
    row(tr("Bank"), d->name);
    row(tr("Sound"), tr("#%1 of %2").arg(index).arg(soundCount()));
    row(tr("Group"), s.group.empty() ? tr("(root)") : QString::fromStdString(s.group));
    if (le > ls) row(tr("Loop"), tr("%1 to %2 (frames %3 to %4)").arg(seconds(double(ls) / s.rate), seconds(double(le) / s.rate)).arg(ls).arg(le));
    else row(tr("Loop"), tr("none (one-shot)"));
    row(tr("Flags"), QStringLiteral("0x%1").arg(s.flags, 0, 16));
    row(tr("Play size"), tr("%1 bytes").arg(s.play_size));
    if (s.looping()) row(tr("Loop start"), tr("byte %1").arg(s.loop_start));
    row(tr("Offset"), tr("%1 in the bank's data").arg(s.offset));
    if (s.lip_count)
        row(tr("Lip-sync"), tr("%1 bytes, %2 a second").arg(s.lip_count).arg(pcm.seconds() > 0 ? s.lip_count / pcm.seconds() : 0, 0, 'f', 1));
    row(tr("SPU RAM"), tr("0x%1, %2 bytes resident%3")
                           .arg(s.spu_addr, 0, 16)
                           .arg(s.spu_size)
                           .arg(s.size > s.spu_size ? tr(" (the rest streams)") : QString()));
    if (!d->bank.spu_image.empty()) row(tr("SPU image"), tr("holds its head"));
    d->player->loadPcm(std::move(pcm), QString::fromStdString(s.name), t, le > ls ? qint64(ls) : -1, le > ls ? qint64(le) : -1);
}

// Writes each chosen sound as a PCM WAV: one asks for a file name; several,
// or all, go under a folder by their group paths.
void BankView::saveSelected(bool all) {
    std::vector<int> pick;
    if (all) {
        for (int i = 0; i < soundCount(); ++i) pick.push_back(i);
    } else {
        for (auto* it : d->list->selectedItems()) pick.push_back(it->data(0, Qt::UserRole).toInt());
        std::sort(pick.begin(), pick.end());
    }
    if (pick.empty()) return;
    QSettings settings;
    QString dir = settings.value("soundSaveDir").toString();
    auto write = [this](int i, const QString& file) {
        uint32_t ls = 0, le = 0;
        auto pcm = sound::decode_bank_sound(d->bank, std::size_t(i), &ls, &le);
        auto wav = sound::write_wav(pcm, ls, le);
        QDir().mkpath(QFileInfo(file).absolutePath());
        QFile f(file);
        return f.open(QIODevice::WriteOnly) && f.write(reinterpret_cast<const char*>(wav.data()), qint64(wav.size())) == qint64(wav.size());
    };
    if (pick.size() == 1 && !all) {
        const auto& s = d->bank.sounds[std::size_t(pick[0])];
        QString file = QFileDialog::getSaveFileName(this, tr("Save Sound"), QDir(dir).filePath(QString::fromStdString(s.name)),
                                                    tr("Sounds (*.wav)"));
        if (file.isEmpty()) return;
        settings.setValue("soundSaveDir", QFileInfo(file).absolutePath());
        if (!write(pick[0], file)) QMessageBox::warning(this, tr("Save Sound"), tr("Couldn't write %1.").arg(file));
        return;
    }
    QString out = QFileDialog::getExistingDirectory(this, tr("Save Sounds to"), dir);
    if (out.isEmpty()) return;
    settings.setValue("soundSaveDir", out);
    int failed = 0;
    for (int i : pick) {
        QString rel = QString::fromStdString(d->bank.sounds[std::size_t(i)].path).replace('\\', '/');
        failed += !write(i, QDir(out).filePath(rel));
    }
    if (failed) QMessageBox::warning(this, tr("Save Sounds"), tr("%1 of %2 sounds couldn't be written.").arg(failed).arg(pick.size()));
}
