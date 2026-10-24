#include "wizard.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QWizardPage>

namespace {
enum Pages { PageAction, PageGame, PageItems };

// Games, in WinRez's order.
const char* kGames[] = {"Shogo / Blood II (LithTech 1.x)", "No One Lives Forever (LithTech 2.x)",
                        "Aliens versus Predator 2 / Global Operations / Purge (LithTech Talon)",
                        "No One Lives Forever 2 / TRON 2.0 (LithTech Jupiter)"};
}  // namespace

QString RezWizard::itemName(Item i) {
    switch (i) {
        case CharSkins: return QObject::tr("New Character Skins");
        case CharModels: return QObject::tr("New Character Models");
        case WeaponSkins: return QObject::tr("New Weapon Skins");
        case WeaponModels: return QObject::tr("New Weapon Models");
        case DeathMatch: return QObject::tr("DeathMatch Maps");
        case TeamAssault: return QObject::tr("Team Assault Maps");
        case Cooperative: return QObject::tr("Cooperative Maps");
        case DoomsDay: return QObject::tr("DoomsDay Maps");
        case SinglePlayer: return QObject::tr("Singleplayer Maps");
        case Attributes: return QObject::tr("New Attribute Files");
        default: return {};
    }
}

QString RezWizard::folderFor(int game, Item i) {
    static const char* table[4][ItemCount] = {
        // skins, models, weapon skins, weapon models, DM, team, coop, doomsday, SP, attributes
        {"Skins\\Characters", "Models\\Characters", "Skins\\Weapons", "Models\\PV_Weapons", "Worlds\\Multi", "", "", "",
         "Worlds", ""},
        {"Chars\\Skins", "Chars\\Models", "Guns\\Skins_PV", "Guns\\Models_PV", "Worlds\\DeathMatch", "Worlds\\AssaultMap",
         "", "", "Worlds", "Attributes"},
        {"Skins\\Chars", "Models\\Chars", "Skins\\Weapons", "Models\\Weapons", "Worlds\\Multi\\DM", "Worlds\\Multi\\Evac",
         "Worlds\\Multi\\Overrun", "", "Worlds\\SinglePlayer", "Attributes"},
        {"Chars\\Skins", "Chars\\Models", "Guns\\Skins_PV", "Guns\\Models_PV", "Worlds\\RetailMultiplayer",
         "", "Worlds\\RetailMultiplayer", "Worlds\\RetailMultiplayer", "Worlds\\RetailSinglePlayer", "Attributes"},
    };
    if (game < 0 || game > 3) return {};
    return QString::fromLatin1(table[game][i]);
}

RezWizard::RezWizard(QWidget* parent) : QWizard(parent) {
    setWindowTitle(tr("Rez Wizard"));
    setWizardStyle(QWizard::ModernStyle);

    auto* p1 = new QWizardPage;
    p1->setTitle(tr("What would you like to do?"));
    auto* l1 = new QVBoxLayout(p1);
    action_ = new QButtonGroup(p1);
    const QString acts[] = {tr("I want to create a REZ file by adding some files."),
                            tr("I want to create a REZ file by adding some folders."),
                            tr("I want to create a REZ for my Skin, Model or Map."),
                            tr("I want to create a REZ for my Skin, Model or Map. I already have all the files in a "
                               "separate folder with the correct folder structure."),
                            tr("I want to extract the opened REZ file.")};
    for (int i = 0; i < 5; ++i) {
        auto* rb = new QRadioButton(acts[i], p1);
        action_->addButton(rb, i);
        l1->addWidget(rb);
        if (i == 0) rb->setChecked(true);
    }
    setPage(PageAction, p1);

    auto* p2 = new QWizardPage;
    p2->setTitle(tr("My REZ file will be used by:"));
    auto* l2 = new QVBoxLayout(p2);
    game_ = new QButtonGroup(p2);
    for (int i = 0; i < 4; ++i) {
        auto* rb = new QRadioButton(tr(kGames[i]), p2);
        game_->addButton(rb, i);
        l2->addWidget(rb);
        if (i == 1) rb->setChecked(true);
    }
    auto* note = new QLabel(tr("If your title isn't listed, choose by the LithTech version it uses. Talon games differ "
                               "a lot in their folders: check the result and adjust it by hand if needed."), p2);
    note->setWordWrap(true);
    l2->addWidget(note);
    setPage(PageGame, p2);

    auto* p3 = new QWizardPage;
    p3->setTitle(tr("I want to add the following to my REZ file:"));
    p3->setSubTitle(tr("You can pick several files for each item on the next screens."));
    auto* l3 = new QVBoxLayout(p3);
    for (int i = 0; i < ItemCount; ++i) {
        auto* cb = new QCheckBox(itemName(Item(i)), p3);
        items_.push_back(cb);
        l3->addWidget(cb);
    }
    setPage(PageItems, p3);
    connect(this, &QWizard::currentIdChanged, this, [this](int id) {
        if (id != PageItems) return;
        int g = game_->checkedId();
        for (int i = 0; i < ItemCount; ++i) {
            bool has = !folderFor(g, Item(i)).isEmpty();
            items_[size_t(i)]->setEnabled(has);
            if (!has) items_[size_t(i)]->setChecked(false);
            items_[size_t(i)]->setToolTip(has ? tr("Goes in %1").arg(folderFor(g, Item(i))) : tr("Not used by this game"));
        }
        if (g == 2)
            QMessageBox::information(this, tr("Rez Wizard"),
                                     tr("Talon games use different folder structures. Check that the folders made "
                                        "suit your game, or make them by hand."));
    });
}

int RezWizard::nextId() const {
    switch (currentId()) {
        case PageAction: return action_->checkedId() == 2 ? PageGame : -1;
        case PageGame: return PageItems;
        default: return -1;
    }
}

bool RezWizard::validateCurrentPage() {
    int id = currentId();
    if (id == PageAction && action_->checkedId() != 2) {
        switch (action_->checkedId()) {
            case 0:
                result_.action = AddFiles;
                result_.paths = QFileDialog::getOpenFileNames(this, tr("Files to add"));
                return !result_.paths.isEmpty();
            case 1: {
                result_.action = AddFolders;
                QString d = QFileDialog::getExistingDirectory(this, tr("Folder to add"));
                if (d.isEmpty()) return false;
                result_.paths = {d};
                return true;
            }
            case 3: {
                result_.action = ProjectFolder;
                QString d = QFileDialog::getExistingDirectory(this, tr("Your project's root folder"));
                if (d.isEmpty()) return false;
                result_.paths = {d};
                return true;
            }
            case 4:
                result_.action = ExtractOpen;
                return QMessageBox::question(this, tr("Rez Wizard"), tr("Extract the open REZ file?")) == QMessageBox::Yes;
        }
    }
    if (id == PageItems) {
        result_.action = GameLayout;
        result_.placed.clear();
        int g = game_->checkedId();
        for (int i = 0; i < ItemCount; ++i) {
            if (!items_[size_t(i)]->isChecked()) continue;
            Item it = Item(i);
            QString filter;
            switch (it) {
                case CharSkins:
                case WeaponSkins: filter = tr("Skins (*.dtx *.DTX)"); break;
                case CharModels:
                case WeaponModels: filter = tr("Models (*.abc *.ltb *.ABC *.LTB)"); break;
                case Attributes: filter = tr("Attributes (*.txt *.TXT)"); break;
                default: filter = tr("Maps (*.dat *.DAT)"); break;
            }
            QStringList files = QFileDialog::getOpenFileNames(this, itemName(it), {}, filter);
            if (files.isEmpty()) continue;
            if (g == 3) {
                // Jupiter tells map types apart by name.
                const char* prefix = it == DeathMatch ? "DM_" : it == Cooperative ? "CP_" : it == DoomsDay ? "DD_" : nullptr;
                if (prefix) {
                    QStringList bad;
                    for (const QString& f : files)
                        if (!QFileInfo(f).fileName().startsWith(prefix, Qt::CaseInsensitive)) bad << QFileInfo(f).fileName();
                    if (!bad.isEmpty())
                        QMessageBox::warning(this, tr("Rez Wizard"),
                                             tr("%1 names should start with \"%2\":\n%3").arg(itemName(it), prefix, bad.join("\n")));
                }
            }
            result_.placed.emplace_back(folderFor(g, it), files);
        }
        return !result_.placed.empty();
    }
    return true;
}
