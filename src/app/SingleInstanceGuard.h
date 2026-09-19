#pragma once

#include <QLockFile>
#include <QString>

namespace lingnest::app {

class SingleInstanceGuard final {
public:
    explicit SingleInstanceGuard(QString lockFilePath = defaultLockFilePath());

    [[nodiscard]] static QString defaultLockFilePath();
    [[nodiscard]] bool tryAcquire();
    [[nodiscard]] bool isAcquired() const noexcept;

private:
    QLockFile m_lockFile;
    bool m_acquired {false};
};

} // namespace lingnest::app
