#include "ui/SystemTrayController.h"

#include <utility>

#include <QAction>
#include <QMenu>

namespace lingnest::ui {
namespace {

constexpr auto trayMenuStyle = R"(
QMenu {
    min-width: 218px;
    padding: 6px;
    background: rgba(245, 248, 252, 238);
    border: 1px solid rgba(255, 255, 255, 226);
    border-radius: 14px;
    color: #293440;
    font-family: "Segoe UI", "Microsoft YaHei UI";
    font-size: 13px;
}
QMenu::item {
    min-height: 22px;
    padding: 6px 18px;
    margin: 1px 0;
    border-radius: 6px;
}
QMenu::item:selected {
    color: #26313e;
    background: rgba(158, 199, 247, 76);
    border: 1px solid rgba(255, 255, 255, 126);
}
QMenu::item:disabled {
    color: #777777;
    background: transparent;
}
QMenu::separator {
    height: 1px;
    margin: 5px 9px;
    background: rgba(255, 255, 255, 116);
}
)";

} // namespace

SystemTrayController::SystemTrayController(
    QIcon icon,
    QObject* parent)
    : QObject(parent)
    , m_trayIcon(new QSystemTrayIcon(std::move(icon), this))
    , m_menu(std::make_unique<QMenu>())
{
    m_trayIcon->setObjectName(QStringLiteral("lingnestSystemTrayIcon"));
    m_trayIcon->setToolTip(QStringLiteral("LingNest"));

    buildMenu();
    setCurrentCharacterName({});
    m_trayIcon->setContextMenu(m_menu.get());
    connect(
        m_trayIcon,
        &QSystemTrayIcon::activated,
        this,
        &SystemTrayController::handleActivation);
}

SystemTrayController::~SystemTrayController()
{
    hide();
    m_trayIcon->setContextMenu(nullptr);
}

bool SystemTrayController::show()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return false;
    }

    m_trayIcon->show();
    return true;
}

void SystemTrayController::hide()
{
    if (m_trayIcon != nullptr) {
        m_trayIcon->hide();
    }
}

void SystemTrayController::setCurrentCharacterName(
    const QString& characterDisplayName)
{
    if (m_currentCharacterAction == nullptr) {
        return;
    }

    const QString normalizedName = characterDisplayName.trimmed();
    m_currentCharacterAction->setText(
        normalizedName.isEmpty()
            ? QStringLiteral("当前角色：未选择")
            : QStringLiteral("当前角色：%1").arg(normalizedName));
}

void SystemTrayController::setPaused(bool paused)
{
    if (m_pauseAction == nullptr) {
        return;
    }

    m_pauseAction->setText(
        paused
            ? QStringLiteral("继续自主活动")
            : QStringLiteral("暂停自主活动"));
}

void SystemTrayController::setPetVisible(bool visible)
{
    if (m_petVisibilityAction == nullptr) {
        return;
    }

    m_petVisibilityAction->setText(
        visible
            ? QStringLiteral("隐藏桌宠")
            : QStringLiteral("显示桌宠"));
}

void SystemTrayController::handleActivation(
    QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        emit chatRequested();
    } else if (reason == QSystemTrayIcon::Trigger) {
        emit petShowRequested();
    }
}

void SystemTrayController::buildMenu()
{
    m_menu->setObjectName(QStringLiteral("lingnestTrayMenu"));
    m_menu->setAttribute(Qt::WA_TranslucentBackground);
    m_menu->setWindowFlag(Qt::NoDropShadowWindowHint, true);
    m_menu->setStyleSheet(QString::fromLatin1(trayMenuStyle));
    m_menu->setWindowOpacity(1.0);

    auto* titleAction = new QAction(QStringLiteral("LingNest"), this);
    titleAction->setObjectName(QStringLiteral("trayTitleAction"));
    titleAction->setEnabled(false);
    m_menu->addAction(titleAction);

    m_currentCharacterAction = new QAction(this);
    m_currentCharacterAction->setObjectName(
        QStringLiteral("trayCurrentCharacterAction"));
    m_currentCharacterAction->setEnabled(false);
    m_menu->addAction(m_currentCharacterAction);
    m_menu->addSeparator();

    auto* chatAction = new QAction(QStringLiteral("打开聊天"), this);
    chatAction->setObjectName(QStringLiteral("trayChatAction"));
    m_menu->addAction(chatAction);
    connect(chatAction, &QAction::triggered, this, &SystemTrayController::chatRequested);

    m_petVisibilityAction = new QAction(QStringLiteral("隐藏桌宠"), this);
    m_petVisibilityAction->setObjectName(QStringLiteral("trayPetVisibilityAction"));
    m_menu->addAction(m_petVisibilityAction);
    connect(
        m_petVisibilityAction,
        &QAction::triggered,
        this,
        &SystemTrayController::petVisibilityToggleRequested);

    auto* characterSwitchAction = new QAction(QStringLiteral("切换角色"), this);
    characterSwitchAction->setObjectName(
        QStringLiteral("trayCharacterSwitchAction"));
    m_menu->addAction(characterSwitchAction);
    connect(
        characterSwitchAction,
        &QAction::triggered,
        this,
        &SystemTrayController::characterSwitchRequested);

    auto* settingsAction = new QAction(QStringLiteral("AI 设置"), this);
    settingsAction->setObjectName(QStringLiteral("traySettingsAction"));
    m_menu->addAction(settingsAction);
    connect(
        settingsAction,
        &QAction::triggered,
        this,
        &SystemTrayController::settingsRequested);

    m_menu->addSeparator();

    auto* waveAction = new QAction(QStringLiteral("让桌宠挥挥手"), this);
    waveAction->setObjectName(QStringLiteral("trayWaveAction"));
    m_menu->addAction(waveAction);
    connect(waveAction, &QAction::triggered, this, &SystemTrayController::waveRequested);

    m_pauseAction = new QAction(QStringLiteral("暂停自主活动"), this);
    m_pauseAction->setObjectName(QStringLiteral("trayPauseAction"));
    m_menu->addAction(m_pauseAction);
    connect(
        m_pauseAction,
        &QAction::triggered,
        this,
        &SystemTrayController::pauseToggleRequested);

    m_menu->addSeparator();

    auto* quitAction = new QAction(QStringLiteral("退出 LingNest"), this);
    quitAction->setObjectName(QStringLiteral("trayQuitAction"));
    m_menu->addAction(quitAction);
    connect(quitAction, &QAction::triggered, this, &SystemTrayController::quitRequested);
}

} // namespace lingnest::ui
