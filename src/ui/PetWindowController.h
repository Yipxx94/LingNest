#pragma once

#include <QElapsedTimer>
#include <QPoint>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QVariantMap>

#include "character/CharacterDefinition.h"
#include "pet/PetState.h"

class QQuickWindow;
class QScreen;

namespace lingnest::config {
class ConfigManager;
}

namespace lingnest::ui {

class PetWindowController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(qreal scale READ scale CONSTANT)
    Q_PROPERTY(int dragThreshold READ dragThreshold CONSTANT)
    Q_PROPERTY(bool walking READ isWalking NOTIFY walkingChanged)

public:
    PetWindowController(
        character::CharacterDefinition character,
        config::ConfigManager* configManager,
        QObject* parent = nullptr);

    [[nodiscard]] QString displayName() const;
    [[nodiscard]] qreal scale() const noexcept;
    [[nodiscard]] int dragThreshold() const noexcept;
    [[nodiscard]] bool isWalking() const noexcept;

    void attachWindow(QQuickWindow* window);
    void restorePosition();
    void savePosition();
    void setWalkSpeedPixelsPerSecond(qreal speed);

    Q_INVOKABLE void beginDrag(qreal globalX, qreal globalY);
    Q_INVOKABLE bool startSystemDrag();
    Q_INVOKABLE void updateDrag(qreal globalX, qreal globalY);
    Q_INVOKABLE void updateDragFromCursor(
        qreal fallbackGlobalX,
        qreal fallbackGlobalY);
    Q_INVOKABLE bool endDrag();
    Q_INVOKABLE QVariantMap availableScreenGeometry() const;
    Q_INVOKABLE QVariantMap boundedPopupPosition(
        qreal preferredGlobalX,
        qreal preferredGlobalY,
        qreal popupWidth,
        qreal popupHeight,
        qreal margin = 10.0) const;

public slots:
    void startWalking(lingnest::pet::WalkDirection direction);
    void setWalkingDirection(lingnest::pet::WalkDirection direction);
    void stopWalking();

signals:
    void userActivity();
    void walkingChanged();
    void walkBoundaryTurned(lingnest::pet::WalkDirection direction);

private slots:
    void handleWalkTick();

private:
    [[nodiscard]] QScreen* screenByName(const QString& name) const;
    [[nodiscard]] QScreen* bestScreenFor(const QPoint& proposedPosition) const;
    [[nodiscard]] QPoint clampToScreen(
        const QPoint& proposedPosition,
        const QScreen* screen) const;

    character::CharacterDefinition m_character;
    config::ConfigManager* m_configManager {nullptr};
    QPointer<QQuickWindow> m_window;
    QTimer m_walkTimer;
    QElapsedTimer m_walkElapsed;
    QPoint m_dragOffset;
    QPoint m_dragStartWindowPosition;
    qreal m_walkPositionX {0.0};
    qreal m_walkSpeedPixelsPerSecond {48.0};
    pet::WalkDirection m_walkDirection {pet::WalkDirection::Right};
    bool m_dragging {false};
    bool m_dragMoved {false};
    bool m_systemMoveActive {false};
    bool m_walking {false};
};

} // namespace lingnest::ui
