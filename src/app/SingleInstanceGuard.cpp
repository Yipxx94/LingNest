#include "app/SingleInstanceGuard.h"

#include <utility>

#include <QDir>
#include <QStandardPaths>

namespace lingnest::app {

SingleInstanceGuard::SingleInstanceGuard(QString lockFilePath)
    : m_lockFile(std::move(lockFilePath))
{
}

QString SingleInstanceGuard::defaultLockFilePath()
{
    QString temporaryDirectory =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (temporaryDirectory.isEmpty()) {
        temporaryDirectory = QDir::tempPath();
    }

    return QDir(temporaryDirectory).filePath(
        QStringLiteral("lingnest-single-instance.lock"));
}

bool SingleInstanceGuard::tryAcquire()
{
    if (!m_acquired) {
        m_acquired = m_lockFile.tryLock(0);
    }
    return m_acquired;
}

bool SingleInstanceGuard::isAcquired() const noexcept
{
    return m_acquired;
}

} // namespace lingnest::app
