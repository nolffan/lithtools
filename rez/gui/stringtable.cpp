// The string table editor (ResEdit's job): a resource DLL's RT_STRING
// strings in a table, edited in place and written back through
// lithtech::pe, which rebuilds the resource section.

#include "stringtable.h"

#include <lithtech/pe_strings.hpp>

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QTableWidget>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

#include <map>
#include <optional>

namespace {

QString fromU16(const std::u16string& s) {
    return QString::fromUtf16(s.data(), qsizetype(s.size()));
}
std::u16string toU16(const QString& s) {
    return std::u16string(reinterpret_cast<const char16_t*>(s.utf16()), std::size_t(s.size()));
}

// Strings are shown on one line with \n, \r, \t and \\ escaped.
QString escape(const QString& s) {
    QString o;
    o.reserve(s.size());
    for (QChar c : s) {
        if (c == QLatin1Char('\\')) o += QLatin1String("\\\\");
        else if (c == QLatin1Char('\n')) o += QLatin1String("\\n");
        else if (c == QLatin1Char('\r')) o += QLatin1String("\\r");
        else if (c == QLatin1Char('\t')) o += QLatin1String("\\t");
        else o += c;
    }
    return o;
}
QString unescape(const QString& s) {
    QString o;
    for (int i = 0; i < s.size(); ++i) {
        QChar c = s[i];
        if (c == QLatin1Char('\\') && i + 1 < s.size()) {
            QChar n = s[i + 1];
            if (n == QLatin1Char('n')) { o += QLatin1Char('\n'); ++i; continue; }
            if (n == QLatin1Char('r')) { o += QLatin1Char('\r'); ++i; continue; }
            if (n == QLatin1Char('t')) { o += QLatin1Char('\t'); ++i; continue; }
            if (n == QLatin1Char('\\')) { o += QLatin1Char('\\'); ++i; continue; }
        }
        o += c;
    }
    return o;
}

const int kMaxRecent = 4;

}  // namespace

struct StringTableWindow::Impl {
    StringTableWindow* w = nullptr;
    QTableWidget* table = nullptr;
    QComboBox* lang = nullptr;
    QLabel* langLabel = nullptr;
    QAction *saveAct = nullptr, *saveAsAct = nullptr, *insertAct = nullptr, *insertIdAct = nullptr,
            *deleteAct = nullptr, *findAct = nullptr, *findNextAct = nullptr, *editAct = nullptr;
    QMenu* recentMenu = nullptr;

    std::optional<lithtech::pe::Image> image;
    std::map<uint16_t, lithtech::pe::StringTable> tables;  // edited copies
    uint16_t current = 0;
    QString path;      // on disk, or empty
    QString name;      // shown in the title
    std::function<bool(const QByteArray&)> saveBack;
    QString findText;
    bool filling = false;

    void retitle() {
        w->setWindowTitle(QStringLiteral("%1[*] - String Table Editor").arg(name.isEmpty() ? QStringLiteral("(none)") : name));
    }

    void setModified(bool m) {
        w->setWindowModified(m);
        saveAct->setEnabled(image.has_value() && m);
    }

    void enable() {
        bool on = image.has_value();
        saveAsAct->setEnabled(on && !saveBack);
        insertAct->setEnabled(on);
        insertIdAct->setEnabled(on);
        deleteAct->setEnabled(on);
        findAct->setEnabled(on);
        findNextAct->setEnabled(on);
        editAct->setEnabled(on);
    }

    void fill() {
        filling = true;
        table->setSortingEnabled(false);
        table->clearContents();
        const auto& t = tables[current];
        table->setRowCount(int(t.size()));
        int row = 0;
        for (auto& [id, s] : t) {
            auto* idItem = new QTableWidgetItem;
            idItem->setData(Qt::DisplayRole, int(id));
            idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
            table->setItem(row, 0, idItem);
            table->setItem(row, 1, new QTableWidgetItem(escape(fromU16(s))));
            ++row;
        }
        table->setSortingEnabled(true);
        table->sortByColumn(0, Qt::AscendingOrder);
        filling = false;
        w->statusBar()->showMessage(QStringLiteral("%1 strings").arg(t.size()));
    }

    bool load(const QByteArray& bytes, const QString& title) {
        try {
            auto img = lithtech::pe::Image::parse(std::vector<uint8_t>(bytes.begin(), bytes.end()));
            image = std::move(img);
        } catch (const std::exception& e) {
            QMessageBox::warning(w, QStringLiteral("String Table Editor"),
                                 QStringLiteral("Can't read %1: %2").arg(title, QString::fromUtf8(e.what())));
            return false;
        }
        tables.clear();
        lang->blockSignals(true);
        lang->clear();
        auto langs = image->string_languages();
        for (auto l : langs) {
            tables[l] = image->strings(l);
            lang->addItem(QString::fromStdString(lithtech::pe::language_name(l)), int(l));
        }
        if (langs.empty()) {
            // No strings yet: start an English (United States) table.
            tables[0x0409] = {};
            lang->addItem(QString::fromStdString(lithtech::pe::language_name(0x0409)), 0x0409);
        }
        lang->blockSignals(false);
        bool several = lang->count() > 1;
        lang->setVisible(several);
        langLabel->setVisible(several);
        current = uint16_t(lang->itemData(0).toInt());
        name = title;
        retitle();
        fill();
        setModified(false);
        enable();
        return true;
    }

    // Write the edits into the image and return the DLL's bytes.
    std::optional<QByteArray> build() {
        if (!image) return std::nullopt;
        try {
            for (auto& [l, t] : tables) image->set_strings(l, t);
            lithtech::pe::WriteInfo info;
            auto out = image->write(&info);
            if (!info.warnings.empty()) {
                QString msg;
                for (auto& s : info.warnings) msg += QString::fromUtf8(s.c_str()) + QLatin1Char('\n');
                QMessageBox::information(w, QStringLiteral("String Table Editor"), msg.trimmed());
            }
            return QByteArray(reinterpret_cast<const char*>(out.data()), qsizetype(out.size()));
        } catch (const std::exception& e) {
            QMessageBox::warning(w, QStringLiteral("String Table Editor"),
                                 QStringLiteral("Can't write the DLL: %1").arg(QString::fromUtf8(e.what())));
            return std::nullopt;
        }
    }

    bool saveTo(const QString& target) {
        auto bytes = build();
        if (!bytes) return false;
        QFile f(target);
        if (!f.open(QIODevice::WriteOnly) || f.write(*bytes) != bytes->size()) {
            QMessageBox::warning(w, QStringLiteral("String Table Editor"), QStringLiteral("Can't write %1").arg(target));
            return false;
        }
        f.close();
        path = target;
        name = QFileInfo(target).fileName();
        load(*bytes, name);
        addRecent(target);
        return true;
    }

    bool save() {
        if (saveBack) {
            auto bytes = build();
            if (!bytes || !saveBack(*bytes)) return false;
            load(*bytes, name);
            return true;
        }
        if (path.isEmpty()) return saveAs();
        return saveTo(path);
    }

    bool saveAs() {
        QString p = QFileDialog::getSaveFileName(w, QStringLiteral("Save DLL As"), path,
                                                 QStringLiteral("Resource DLLs (*.dll);;All files (*)"));
        if (p.isEmpty()) return false;
        saveBack = nullptr;
        return saveTo(p);
    }

    bool maybeSave() {
        if (!w->isWindowModified()) return true;
        auto r = QMessageBox::question(w, QStringLiteral("String Table Editor"),
                                       QStringLiteral("%1 has unsaved changes. Save them?").arg(name),
                                       QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (r == QMessageBox::Save) return save();
        return r == QMessageBox::Discard;
    }

    void addRecent(const QString& p) {
        QSettings s(QStringLiteral("lithtools"), QStringLiteral("Rez Studio"));
        QStringList l = s.value(QStringLiteral("stringtable/recent")).toStringList();
        l.removeAll(p);
        l.prepend(p);
        while (l.size() > kMaxRecent) l.removeLast();
        s.setValue(QStringLiteral("stringtable/recent"), l);
        // Later: this may run from one of the menu's own actions.
        QTimer::singleShot(0, w, [this] { updateRecent(); });
    }

    void updateRecent() {
        recentMenu->clear();
        QSettings s(QStringLiteral("lithtools"), QStringLiteral("Rez Studio"));
        const QStringList l = s.value(QStringLiteral("stringtable/recent")).toStringList();
        int n = 1;
        for (const QString& p : l) {
            QAction* a = recentMenu->addAction(QStringLiteral("&%1 %2").arg(n++).arg(QFileInfo(p).fileName()));
            a->setToolTip(p);
            QObject::connect(a, &QAction::triggered, w, [this, p] {
                if (maybeSave()) w->openFile(p);
            });
        }
        recentMenu->setEnabled(!l.isEmpty());
    }

    int rowOf(uint32_t id) const {
        for (int r = 0; r < table->rowCount(); ++r)
            if (table->item(r, 0) && uint32_t(table->item(r, 0)->data(Qt::DisplayRole).toInt()) == id) return r;
        return -1;
    }

    void insert(uint32_t id, const QString& text) {
        auto& t = tables[current];
        if (id > 0xFFFF) return;
        t[id] = toU16(text.isEmpty() ? QStringLiteral("New string") : text);
        fill();
        setModified(true);
        int r = rowOf(id);
        if (r >= 0) {
            table->setCurrentCell(r, 1);
            table->scrollToItem(table->item(r, 1));
            table->editItem(table->item(r, 1));
        }
    }

    void insertNext() {
        auto& t = tables[current];
        uint32_t id = t.empty() ? 1 : t.rbegin()->first + 1;
        if (id > 0xFFFF) {
            QMessageBox::information(w, QStringLiteral("Insert"), QStringLiteral("The highest string id (65535) is taken."));
            return;
        }
        insert(id, QString());
    }

    void insertWithId() {
        auto& t = tables[current];
        bool ok = false;
        int id = QInputDialog::getInt(w, QStringLiteral("Insert String"), QStringLiteral("String id (0-65535):"),
                                      t.empty() ? 1 : int(std::min<uint32_t>(t.rbegin()->first + 1, 0xFFFF)), 0, 0xFFFF, 1, &ok);
        if (!ok) return;
        if (t.count(uint32_t(id))) {
            QMessageBox::information(w, QStringLiteral("Insert String"), QStringLiteral("String %1 already exists.").arg(id));
            int r = rowOf(uint32_t(id));
            if (r >= 0) table->setCurrentCell(r, 1);
            return;
        }
        insert(uint32_t(id), QString());
    }

    void remove() {
        auto rows = table->selectionModel()->selectedRows();
        if (rows.isEmpty() && table->currentRow() >= 0) rows.append(table->model()->index(table->currentRow(), 0));
        if (rows.isEmpty()) return;
        auto& t = tables[current];
        for (auto& ix : rows)
            if (auto* it = table->item(ix.row(), 0)) t.erase(uint32_t(it->data(Qt::DisplayRole).toInt()));
        int keep = std::min(rows.first().row(), int(t.size()) - 1);
        fill();
        setModified(true);
        if (keep >= 0) table->setCurrentCell(keep, 1);
    }

    void find(bool next) {
        if (!next || findText.isEmpty()) {
            bool ok = false;
            QString s = QInputDialog::getText(w, QStringLiteral("Find"), QStringLiteral("Find what:"), QLineEdit::Normal,
                                              findText, &ok);
            if (!ok || s.isEmpty()) return;
            findText = s;
        }
        int n = table->rowCount();
        int start = table->currentRow();
        for (int k = 1; k <= n; ++k) {
            int r = (start + k + n) % n;
            auto* it = table->item(r, 1);
            auto* idit = table->item(r, 0);
            if ((it && unescape(it->text()).contains(findText, Qt::CaseInsensitive)) ||
                (idit && idit->text() == findText)) {
                table->setCurrentCell(r, 1);
                table->scrollToItem(it);
                return;
            }
        }
        w->statusBar()->showMessage(QStringLiteral("\"%1\" not found").arg(findText), 4000);
    }

    // A multi-line editor for the current string.
    void editInDialog() {
        int r = table->currentRow();
        if (r < 0) return;
        auto* idit = table->item(r, 0);
        auto* it = table->item(r, 1);
        if (!idit || !it) return;
        QDialog dlg(w);
        dlg.setWindowTitle(QStringLiteral("String %1").arg(idit->text()));
        auto* lay = new QVBoxLayout(&dlg);
        auto* ed = new QPlainTextEdit(&dlg);
        ed->setPlainText(unescape(it->text()));
        lay->addWidget(ed);
        auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
        QObject::connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        QObject::connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        lay->addWidget(bb);
        dlg.resize(560, 300);
        if (dlg.exec() == QDialog::Accepted) it->setText(escape(ed->toPlainText()));
    }
};

StringTableWindow::StringTableWindow(QWidget* parent) : QMainWindow(parent), d(new Impl) {
    d->w = this;
    setAttribute(Qt::WA_DeleteOnClose, false);

    d->table = new QTableWidget(0, 2, this);
    d->table->setHorizontalHeaderLabels({QStringLiteral("ID"), QStringLiteral("String")});
    d->table->horizontalHeader()->setStretchLastSection(true);
    d->table->verticalHeader()->setVisible(false);
    d->table->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed |
                              QAbstractItemView::AnyKeyPressed);
    d->table->setColumnWidth(0, 70);
    d->table->setWordWrap(false);
    d->table->verticalHeader()->setDefaultSectionSize(d->table->fontMetrics().height() + 6);

    auto* central = new QWidget(this);
    auto* lay = new QVBoxLayout(central);
    lay->setContentsMargins(0, 0, 0, 0);
    auto* top = new QHBoxLayout;
    top->setContentsMargins(6, 6, 6, 0);
    d->langLabel = new QLabel(QStringLiteral("Language:"), central);
    d->lang = new QComboBox(central);
    top->addWidget(d->langLabel);
    top->addWidget(d->lang);
    top->addStretch();
    lay->addLayout(top);
    lay->addWidget(d->table);
    setCentralWidget(central);
    d->langLabel->hide();
    d->lang->hide();

    QMenu* file = menuBar()->addMenu(QStringLiteral("&File"));
    QAction* openAct = file->addAction(QStringLiteral("&Open..."), this, [this] {
        if (!d->maybeSave()) return;
        QString p = QFileDialog::getOpenFileName(this, QStringLiteral("Open Resource DLL"), d->path,
                                                 QStringLiteral("Resource DLLs (*.dll);;All files (*)"));
        if (!p.isEmpty()) openFile(p);
    });
    openAct->setShortcut(QKeySequence::Open);
    d->saveAct = file->addAction(QStringLiteral("&Save"), this, [this] { d->save(); });
    d->saveAct->setShortcut(QKeySequence::Save);
    d->saveAsAct = file->addAction(QStringLiteral("Save &As..."), this, [this] { d->saveAs(); });
    d->saveAsAct->setShortcut(QKeySequence::SaveAs);
    file->addSeparator();
    d->recentMenu = file->addMenu(QStringLiteral("Recent &Files"));
    file->addSeparator();
    file->addAction(QStringLiteral("&Close"), this, &QWidget::close)->setShortcut(QKeySequence::Close);

    QMenu* edit = menuBar()->addMenu(QStringLiteral("&Edit"));
    d->insertAct = edit->addAction(QStringLiteral("&Insert"), this, [this] { d->insertNext(); });
    d->insertAct->setShortcut(Qt::Key_Insert);
    d->insertAct->setToolTip(QStringLiteral("Append a new string after the highest id"));
    d->insertIdAct = edit->addAction(QStringLiteral("Insert with &ID..."), this, [this] { d->insertWithId(); });
    d->insertIdAct->setShortcut(Qt::CTRL | Qt::Key_Insert);
    d->deleteAct = edit->addAction(QStringLiteral("&Delete"), this, [this] { d->remove(); });
    d->deleteAct->setShortcut(QKeySequence::Delete);
    edit->addSeparator();
    d->editAct = edit->addAction(QStringLiteral("&Edit String..."), this, [this] { d->editInDialog(); });
    d->editAct->setShortcut(Qt::CTRL | Qt::Key_E);
    edit->addSeparator();
    d->findAct = edit->addAction(QStringLiteral("&Find..."), this, [this] { d->find(false); });
    d->findAct->setShortcut(QKeySequence::Find);
    d->findNextAct = edit->addAction(QStringLiteral("Find &Next"), this, [this] { d->find(true); });
    d->findNextAct->setShortcut(QKeySequence::FindNext);

    QToolBar* tb = addToolBar(QStringLiteral("Main"));
    tb->setObjectName(QStringLiteral("stringtable-toolbar"));
    tb->addAction(openAct);
    tb->addAction(d->saveAct);
    tb->addSeparator();
    tb->addAction(d->insertAct);
    tb->addAction(d->deleteAct);

    connect(d->table, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* it) {
        if (d->filling || it->column() != 1) return;
        auto* idit = d->table->item(it->row(), 0);
        if (!idit) return;
        uint32_t id = uint32_t(idit->data(Qt::DisplayRole).toInt());
        QString s = unescape(it->text());
        auto& t = d->tables[d->current];
        if (s.isEmpty()) {
            // An empty string is no string.
            t.erase(id);
        } else {
            auto u = toU16(s);
            if (t[id] == u) return;
            t[id] = u;
        }
        d->setModified(true);
    });
    connect(d->lang, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int i) {
        if (i < 0) return;
        d->current = uint16_t(d->lang->itemData(i).toInt());
        d->fill();
    });

    d->retitle();
    d->setModified(false);
    d->enable();
    d->updateRecent();
    resize(760, 560);
}

StringTableWindow::~StringTableWindow() { delete d; }

bool StringTableWindow::openFile(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, QStringLiteral("String Table Editor"), QStringLiteral("Can't open %1").arg(path));
        return false;
    }
    QByteArray b = f.readAll();
    d->saveBack = nullptr;
    if (!d->load(b, QFileInfo(path).fileName())) return false;
    d->path = path;
    d->addRecent(path);
    return true;
}

bool StringTableWindow::openData(const QByteArray& data, const QString& name,
                                 std::function<bool(const QByteArray&)> saveBack) {
    d->saveBack = std::move(saveBack);
    d->path.clear();
    if (!d->load(data, name)) return false;
    d->enable();
    return true;
}

void StringTableWindow::closeEvent(QCloseEvent* e) {
    if (d->maybeSave()) e->accept();
    else e->ignore();
}
