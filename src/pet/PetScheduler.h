#pragma once

#include <QObject>
#include <QTimer>

#include "pet/PetStateMachine.h"

namespace lingnest::pet {

class PetScheduler final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int minimumIntervalMs READ minimumIntervalMs WRITE setMinimumIntervalMs)

public:
    explicit PetScheduler(PetStateMachine* stateMachine, QObject* parent = nullptr);

    [[nodiscard]] bool isEnabled() const noexcept;
    [[nodiscard]] int minimumIntervalMs() const noexcept;

    void start();
    void stop();
    void setEnabled(bool enabled);
    void setMinimumIntervalMs(int intervalMs);
    void setWalkIntervalRangeMs(int minimumMs, int maximumMs);
    void setDazeIntervalRangeMs(int minimumMs, int maximumMs);
    void setBlinkIntervalRangeMs(int minimumMs, int maximumMs);
    void setSleepAfterMs(int intervalMs);

public slots:
    void notifyUserActivity();

signals:
    void enabledChanged();

private slots:
    void handleWalkTimeout();
    void handleDazeTimeout();
    void handleBlinkTimeout();
    void handleSleepTimeout();

private:
    [[nodiscard]] static int randomInterval(int minimumMs, int maximumMs);
    void scheduleWalk();
    void scheduleDaze();
    void scheduleBlink();
    void scheduleSleep(int delayMs = -1);

    PetStateMachine* m_stateMachine {nullptr};
    QTimer m_walkTimer;
    QTimer m_dazeTimer;
    QTimer m_blinkTimer;
    QTimer m_sleepTimer;
    int m_walkMinimumMs {60000};
    int m_walkMaximumMs {120000};
    int m_dazeMinimumMs {30000};
    int m_dazeMaximumMs {90000};
    int m_blinkMinimumMs {3000};
    int m_blinkMaximumMs {7000};
    int m_sleepAfterMs {15 * 60 * 1000};
    bool m_enabled {true};
};

} // namespace lingnest::pet
