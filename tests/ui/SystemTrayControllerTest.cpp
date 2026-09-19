#include <QAction>
#include <QIcon>
#include <QSignalSpy>
#include <QTest>

#include "ui/SystemTrayController.h"

namespace lingnest::ui {

class SystemTrayControllerTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesExpectedActionsAndSignals();
    void reflectsPausedAndPetVisibilityState();
};

void SystemTrayControllerTest::exposesExpectedActionsAndSignals()
{
    SystemTrayController controller(QIcon {});

    auto* chatAction = controller.findChild<QAction*>(
        QStringLiteral("trayChatAction"));
    auto* settingsAction = controller.findChild<QAction*>(
        QStringLiteral("traySettingsAction"));
    auto* characterSwitchAction = controller.findChild<QAction*>(
        QStringLiteral("trayCharacterSwitchAction"));
    auto* waveAction = controller.findChild<QAction*>(
        QStringLiteral("trayWaveAction"));
    auto* pauseAction = controller.findChild<QAction*>(
        QStringLiteral("trayPauseAction"));
    auto* quitAction = controller.findChild<QAction*>(
        QStringLiteral("trayQuitAction"));

    QVERIFY(chatAction != nullptr);
    QVERIFY(settingsAction != nullptr);
    QVERIFY(characterSwitchAction != nullptr);
    QVERIFY(waveAction != nullptr);
    QVERIFY(pauseAction != nullptr);
    QVERIFY(quitAction != nullptr);
    QCOMPARE(chatAction->text(), QStringLiteral("打开聊天"));
    QCOMPARE(characterSwitchAction->text(), QStringLiteral("切换角色"));
    QCOMPARE(quitAction->text(), QStringLiteral("退出 LingNest"));

    QSignalSpy chatSpy(&controller, &SystemTrayController::chatRequested);
    QSignalSpy settingsSpy(&controller, &SystemTrayController::settingsRequested);
    QSignalSpy characterSwitchSpy(
        &controller, &SystemTrayController::characterSwitchRequested);
    QSignalSpy waveSpy(&controller, &SystemTrayController::waveRequested);
    QSignalSpy pauseSpy(&controller, &SystemTrayController::pauseToggleRequested);
    QSignalSpy quitSpy(&controller, &SystemTrayController::quitRequested);

    chatAction->trigger();
    settingsAction->trigger();
    characterSwitchAction->trigger();
    waveAction->trigger();
    pauseAction->trigger();
    quitAction->trigger();

    QCOMPARE(chatSpy.count(), 1);
    QCOMPARE(settingsSpy.count(), 1);
    QCOMPARE(characterSwitchSpy.count(), 1);
    QCOMPARE(waveSpy.count(), 1);
    QCOMPARE(pauseSpy.count(), 1);
    QCOMPARE(quitSpy.count(), 1);
}

void SystemTrayControllerTest::reflectsPausedAndPetVisibilityState()
{
    SystemTrayController controller(QIcon {});
    auto* currentCharacterAction = controller.findChild<QAction*>(
        QStringLiteral("trayCurrentCharacterAction"));
    auto* pauseAction = controller.findChild<QAction*>(
        QStringLiteral("trayPauseAction"));
    auto* visibilityAction = controller.findChild<QAction*>(
        QStringLiteral("trayPetVisibilityAction"));

    QVERIFY(currentCharacterAction != nullptr);
    QVERIFY(pauseAction != nullptr);
    QVERIFY(visibilityAction != nullptr);

    controller.setPaused(true);
    QVERIFY(!pauseAction->isCheckable());
    QCOMPARE(pauseAction->text(), QStringLiteral("继续自主活动"));
    controller.setPaused(false);
    QCOMPARE(pauseAction->text(), QStringLiteral("暂停自主活动"));

    controller.setPetVisible(false);
    QCOMPARE(visibilityAction->text(), QStringLiteral("显示桌宠"));
    controller.setPetVisible(true);
    QCOMPARE(visibilityAction->text(), QStringLiteral("隐藏桌宠"));

    QCOMPARE(
        currentCharacterAction->text(),
        QStringLiteral("当前角色：未选择"));
    controller.setCurrentCharacterName(QStringLiteral("渝爱"));
    QCOMPARE(
        currentCharacterAction->text(),
        QStringLiteral("当前角色：渝爱"));
    controller.setCurrentCharacterName(QStringLiteral("小狐"));
    QCOMPARE(
        currentCharacterAction->text(),
        QStringLiteral("当前角色：小狐"));

    auto* trayIcon = controller.findChild<QSystemTrayIcon*>(
        QStringLiteral("lingnestSystemTrayIcon"));
    QVERIFY(trayIcon != nullptr);
    QCOMPARE(trayIcon->toolTip(), QStringLiteral("LingNest"));
}

} // namespace lingnest::ui

QTEST_MAIN(lingnest::ui::SystemTrayControllerTest)

#include "SystemTrayControllerTest.moc"
