#include "util.h"

namespace util {
QString humanSize(quint64 b) {
    if (b < 1024) return QStringLiteral("%1 bytes").arg(b);
    if (b < 1024 * 1024) return QStringLiteral("%1 KB").arg(double(b) / 1024, 0, 'f', 1);
    if (b < 1024ull * 1024 * 1024) return QStringLiteral("%1 MB").arg(double(b) / (1024 * 1024), 0, 'f', 1);
    return QStringLiteral("%1 GB").arg(double(b) / (1024.0 * 1024 * 1024), 0, 'f', 2);
}
}  // namespace util
