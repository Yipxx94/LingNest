#include "ui/SpeechBubbleController.h"

#include <QGuiApplication>
#include <QQuickWindow>
#include <QScreen>
#include <QWindow>
#include <QtGlobal>

namespace lingnest::ui {
namespace {

constexpr int bubbleGap = 8;

} // namespace

SpeechBubbleController::SpeechBubbleController(QObject* parent)
    : QObject(parent)
{
    m_hideTimer.setSingleShot(true);
    connect(&m_hideTimer, &QTimer::timeout, this, &SpeechBubbleController::hide);
}

QString SpeechBubbleController::text() const
{
    return m_text;
}

bool SpeechBubbleController::isVisible() const noexcept
{
    return m_visible;
}

bool SpeechBubbleController::arrowOnTop() const noexcept
{
    return m_arrowOnTop;
}

bool SpeechBubbleController::isThinking() const noexcept
{
    return m_thinking;
}

void SpeechBubbleController::attachWindows(
    QQuickWindow* petWindow,
    QQuickWindow* bubbleWindow)
{
    m_petWindow = petWindow;
    m_bubbleWindow = bubbleWindow;

    if (m_petWindow.isNull() || m_bubbleWindow.isNull()) {
        return;
    }

    connect(m_petWindow, &QWindow::xChanged, this, [this]() { reposition(); });
    connect(m_petWindow, &QWindow::yChanged, this, [this]() { reposition(); });
    connect(m_petWindow, &QWindow::widthChanged, this, [this]() { reposition(); });
    connect(m_petWindow, &QWindow::heightChanged, this, [this]() { reposition(); });
    connect(m_bubbleWindow, &QWindow::widthChanged, this, [this]() { reposition(); });
    connect(m_bubbleWindow, &QWindow::heightChanged, this, [this]() { reposition(); });
    reposition();
}

void SpeechBubbleController::showMessage(const QString& message, int durationMs)
{
    const QString trimmedMessage = message.trimmed();
    if (trimmedMessage.isEmpty()) {
        hide();
        return;
    }

    if (m_text != trimmedMessage) {
        m_text = trimmedMessage;
        emit textChanged();
    }
    if (m_thinking) {
        m_thinking = false;
        emit thinkingChanged();
    }

    reposition();
    if (!m_visible) {
        m_visible = true;
        emit visibleChanged();
    }
    m_hideTimer.start(qMax(800, durationMs));
    QTimer::singleShot(0, this, [this]() { reposition(); });
}

void SpeechBubbleController::showThinking()
{
    m_hideTimer.stop();
    if (!m_text.isEmpty()) {
        m_text.clear();
        emit textChanged();
    }
    if (!m_thinking) {
        m_thinking = true;
        emit thinkingChanged();
    }

    reposition();
    if (!m_visible) {
        m_visible = true;
        emit visibleChanged();
    }
    QTimer::singleShot(0, this, [this]() { reposition(); });
}

void SpeechBubbleController::hide()
{
    m_hideTimer.stop();
    if (m_thinking) {
        m_thinking = false;
        emit thinkingChanged();
    }
    if (!m_visible) {
        return;
    }

    m_visible = false;
    emit visibleChanged();
}

void SpeechBubbleController::reposition()
{
    if (m_petWindow.isNull() || m_bubbleWindow.isNull()) {
        return;
    }

    QScreen* screen = QGuiApplication::screenAt(m_petWindow->geometry().center());
    if (screen == nullptr) {
        screen = m_petWindow->screen();
    }
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen == nullptr) {
        return;
    }

    const QRect available = screen->availableGeometry();
    const QRect petGeometry = m_petWindow->geometry();
    const QSize bubbleSize = m_bubbleWindow->size();

    const int maximumX = qMax(available.left(), available.right() - bubbleSize.width() + 1);
    const int desiredX = petGeometry.center().x() - bubbleSize.width() / 2;
    const int bubbleX = qBound(available.left(), desiredX, maximumX);

    const int aboveY = petGeometry.top() - bubbleSize.height() - bubbleGap;
    const bool placeBelow = aboveY < available.top();
    const int desiredY = placeBelow
        ? petGeometry.bottom() + bubbleGap + 1
        : aboveY;
    const int maximumY = qMax(available.top(), available.bottom() - bubbleSize.height() + 1);
    const int bubbleY = qBound(available.top(), desiredY, maximumY);

    if (m_arrowOnTop != placeBelow) {
        m_arrowOnTop = placeBelow;
        emit placementChanged();
    }

    m_bubbleWindow->setPosition(bubbleX, bubbleY);
}

} // namespace lingnest::ui
