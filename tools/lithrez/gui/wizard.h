#pragma once
// The Rez Wizard (WinRez LT's): make a rez from files, folders, a project
// folder, or a mod's files placed in the right folders for its game; or
// extract the open one.

#include <QStringList>
#include <QWizard>

#include <utility>
#include <vector>

class QButtonGroup;
class QCheckBox;

class RezWizard : public QWizard {
    Q_OBJECT
public:
    enum Action { AddFiles, AddFolders, GameLayout, ProjectFolder, ExtractOpen };
    struct Result {
        Action action = AddFiles;
        QStringList paths;                                       // files, folders or the project
        std::vector<std::pair<QString, QStringList>> placed;     // folder in the rez -> files
    };

    explicit RezWizard(QWidget* parent = nullptr);
    const Result& result() const { return result_; }

    // The folder layouts, by engine generation (from WinRez LT 3.0).
    enum Item { CharSkins, CharModels, WeaponSkins, WeaponModels, DeathMatch, TeamAssault, Cooperative,
                DoomsDay, SinglePlayer, Attributes, ItemCount };
    static QString folderFor(int game, Item item);   // empty if the game has none
    static QString itemName(Item item);

    bool validateCurrentPage() override;
    int nextId() const override;

private:
    Result result_;
    QButtonGroup* action_;
    QButtonGroup* game_;
    std::vector<QCheckBox*> items_;
};
