#pragma once

#include <QPointer>
#include <QString>
#include <QTimer>

class QQuickWindow;

namespace lingnest::ui {

class SpeechBubbleController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString text READ text NOTIFY textChanged)
    Q_PROPERTY(bool visible READ isVisible NOTIFY visibleChanged)
    Q_PROPERTY(bool arrowOnTop READ arrowOnTop NOTIFY placementChanged)
    Q_PROPERTY(bool thinking READ isThinking NOTIFY thinkingChanged)

public:
    explicit SpeechBubbleController(QObject* parent = nullptr);

    [[nodiscard]] QString text() const;
    [[nodiscard]] bool isVisible() const noexcept;
    [[nodiscard]] bool arrowOnTop() const noexcept;
    [[nodiscard]] bool isThinking() const noexcept;

    void attachWindows(QQuickWindow* petWindow, QQuickWindow* bubbleWindow);

    Q_INVOKABLE void showMessage(const QString& message, int durationMs = 3500);
    Q_INVOKABLE void showThinking();
    Q_INVOKABLE void hide();

signals:
    void textChanged();
    void visibleChanged();
    void placementChanged();
    void thinkingChanged();

private:
    void reposition();

    QPointer<QQuickWindow> m_petWindow;
    QPointer<QQuickWindow> m_bubbleWindow;
    QTimer m_hideTimer;
    QString m_text;
    bool m_visible {false};
    bool m_arrowOnTop {false};
    bool m_thinking {false};
};

} // namespace lingnest::ui
