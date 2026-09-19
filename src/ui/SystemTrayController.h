#pragma once

#include <memory>

#include <QIcon>
#include <QObject>
#include <QString>
#include <QSystemTrayIcon>

class QAction;
class QMenu;

namespace lingnest::ui {

class SystemTrayController final : public QObject {
    Q_OBJECT

public:
    explicit SystemTrayController(
        QIcon icon,
        QObject* parent = nullptr);
    ~SystemTrayController() override;

    [[nodiscard]] bool show();
    void hide();
    void setCurrentCharacterName(const QString& characterDisplayName);
    void setPaused(bool paused);
    void setPetVisible(bool visible);

signals:
    void chatRequested();
    void petShowRequested();
    void petVisibilityToggleRequested();
    void characterSwitchRequested();
    void settingsRequested();
    void waveRequested();
    void pauseToggleRequested();
    void quitRequested();

private slots:
    void handleActivation(QSystemTrayIcon::ActivationReason reason);

private:
    void buildMenu();

    QSystemTrayIcon* m_trayIcon {nullptr};
    std::unique_ptr<QMenu> m_menu;
    QAction* m_currentCharacterAction {nullptr};
    QAction* m_petVisibilityAction {nullptr};
    QAction* m_pauseAction {nullptr};
};

} // namespace lingnest::ui
