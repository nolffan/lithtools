#pragma once
// Small helpers shared by the Rez Studio windows.

#include <QByteArray>
#include <QString>
#include <string>
#include <vector>

namespace util {
inline QString qs(const std::string& s) { return QString::fromUtf8(s.data(), int(s.size())); }
inline std::string ss(const QString& s) { return s.toUtf8().toStdString(); }
inline QByteArray qb(const std::vector<uint8_t>& v) {
    return QByteArray(reinterpret_cast<const char*>(v.data()), int(v.size()));
}
inline std::vector<uint8_t> vb(const QByteArray& b) {
    return std::vector<uint8_t>(b.begin(), b.end());
}
QString humanSize(quint64 bytes);
}  // namespace util
