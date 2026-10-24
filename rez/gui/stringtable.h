#pragma once
// String table editor for CRes.dll and other resource DLLs (WinRez LT
// Studio's ResEdit), cross-platform: the DLL's RT_STRING resources are read
// and written directly (lithtech/pe_strings.hpp).

#include <QByteArray>
#include <QMainWindow>

class QCloseEvent;
#include <functional>

class StringTableWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit StringTableWindow(QWidget* parent = nullptr);
    ~StringTableWindow() override;
    bool openFile(const QString& path);
    // A DLL from inside a rez: saveBack gets the edited DLL's bytes.
    bool openData(const QByteArray& data, const QString& name, std::function<bool(const QByteArray&)> saveBack);

protected:
    void closeEvent(QCloseEvent* e) override;

private:
    struct Impl;
    Impl* d;
};
