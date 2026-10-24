// A PS2 sound bank (.lps): its sounds in a list (name, group, rate, length,
// loop, lip-sync), the selected one in a SoundView, and saving sounds as
// WAVs.

#pragma once

#include <QWidget>

class SoundView;

class BankView : public QWidget {
    Q_OBJECT
public:
    explicit BankView(QWidget* parent = nullptr);
    ~BankView() override;

    bool loadBank(const QByteArray& data, const QString& name, QString* error = nullptr);
    void clear();
    void stop();

    // For tests and scripting.
    int soundCount() const;
    void selectSound(int index);
    SoundView* player() const;

private:
    void saveSelected(bool all);
    struct Impl;
    Impl* d;
};
