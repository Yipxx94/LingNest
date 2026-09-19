#include "ui/PetWindowController.h"

#include <utility>

#include <QCursor>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QScreen>
#include <QStyleHints>
#include <QtGlobal>

#include "config/ConfigManager.h"

namespace lingnest::ui {
namespace {

constexpr int defaultScreenMargin = 24;

} // namespace

PetWindowController::PetWindowController(
    character::CharacterDefinition character,
    config::ConfigManager* configManager,
    QObject* parent)
    : QObject(parent)
    , m_character(std::move(character))
    , m_configManager(configManager)
{
    m_walkTimer.setInterval(16);
    m_walkTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_walkTimer, &QTimer::timeout,
        this, &PetWindowController::handleWalkTick);
}

QString PetWindowController::displayName() const
{
    return m_character.displayName;
}

qreal PetWindowController::scale() const noexcept
{
    return m_character.scale;
}

int PetWindowController::dragThreshold() const noexcept
{
    const QStyleHints* styleHints = QGuiApplication::styleHints();
    return styleHints != nullptr ? styleHints->startDragDistance() : 10;
}

bool PetWindowController::isWalking() const noexcept
{
    return m_walking;
}

void PetWindowController::attachWindow(QQuickWindow* window)
{
    m_window = window;
    if (!m_window.isNull()) {
        m_walkPositionX = m_window->x();
    }
}

void PetWindowController::restorePosition()
{
    if (m_window.isNull()) {
        return;
    }

    QScreen* targetScreen = QGuiApplication::primaryScreen();
    QPoint restoredPosition;
    bool hasStoredPosition = false;

    if (m_configManager != nullptr) {
        const auto placement = m_configManager->petWindowPlacement();
        if (placement.has_value()) {
            if (QScreen* storedScreen = screenByName(placement->screenName)) {
                targetScreen = storedScreen;
            }

            if (targetScreen != nullptr && placement->hasRelativePosition) {
                const QRect available = targetScreen->availableGeometry();
                const int horizontalTravel = qMax(0, available.width() - m_window->width());
                const int verticalTravel = qMax(0, available.height() - m_window->height());
                restoredPosition = QPoint(
                    available.left()
                        + qRound(placement->relativeX * static_cast<double>(horizontalTravel)),
                    available.top()
                        + qRound(placement->relativeY * static_cast<double>(verticalTravel)));
            } else {
                restoredPosition = placement->position;
            }
            hasStoredPosition = true;
        }
    }

    if (targetScreen == nullptr) {
        return;
    }

    if (!hasStoredPosition) {
        const QRect available = targetScreen->availableGeometry();
        restoredPosition = QPoint(
            available.right() - m_window->width() - defaultScreenMargin + 1,
            available.bottom() - m_window->height() - defaultScreenMargin + 1);
    }

    m_window->setPosition(clampToScreen(restoredPosition, targetScreen));
    m_walkPositionX = m_window->x();
}

void PetWindowController::setWalkSpeedPixelsPerSecond(qreal speed)
{
    m_walkSpeedPixelsPerSecond = qMax(1.0, speed);
}

void PetWindowController::savePosition()
{
    if (m_window.isNull() || m_configManager == nullptr) {
        return;
    }

    QScreen* targetScreen = QGuiApplication::screenAt(m_window->geometry().center());
    if (targetScreen == nullptr) {
        targetScreen = m_window->screen();
    }
    if (targetScreen == nullptr) {
        targetScreen = QGuiApplication::primaryScreen();
    }
    if (targetScreen == nullptr) {
        return;
    }

    const QRect available = targetScreen->availableGeometry();
    const int horizontalTravel = qMax(0, available.width() - m_window->width());
    const int verticalTravel = qMax(0, available.height() - m_window->height());
    const QPoint position = clampToScreen(m_window->position(), targetScreen);

    config::WindowPlacement placement;
    placement.screenName = targetScreen->name();
    placement.position = position;
    placement.relativeX = horizontalTravel > 0
        ? static_cast<double>(position.x() - available.left()) / horizontalTravel
        : 0.0;
    placement.relativeY = verticalTravel > 0
        ? static_cast<double>(position.y() - available.top()) / verticalTravel
        : 0.0;
    placement.hasRelativePosition = true;

    QString errorMessage;
    if (!m_configManager->savePetWindowPlacement(placement, &errorMessage)) {
        qWarning().noquote() << errorMessage;
    }
}

void PetWindowController::beginDrag(qreal globalX, qreal globalY)
{
    if (m_window.isNull()) {
        return;
    }

    stopWalking();
    const QPoint pointerPosition(qRound(globalX), qRound(globalY));
    m_dragOffset = pointerPosition - m_window->position();
    m_dragStartWindowPosition = m_window->position();
    m_dragging = true;
    m_dragMoved = false;
    m_systemMoveActive = false;

    emit userActivity();
}

bool PetWindowController::startSystemDrag()
{
    if (!m_dragging || m_window.isNull()) {
        return false;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    if (!m_systemMoveActive
        && !qEnvironmentVariableIsSet("LINGNEST_DISABLE_SYSTEM_MOVE")) {
        m_systemMoveActive = m_window->startSystemMove();
    }
#endif

    return m_systemMoveActive;
}

void PetWindowController::updateDrag(qreal globalX, qreal globalY)
{
    if (!m_dragging || m_window.isNull()) {
        return;
    }

    if (m_systemMoveActive) {
        m_walkPositionX = m_window->x();
        m_dragMoved = m_dragMoved
            || (m_window->position() - m_dragStartWindowPosition).manhattanLength()
                >= dragThreshold();
        return;
    }

    const QPoint pointerPosition(qRound(globalX), qRound(globalY));
    const QPoint proposedPosition = pointerPosition - m_dragOffset;
    QScreen* targetScreen = QGuiApplication::screenAt(pointerPosition);
    if (targetScreen == nullptr) {
        targetScreen = bestScreenFor(proposedPosition);
    }
    if (targetScreen != nullptr) {
        const QPoint nextPosition = clampToScreen(proposedPosition, targetScreen);
        if (nextPosition != m_window->position()) {
            m_window->setPosition(nextPosition);
        }
        m_walkPositionX = m_window->x();
        m_dragMoved = m_dragMoved
            || (nextPosition - m_dragStartWindowPosition).manhattanLength()
                >= dragThreshold();
    }
}

void PetWindowController::updateDragFromCursor(
    qreal fallbackGlobalX,
    qreal fallbackGlobalY)
{
    const QPoint pointerPosition = qEnvironmentVariableIsSet(
        "LINGNEST_DISABLE_SYSTEM_MOVE")
        ? QPoint(qRound(fallbackGlobalX), qRound(fallbackGlobalY))
        : QCursor::pos();
    updateDrag(pointerPosition.x(), pointerPosition.y());
}

bool PetWindowController::endDrag()
{
    if (!m_dragging) {
        return false;
    }

    if (!m_window.isNull()) {
        m_walkPositionX = m_window->x();
        m_dragMoved = m_dragMoved
            || (m_window->position() - m_dragStartWindowPosition).manhattanLength()
                >= dragThreshold();
    }
    m_dragging = false;
    m_systemMoveActive = false;
    if (m_dragMoved) {
        savePosition();
    }
    return m_dragMoved;
}

QVariantMap PetWindowController::boundedPopupPosition(
    qreal preferredGlobalX,
    qreal preferredGlobalY,
    qreal popupWidth,
    qreal popupHeight,
    qreal margin) const
{
    const QPoint preferredPosition(
        qRound(preferredGlobalX),
        qRound(preferredGlobalY));
    QScreen* targetScreen = QGuiApplication::screenAt(preferredPosition);
    if (targetScreen == nullptr && !m_window.isNull()) {
        targetScreen = m_window->screen();
    }
    if (targetScreen == nullptr) {
        targetScreen = QGuiApplication::primaryScreen();
    }
    if (targetScreen == nullptr) {
        return {
            {QStringLiteral("x"), preferredPosition.x()},
            {QStringLiteral("y"), preferredPosition.y()}
        };
    }

    const QRect available = targetScreen->availableGeometry();
    const int safeMargin = qMax(0, qRound(margin));
    const int width = qMax(1, qRound(popupWidth));
    const int height = qMax(1, qRound(popupHeight));
    const int minimumX = available.left() + safeMargin;
    const int minimumY = available.top() + safeMargin;
    const int maximumX = qMax(
        minimumX,
        available.right() - width - safeMargin + 1);
    const int maximumY = qMax(
        minimumY,
        available.bottom() - height - safeMargin + 1);

    return {
        {QStringLiteral("x"), qBound(minimumX, preferredPosition.x(), maximumX)},
        {QStringLiteral("y"), qBound(minimumY, preferredPosition.y(), maximumY)}
    };
}

void PetWindowController::startWalking(pet::WalkDirection direction)
{
    m_walkDirection = direction;
    if (!m_window.isNull()) {
        m_walkPositionX = m_window->x();
    }
    m_walkElapsed.start();
    m_walkTimer.start();
    if (!m_walking) {
        m_walking = true;
        emit walkingChanged();
    }
}

void PetWindowController::setWalkingDirection(pet::WalkDirection direction)
{
    m_walkDirection = direction;
}

void PetWindowController::stopWalking()
{
    m_walkTimer.stop();
    m_walkElapsed.invalidate();
    if (!m_walking) {
        return;
    }

    m_walking = false;
    savePosition();
    emit walkingChanged();
}

void PetWindowController::handleWalkTick()
{
    if (!m_walking || m_window.isNull()) {
        return;
    }

    const qint64 elapsedMs = qMin<qint64>(m_walkElapsed.restart(), 50);
    if (elapsedMs <= 0) {
        return;
    }

    QScreen* screen = QGuiApplication::screenAt(m_window->geometry().center());
    if (screen == nullptr) {
        screen = m_window->screen();
    }
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen == nullptr) {
        return;
    }

    const QRect available = screen->availableGeometry();
    const int minimumX = available.left();
    const int maximumX = qMax(
        minimumX,
        available.right() - m_window->width() + 1);
    const qreal directionSign = m_walkDirection == pet::WalkDirection::Left
        ? -1.0
        : 1.0;
    qreal nextX = m_walkPositionX
        + directionSign * m_walkSpeedPixelsPerSecond
            * static_cast<qreal>(elapsedMs) / 1000.0;

    bool turnedAtBoundary = false;
    if (nextX <= minimumX) {
        nextX = minimumX;
        if (m_walkDirection != pet::WalkDirection::Right) {
            m_walkDirection = pet::WalkDirection::Right;
            turnedAtBoundary = true;
        }
    } else if (nextX >= maximumX) {
        nextX = maximumX;
        if (m_walkDirection != pet::WalkDirection::Left) {
            m_walkDirection = pet::WalkDirection::Left;
            turnedAtBoundary = true;
        }
    }

    m_walkPositionX = nextX;
    m_window->setX(qRound(m_walkPositionX));
    if (turnedAtBoundary) {
        emit walkBoundaryTurned(m_walkDirection);
    }
}

QScreen* PetWindowController::screenByName(const QString& name) const
{
    for (QScreen* screen : QGuiApplication::screens()) {
        if (screen != nullptr && screen->name() == name) {
            return screen;
        }
    }
    return nullptr;
}

QScreen* PetWindowController::bestScreenFor(const QPoint& proposedPosition) const
{
    if (m_window.isNull()) {
        return QGuiApplication::primaryScreen();
    }

    const QRect proposedGeometry(proposedPosition, m_window->size());
    QScreen* bestScreen = nullptr;
    qint64 bestIntersectionArea = -1;

    for (QScreen* screen : QGuiApplication::screens()) {
        if (screen == nullptr) {
            continue;
        }

        const QRect intersection = proposedGeometry.intersected(screen->availableGeometry());
        const qint64 area = static_cast<qint64>(intersection.width()) * intersection.height();
        if (area > bestIntersectionArea) {
            bestIntersectionArea = area;
            bestScreen = screen;
        }
    }

    return bestScreen != nullptr ? bestScreen : QGuiApplication::primaryScreen();
}

QPoint PetWindowController::clampToScreen(
    const QPoint& proposedPosition,
    const QScreen* screen) const
{
    if (m_window.isNull() || screen == nullptr) {
        return proposedPosition;
    }

    const QRect available = screen->availableGeometry();
    const int maximumX = available.width() >= m_window->width()
        ? available.right() - m_window->width() + 1
        : available.left();
    const int maximumY = available.height() >= m_window->height()
        ? available.bottom() - m_window->height() + 1
        : available.top();

    return QPoint(
        qBound(available.left(), proposedPosition.x(), maximumX),
        qBound(available.top(), proposedPosition.y(), maximumY));
}

} // namespace lingnest::ui
