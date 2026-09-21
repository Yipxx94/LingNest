#include <QtTest>

#include <utility>

#include <QCoreApplication>
#include <QColor>
#include <QDir>
#include <QGuiApplication>
#include <QIcon>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QTemporaryDir>
#include <QUrl>

#include "animation/AnimationCatalog.h"
#include "animation/IAnimationPlayer.h"
#include "animation/PngSequenceAnimationPlayer.h"
#include "character/CharacterDefinition.h"
#include "character/CharacterLoader.h"
#include "config/ConfigManager.h"
#include "ui/ChatController.h"
#include "ui/PetWindowController.h"
#include "ui/SpeechBubbleController.h"

namespace {

bool saveQaImage(const QImage& image, const QString& fileName)
{
    const QString outputDirectory = qEnvironmentVariable("LINGNEST_QA_OUTPUT_DIR");
    if (outputDirectory.isEmpty()) {
        return true;
    }

    QDir directory(outputDirectory);
    return directory.mkpath(QStringLiteral("."))
        && image.save(directory.filePath(fileName));
}

class FakeAnimationView final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl currentFrame READ currentFrame CONSTANT)

public:
    [[nodiscard]] QUrl currentFrame() const
    {
        return {};
    }
};

class FakeInteraction final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool paused READ isPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool chatVisible READ isChatVisible NOTIFY chatVisibleChanged)
    Q_PROPERTY(bool settingsVisible READ isSettingsVisible NOTIFY settingsVisibleChanged)

public:
    [[nodiscard]] bool isPaused() const noexcept { return m_paused; }
    [[nodiscard]] bool isChatVisible() const noexcept { return m_chatVisible; }
    [[nodiscard]] bool isSettingsVisible() const noexcept { return m_settingsVisible; }
    [[nodiscard]] int clickCandidateCount() const noexcept { return m_clickCandidateCount; }
    [[nodiscard]] int doubleClickCount() const noexcept { return m_doubleClickCount; }

    Q_INVOKABLE void clickCandidate() { ++m_clickCandidateCount; }
    Q_INVOKABLE void quickResponse() { ++m_clickCandidateCount; }
    Q_INVOKABLE void doubleClick()
    {
        ++m_doubleClickCount;
        openChat();
    }
    Q_INVOKABLE void openChat()
    {
        if (!m_chatVisible) {
            m_chatVisible = true;
            emit chatVisibleChanged();
        }
    }
    Q_INVOKABLE void closeChat()
    {
        if (m_chatVisible) {
            m_chatVisible = false;
            emit chatVisibleChanged();
        }
    }
    Q_INVOKABLE void showSettings()
    {
        if (!m_settingsVisible) {
            m_settingsVisible = true;
            emit settingsVisibleChanged();
        }
    }
    Q_INVOKABLE void closeSettings()
    {
        if (m_settingsVisible) {
            m_settingsVisible = false;
            emit settingsVisibleChanged();
        }
    }
    Q_INVOKABLE void showCharacterSwitcher() {}
    Q_INVOKABLE void playDebugAction(const QString&) {}
    Q_INVOKABLE void togglePaused()
    {
        m_paused = !m_paused;
        emit pausedChanged();
    }
    Q_INVOKABLE void quitApplication() {}

signals:
    void pausedChanged();
    void chatVisibleChanged();
    void settingsVisibleChanged();

private:
    bool m_paused {false};
    bool m_chatVisible {false};
    bool m_settingsVisible {false};
    int m_clickCandidateCount {0};
    int m_doubleClickCount {0};
};

class FakeAISettings final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString baseUrl READ baseUrl CONSTANT)
    Q_PROPERTY(QString model READ model CONSTANT)
    Q_PROPERTY(double temperature READ temperature CONSTANT)
    Q_PROPERTY(int maxTokens READ maxTokens CONSTANT)
    Q_PROPERTY(int timeoutSeconds READ timeoutSeconds CONSTANT)
    Q_PROPERTY(bool apiKeyConfigured READ apiKeyConfigured CONSTANT)
    Q_PROPERTY(QString statusMessage READ statusMessage CONSTANT)
    Q_PROPERTY(bool statusError READ statusError CONSTANT)

public:
    [[nodiscard]] QString baseUrl() const { return QStringLiteral("https://example.test/v1"); }
    [[nodiscard]] QString model() const { return QStringLiteral("test-model"); }
    [[nodiscard]] double temperature() const noexcept { return 0.7; }
    [[nodiscard]] int maxTokens() const noexcept { return 512; }
    [[nodiscard]] int timeoutSeconds() const noexcept { return 30; }
    [[nodiscard]] bool apiKeyConfigured() const noexcept { return false; }
    [[nodiscard]] QString statusMessage() const { return {}; }
    [[nodiscard]] bool statusError() const noexcept { return false; }

    Q_INVOKABLE void save(
        const QString&, const QString&, double, int, int, const QString&)
    {
    }
    Q_INVOKABLE void clearApiKey() {}
};

void installQmlContext(
    QQmlApplicationEngine& engine,
    lingnest::ui::PetWindowController* petController,
    QObject* animationView,
    FakeInteraction* interaction,
    lingnest::ui::SpeechBubbleController* bubble,
    lingnest::ui::ChatController* chat,
    FakeAISettings* aiSettings)
{
    engine.rootContext()->setContextProperty(QStringLiteral("petController"), petController);
    engine.rootContext()->setContextProperty(QStringLiteral("petAnimation"), animationView);
    engine.rootContext()->setContextProperty(QStringLiteral("petInteraction"), interaction);
    engine.rootContext()->setContextProperty(QStringLiteral("bubbleController"), bubble);
    engine.rootContext()->setContextProperty(QStringLiteral("chatController"), chat);
    engine.rootContext()->setContextProperty(QStringLiteral("aiSettings"), aiSettings);
}

class PetWindowControllerTest final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void mouseDragMovesWindowAndPersistsPosition();
    void clampsDragToAvailableScreen();
    void releasedNativeDragReturnsFromEveryScreenEdge();
    void popupPositionStaysInsideAvailableScreen();
    void repeatedDragCoordinateDoesNotDrift();
    void autonomousWalkMovesAndTurnsAtScreenEdge();
    void clickDoubleClickAndContextMenuAreRouted();
    void rendersChangingIdleFrames();
};

void PetWindowControllerTest::initTestCase()
{
    qputenv("LINGNEST_DISABLE_SYSTEM_MOVE", QByteArrayLiteral("1"));
    const QIcon packagedIcon(QStringLiteral(":/icons/lingnest.ico"));
    QVERIFY(!packagedIcon.isNull());
    QVERIFY(!packagedIcon.pixmap(QSize(32, 32)).isNull());
}

void PetWindowControllerTest::mouseDragMovesWindowAndPersistsPosition()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString configPath =
        QDir(temporaryDirectory.path()).filePath(QStringLiteral("config.json"));
    lingnest::config::ConfigManager configManager(configPath);
    QString errorMessage;
    QVERIFY2(configManager.load(&errorMessage), qPrintable(errorMessage));

    lingnest::character::CharacterDefinition character;
    character.id = QStringLiteral("test-pet");
    character.displayName = QStringLiteral("Test Pet");
    character.scale = 1.0;

    lingnest::ui::PetWindowController controller(character, &configManager);
    FakeAnimationView animationView;
    FakeInteraction interaction;
    FakeAISettings aiSettings;
    lingnest::ui::SpeechBubbleController bubbleController;
    lingnest::ui::ChatController chatController(character);
    QQmlApplicationEngine engine;
    installQmlContext(
        engine,
        &controller,
        &animationView,
        &interaction,
        &bubbleController,
        &chatController,
        &aiSettings);
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    QVERIFY(!engine.rootObjects().isEmpty());

    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
    QVERIFY(window != nullptr);
    controller.attachWindow(window);

    QScreen* screen = QGuiApplication::primaryScreen();
    QVERIFY(screen != nullptr);
    const QRect available = screen->availableGeometry();
    const QPoint initialPosition(
        available.left() + qMax(20, (available.width() - window->width()) / 2),
        available.top() + qMax(20, (available.height() - window->height()) / 2));
    window->setPosition(initialPosition);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    const QPoint pressPosition(window->width() / 2, window->height() / 2);
    const QPoint dragPosition = pressPosition + QPoint(40, 30);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, pressPosition);
    QTest::mouseMove(window, dragPosition, 50);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, dragPosition);
    QCoreApplication::processEvents();

    QVERIFY(window->position().x() > initialPosition.x());
    QVERIFY(window->position().y() > initialPosition.y());
    QCOMPARE(interaction.clickCandidateCount(), 0);

    lingnest::config::ConfigManager savedConfig(configPath);
    QVERIFY2(savedConfig.load(&errorMessage), qPrintable(errorMessage));
    const auto placement = savedConfig.petWindowPlacement();
    QVERIFY(placement.has_value());
    QCOMPARE(placement->position, window->position());
}

void PetWindowControllerTest::clickDoubleClickAndContextMenuAreRouted()
{
    lingnest::character::CharacterDefinition character;
    character.id = QStringLiteral("test-pet");
    character.displayName = QStringLiteral("Test Pet");
    character.scale = 1.0;

    lingnest::ui::PetWindowController controller(character, nullptr);
    FakeAnimationView animationView;
    FakeInteraction interaction;
    FakeAISettings aiSettings;
    lingnest::ui::SpeechBubbleController bubbleController;
    lingnest::ui::ChatController chatController(character);
    QQmlApplicationEngine engine;
    installQmlContext(
        engine,
        &controller,
        &animationView,
        &interaction,
        &bubbleController,
        &chatController,
        &aiSettings);
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    QVERIFY(!engine.rootObjects().isEmpty());

    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
    QVERIFY(window != nullptr);
    controller.attachWindow(window);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    const QPoint center(window->width() / 2, window->height() / 2);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(interaction.clickCandidateCount(), 1);
    QCOMPARE(interaction.doubleClickCount(), 0);

    QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, center);
    QTRY_COMPARE(interaction.doubleClickCount(), 1);
    QVERIFY(interaction.isChatVisible());
    interaction.closeChat();
    QTest::qWait(140);
    QVERIFY(!interaction.isChatVisible());

    QScreen* screen = QGuiApplication::primaryScreen();
    QVERIFY(screen != nullptr);
    const QRect available = screen->availableGeometry();
    window->setPosition(
        available.right() - window->width() + 1,
        available.bottom() - window->height() + 1);
    window->requestActivate();
    QTRY_VERIFY(window->isActive());

    auto* contextMenu = window->findChild<QObject*>(
        QStringLiteral("contextMenu"), Qt::FindChildrenRecursively);
    QVERIFY(contextMenu != nullptr);
    auto* contextMenuWindow = qobject_cast<QQuickWindow*>(contextMenu);
    QVERIFY(contextMenuWindow != nullptr);
    QCOMPARE(
        contextMenuWindow->flags() & Qt::WindowType_Mask,
        Qt::WindowFlags(Qt::Tool));
    QVERIFY(contextMenuWindow->flags().testFlag(Qt::NoDropShadowWindowHint));
    QTest::mouseClick(window, Qt::RightButton, Qt::NoModifier, center);
    QTRY_VERIFY(contextMenu->property("visible").toBool());
    QVERIFY(QTest::qWaitForWindowExposed(contextMenuWindow));
    auto* mainMenuContent = contextMenuWindow->findChild<QQuickItem*>(
        QStringLiteral("mainMenuContent"), Qt::FindChildrenRecursively);
    auto* quitMenuButton = contextMenuWindow->findChild<QQuickItem*>(
        QStringLiteral("quitMenuButton"), Qt::FindChildrenRecursively);
    QVERIFY(mainMenuContent != nullptr);
    QVERIFY(quitMenuButton != nullptr);
    QTRY_VERIFY(mainMenuContent->implicitHeight() > 280);
    QTRY_VERIFY(quitMenuButton->height() >= 36);
    const QStringList mainRowNames {
        QStringLiteral("chatMenuButton"),
        QStringLiteral("settingsMenuButton"),
        QStringLiteral("switchMenuButton"),
        QStringLiteral("pauseMenuButton"),
        QStringLiteral("previewMenuButton"),
        QStringLiteral("quitMenuButton")
    };
    QVector<QQuickItem*> mainRows;
    for (const QString& rowName : mainRowNames) {
        auto* row = contextMenuWindow->findChild<QQuickItem*>(
            rowName, Qt::FindChildrenRecursively);
        QVERIFY2(row != nullptr, qPrintable(rowName));
        mainRows.append(row);
    }
    for (int index = 1; index < mainRows.size(); ++index) {
        const qreal gap = mainRows.at(index)->y()
            - (mainRows.at(index - 1)->y()
                + mainRows.at(index - 1)->height());
        QCOMPARE(qRound(gap), 2);
    }
    QTRY_VERIFY(contextMenuWindow->height()
        >= qCeil(mainMenuContent->implicitHeight() + 36));
    const qreal quitButtonBottom = quitMenuButton->mapToScene(
        QPointF(0, quitMenuButton->height())).y();
    QVERIFY2(quitButtonBottom <= contextMenuWindow->height() - 20,
        "The final main-menu action is clipped by the glass card");
    QTRY_VERIFY(available.contains(contextMenuWindow->geometry()));
    QTest::qWait(220);
    const QImage menuFrame = contextMenuWindow->grabWindow();
    QVERIFY(!menuFrame.isNull());
    QVERIFY(saveQaImage(menuFrame, QStringLiteral("context-menu.png")));

    auto* chatMenuButton = mainRows.constFirst();
    const QPoint chatButtonCenter = chatMenuButton->mapToScene(
        QPointF(chatMenuButton->width() / 2,
                chatMenuButton->height() / 2)).toPoint();
    QTest::mouseMove(contextMenuWindow, chatButtonCenter);
    QTRY_VERIFY(chatMenuButton->property("pointerHovered").toBool());
    QTest::qWait(300);
    QVERIFY2(chatMenuButton->property("pointerHovered").toBool(),
        "A menu row must remain highlighted until the pointer leaves it");
    auto* chatButtonBackground = chatMenuButton->property("background")
        .value<QObject*>();
    QVERIFY(chatButtonBackground != nullptr);
    QCOMPARE(chatButtonBackground->property("color").value<QColor>(),
        QColor(QStringLiteral("#4CFFFFFF")));
    QTest::mouseMove(contextMenuWindow, QPoint(2, 2));
    QTRY_VERIFY(!chatMenuButton->property("pointerHovered").toBool());
    QTRY_COMPARE(chatButtonBackground->property("color").value<QColor>(),
        QColor(Qt::transparent));

    auto* previewMenuButton = contextMenuWindow->findChild<QQuickItem*>(
        QStringLiteral("previewMenuButton"), Qt::FindChildrenRecursively);
    QVERIFY(previewMenuButton != nullptr);
    const QPoint previewButtonCenter = previewMenuButton->mapToScene(
        QPointF(previewMenuButton->width() / 2,
                previewMenuButton->height() / 2)).toPoint();
    QTest::mouseClick(
        contextMenuWindow, Qt::LeftButton, Qt::NoModifier, previewButtonCenter);
    QTRY_VERIFY(contextMenu->property("actionsPage").toBool());
    auto* actionsMenuContent = contextMenuWindow->findChild<QQuickItem*>(
        QStringLiteral("actionsMenuContent"), Qt::FindChildrenRecursively);
    auto* sleepMenuButton = contextMenuWindow->findChild<QQuickItem*>(
        QStringLiteral("sleepMenuButton"), Qt::FindChildrenRecursively);
    QVERIFY(actionsMenuContent != nullptr);
    QVERIFY(sleepMenuButton != nullptr);
    QTRY_VERIFY(actionsMenuContent->implicitHeight() > 350);
    QTRY_VERIFY(sleepMenuButton->height() >= 36);
    QTRY_VERIFY(contextMenuWindow->height()
        >= qCeil(actionsMenuContent->implicitHeight() + 36));
    const qreal sleepButtonBottom = sleepMenuButton->mapToScene(
        QPointF(0, sleepMenuButton->height())).y();
    QVERIFY2(sleepButtonBottom <= contextMenuWindow->height() - 20,
        "The final action-preview item is clipped by the glass card");
    QTRY_VERIFY(available.contains(contextMenuWindow->geometry()));
    QTest::qWait(220);
    const QImage actionMenuFrame = contextMenuWindow->grabWindow();
    QVERIFY(!actionMenuFrame.isNull());
    QVERIFY(saveQaImage(
        actionMenuFrame, QStringLiteral("context-actions-menu.png")));

    // A click on the pet must dismiss a menu that was opened by right-click.
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, center);
    QTRY_VERIFY(!contextMenuWindow->isVisible());

    // Focusing another window must dismiss it as well.
    window->requestActivate();
    QTRY_VERIFY(window->isActive());
    QTest::mouseClick(window, Qt::RightButton, Qt::NoModifier, center);
    QTRY_VERIFY(contextMenuWindow->isVisible());
    QQuickWindow otherWindow;
    otherWindow.setGeometry(
        available.left() + 40, available.top() + 40, 160, 120);
    otherWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&otherWindow));
    otherWindow.requestActivate();
    QTRY_VERIFY(otherWindow.isActive());
    QTRY_VERIFY(!contextMenuWindow->isVisible());
}

void PetWindowControllerTest::popupPositionStaysInsideAvailableScreen()
{
    lingnest::character::CharacterDefinition character;
    character.id = QStringLiteral("test-pet");
    character.displayName = QStringLiteral("Test Pet");
    character.scale = 1.0;

    lingnest::ui::PetWindowController controller(character, nullptr);
    QQuickWindow window;
    window.resize(192, 208);
    controller.attachWindow(&window);

    QScreen* screen = QGuiApplication::primaryScreen();
    QVERIFY(screen != nullptr);
    const QRect available = screen->availableGeometry();
    const QVariantMap availableMap = controller.availableScreenGeometry();
    QCOMPARE(availableMap.value(QStringLiteral("x")).toInt(), available.x());
    QCOMPARE(availableMap.value(QStringLiteral("y")).toInt(), available.y());
    QCOMPARE(availableMap.value(QStringLiteral("width")).toInt(), available.width());
    QCOMPARE(availableMap.value(QStringLiteral("height")).toInt(), available.height());
    window.setPosition(available.bottomRight() - QPoint(191, 207));

    constexpr int popupWidth = 232;
    constexpr int popupHeight = 382;
    constexpr int margin = 10;
    const QVariantMap bottomRight = controller.boundedPopupPosition(
        available.right(),
        available.bottom(),
        popupWidth,
        popupHeight,
        margin);
    const QRect bottomRightPopup(
        bottomRight.value(QStringLiteral("x")).toInt(),
        bottomRight.value(QStringLiteral("y")).toInt(),
        popupWidth,
        popupHeight);
    QVERIFY(available.adjusted(margin, margin, -margin, -margin)
                .contains(bottomRightPopup));

    const QVariantMap topLeft = controller.boundedPopupPosition(
        available.left() - 500,
        available.top() - 500,
        popupWidth,
        popupHeight,
        margin);
    QCOMPARE(
        topLeft.value(QStringLiteral("x")).toInt(),
        available.left() + margin);
    QCOMPARE(
        topLeft.value(QStringLiteral("y")).toInt(),
        available.top() + margin);
}

void PetWindowControllerTest::clampsDragToAvailableScreen()
{
    lingnest::character::CharacterDefinition character;
    character.id = QStringLiteral("test-pet");
    character.displayName = QStringLiteral("Test Pet");
    character.scale = 1.0;

    lingnest::ui::PetWindowController controller(character, nullptr);
    QQuickWindow window;
    window.resize(192, 208);
    controller.attachWindow(&window);

    QScreen* screen = QGuiApplication::primaryScreen();
    QVERIFY(screen != nullptr);
    const QRect available = screen->availableGeometry();
    const QPoint initialPosition = available.center() - QPoint(96, 104);
    window.setPosition(initialPosition);

    controller.beginDrag(
        initialPosition.x() + 96, initialPosition.y() + 104);
    controller.updateDrag(-100000.0, -100000.0);
    QVERIFY(controller.endDrag());

    QCOMPARE(window.position(), available.topLeft());
}

void PetWindowControllerTest::releasedNativeDragReturnsFromEveryScreenEdge()
{
    lingnest::character::CharacterDefinition character;
    character.id = QStringLiteral("test-pet");
    character.displayName = QStringLiteral("Test Pet");
    character.scale = 1.0;

    lingnest::ui::PetWindowController controller(character, nullptr);
    QQuickWindow window;
    window.resize(192, 208);
    controller.attachWindow(&window);

    QScreen* screen = QGuiApplication::primaryScreen();
    QVERIFY(screen != nullptr);
    const QRect available = screen->availableGeometry();
    const QPoint initialPosition = available.center()
        - QPoint(window.width() / 2, window.height() / 2);
    const int maximumX = available.right() - window.width() + 1;
    const int maximumY = available.bottom() - window.height() + 1;

    const QVector<QPair<QPoint, QPoint>> edgeCases {
        {QPoint(available.left() - window.width() - 50, initialPosition.y()),
         QPoint(available.left(), initialPosition.y())},
        {QPoint(available.right() + 50, initialPosition.y()),
         QPoint(maximumX, initialPosition.y())},
        {QPoint(initialPosition.x(), available.top() - window.height() - 50),
         QPoint(initialPosition.x(), available.top())},
        {QPoint(initialPosition.x(), available.bottom() + 50),
         QPoint(initialPosition.x(), maximumY)}
    };

    for (const auto& edgeCase : edgeCases) {
        window.setPosition(initialPosition);
        controller.beginDrag(initialPosition.x() + window.width() / 2,
                             initialPosition.y() + window.height() / 2);
        // The native window manager moves the window without updateDrag().
        window.setPosition(edgeCase.first);
        QVERIFY(controller.endDrag());
        QCOMPARE(window.position(), edgeCase.second);
    }
}

void PetWindowControllerTest::repeatedDragCoordinateDoesNotDrift()
{
    lingnest::character::CharacterDefinition character;
    character.id = QStringLiteral("test-pet");
    character.displayName = QStringLiteral("Test Pet");
    character.scale = 1.0;

    lingnest::ui::PetWindowController controller(character, nullptr);
    QQuickWindow window;
    window.resize(192, 208);
    controller.attachWindow(&window);

    QScreen* screen = QGuiApplication::primaryScreen();
    QVERIFY(screen != nullptr);
    const QRect available = screen->availableGeometry();
    const QPoint initialPosition(
        available.center().x() - window.width() / 2,
        available.center().y() - window.height() / 2);
    window.setPosition(initialPosition);

    const QPoint pressPosition = initialPosition + QPoint(96, 104);
    controller.beginDrag(pressPosition.x(), pressPosition.y());
    const QPoint movedPointer = pressPosition + QPoint(48, 36);
    controller.updateDrag(movedPointer.x(), movedPointer.y());
    const QPoint movedWindowPosition = window.position();

    for (int iteration = 0; iteration < 20; ++iteration) {
        controller.updateDrag(movedPointer.x(), movedPointer.y());
        QCOMPARE(window.position(), movedWindowPosition);
    }

    QCOMPARE(movedWindowPosition, initialPosition + QPoint(48, 36));
    QVERIFY(controller.endDrag());
}

void PetWindowControllerTest::autonomousWalkMovesAndTurnsAtScreenEdge()
{
    lingnest::character::CharacterDefinition character;
    character.id = QStringLiteral("test-pet");
    character.displayName = QStringLiteral("Test Pet");
    character.scale = 1.0;

    lingnest::ui::PetWindowController controller(character, nullptr);
    controller.setWalkSpeedPixelsPerSecond(1000.0);
    QQuickWindow window;
    window.resize(192, 208);
    controller.attachWindow(&window);

    QScreen* screen = QGuiApplication::primaryScreen();
    QVERIFY(screen != nullptr);
    const QRect available = screen->availableGeometry();
    const int maximumX = available.right() - window.width() + 1;
    const int initialX = available.center().x() - window.width() / 2;
    window.setPosition(initialX, available.center().y() - window.height() / 2);

    controller.startWalking(lingnest::pet::WalkDirection::Right);
    QTRY_VERIFY(window.x() > initialX);
    QVERIFY(controller.isWalking());
    controller.stopWalking();
    QVERIFY(!controller.isWalking());

    window.setX(maximumX - 1);
    QSignalSpy boundarySpy(
        &controller, &lingnest::ui::PetWindowController::walkBoundaryTurned);
    controller.startWalking(lingnest::pet::WalkDirection::Right);
    QTRY_COMPARE(boundarySpy.count(), 1);
    QCOMPARE(
        qvariant_cast<lingnest::pet::WalkDirection>(
            boundarySpy.constFirst().constFirst()),
        lingnest::pet::WalkDirection::Left);
    QVERIFY(window.x() >= available.left());
    QVERIFY(window.x() <= maximumX);
    controller.stopWalking();
}

void PetWindowControllerTest::rendersChangingIdleFrames()
{
    QString errorMessage;
    auto character = lingnest::character::CharacterLoader::loadFromDirectory(
        QStringLiteral(LINGNEST_TEST_CHARACTER_DIR), &errorMessage);
    QVERIFY2(character.has_value(), qPrintable(errorMessage));

    auto catalog = lingnest::animation::AnimationCatalog::load(
        character->animationManifestPath,
        character->packageDirectory,
        &errorMessage);
    QVERIFY2(catalog.has_value(), qPrintable(errorMessage));

    lingnest::animation::PngSequenceAnimationPlayer animationPlayer(
        std::move(*catalog));
    QVERIFY(animationPlayer.play(
        character->defaultAnimation,
        lingnest::animation::IAnimationPlayer::PlaybackMode::Loop));

    lingnest::ui::PetWindowController controller(*character, nullptr);
    FakeInteraction interaction;
    FakeAISettings aiSettings;
    lingnest::ui::SpeechBubbleController bubbleController;
    lingnest::ui::ChatController chatController(*character);
    QQmlApplicationEngine engine;
    installQmlContext(
        engine,
        &controller,
        &animationPlayer,
        &interaction,
        &bubbleController,
        &chatController,
        &aiSettings);
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    QVERIFY(!engine.rootObjects().isEmpty());

    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
    QVERIFY(window != nullptr);
    auto* bubbleWindow = window->findChild<QQuickWindow*>(
        QStringLiteral("speechBubbleWindow"), Qt::FindChildrenRecursively);
    auto* chatWindow = window->findChild<QQuickWindow*>(
        QStringLiteral("chatWindow"), Qt::FindChildrenRecursively);
    auto* hoverDock = window->findChild<QQuickWindow*>(
        QStringLiteral("petHoverDock"), Qt::FindChildrenRecursively);
    auto* settingsWindow = window->findChild<QQuickWindow*>(
        QStringLiteral("settingsWindow"), Qt::FindChildrenRecursively);
    QVERIFY(bubbleWindow != nullptr);
    QVERIFY(chatWindow != nullptr);
    QVERIFY(hoverDock != nullptr);
    QVERIFY(settingsWindow != nullptr);
    QVERIFY(bubbleWindow->flags().testFlag(Qt::NoDropShadowWindowHint));
    QVERIFY(chatWindow->flags().testFlag(Qt::NoDropShadowWindowHint));
    QVERIFY(hoverDock->flags().testFlag(Qt::NoDropShadowWindowHint));
    bubbleController.attachWindows(window, bubbleWindow);
    controller.attachWindow(window);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QTest::qWait(80);

    QScreen* screen = QGuiApplication::primaryScreen();
    QVERIFY(screen != nullptr);
    const QRect available = screen->availableGeometry();
    window->setPosition(
        available.center().x() - window->width() / 2,
        available.center().y() - window->height() / 2);
    QCoreApplication::processEvents();

    QVERIFY(QMetaObject::invokeMethod(hoverDock, "petEntered"));
    QTRY_VERIFY(hoverDock->isVisible());
    QVERIFY(QTest::qWaitForWindowExposed(hoverDock));
    QTest::qWait(200);
    QVERIFY(available.contains(hoverDock->geometry()));
    QVERIFY(hoverDock->geometry().top()
            >= window->geometry().bottom() - 7);
    const QImage hoverDockFrame = hoverDock->grabWindow();
    QVERIFY(!hoverDockFrame.isNull());
    QVERIFY(saveQaImage(hoverDockFrame, QStringLiteral("hover-chat-dock.png")));
    QTest::mouseClick(
        hoverDock,
        Qt::LeftButton,
        Qt::NoModifier,
        QPoint(hoverDock->width() * 3 / 4, hoverDock->height() / 2));
    QTRY_COMPARE(interaction.clickCandidateCount(), 1);
    QVERIFY(QMetaObject::invokeMethod(hoverDock, "petEntered"));
    QTRY_VERIFY(hoverDock->isVisible());
    QTest::mouseClick(
        hoverDock,
        Qt::LeftButton,
        Qt::NoModifier,
        QPoint(hoverDock->width() / 4, hoverDock->height() / 2));
    QTRY_VERIFY(interaction.isChatVisible());
    QTRY_VERIFY(chatWindow->geometry().top() > window->geometry().bottom());
    interaction.closeChat();
    QTRY_VERIFY(!chatWindow->isVisible());
    QVERIFY(QMetaObject::invokeMethod(hoverDock, "petExited"));

    const QImage firstFrame = window->grabWindow();
    QVERIFY(!firstFrame.isNull());
    QCOMPARE(firstFrame.size(), QSize(192, 208));

    bool hasVisiblePixel = false;
    for (int y = 0; y < firstFrame.height() && !hasVisiblePixel; ++y) {
        for (int x = 0; x < firstFrame.width(); ++x) {
            if (qAlpha(firstFrame.pixel(x, y)) > 0) {
                hasVisiblePixel = true;
                break;
            }
        }
    }
    QVERIFY(hasVisiblePixel);

    QTest::qWait(360);
    const QImage laterFrame = window->grabWindow();
    QVERIFY(!laterFrame.isNull());
    QVERIFY(firstFrame != laterFrame);

    const QStringList newlyWiredAnimations {
        QStringLiteral("blink"),
        QStringLiteral("wave"),
        QStringLiteral("daze_prone"),
        QStringLiteral("sleep"),
        QStringLiteral("walk_left"),
        QStringLiteral("walk_right")
    };
    for (const QString& animationName : newlyWiredAnimations) {
        QVERIFY2(
            animationPlayer.play(
                animationName,
                lingnest::animation::IAnimationPlayer::PlaybackMode::Loop),
            qPrintable(animationName));
        QTest::qWait(80);
        const QImage actionFrame = window->grabWindow();
        QVERIFY2(!actionFrame.isNull(), qPrintable(animationName));
        QCOMPARE(actionFrame.size(), QSize(192, 208));
        QVERIFY2(actionFrame != firstFrame, qPrintable(animationName));
    }

    window->setPosition(
        available.center().x() - window->width() / 2,
        available.top());
    bubbleController.showMessage(QStringLiteral("气泡窗口可见性验证"), 5000);
    QTRY_VERIFY(bubbleWindow->isVisible());
    QVERIFY(QTest::qWaitForWindowExposed(bubbleWindow));
    QTRY_VERIFY(bubbleController.arrowOnTop());
    QVERIFY(available.contains(bubbleWindow->geometry()));
    QVERIFY(bubbleWindow->geometry().top() > window->geometry().bottom());
    const QImage bubbleFrame = bubbleWindow->grabWindow();
    QVERIFY(!bubbleFrame.isNull());
    QCOMPARE(bubbleFrame.size(), bubbleWindow->size());
    QVERIFY(saveQaImage(bubbleFrame, QStringLiteral("speech-bubble.png")));

    bubbleController.showThinking();
    QVERIFY(bubbleController.isThinking());
    QTest::qWait(180);
    const QImage thinkingFrame = bubbleWindow->grabWindow();
    QVERIFY(!thinkingFrame.isNull());
    QVERIFY(saveQaImage(
        thinkingFrame, QStringLiteral("speech-bubble-thinking.png")));
    bubbleController.showMessage(QStringLiteral("我想好啦，我们继续聊吧。"), 5000);
    QVERIFY(!bubbleController.isThinking());

    window->setPosition(
        available.center().x() - window->width() / 2,
        available.bottom() - window->height() + 1);
    QTRY_VERIFY(!bubbleController.arrowOnTop());
    QVERIFY(available.contains(bubbleWindow->geometry()));
    QVERIFY(bubbleWindow->geometry().bottom() < window->geometry().top());

    interaction.openChat();
    QTRY_VERIFY(chatWindow->isVisible());
    QTRY_VERIFY(!bubbleWindow->isVisible());
    QVERIFY(QTest::qWaitForWindowExposed(chatWindow));
    QTest::qWait(180);
    auto* composerInput = chatWindow->findChild<QQuickItem*>(
        QStringLiteral("chatComposerInput"), Qt::FindChildrenRecursively);
    QVERIFY(composerInput != nullptr);
    QVERIFY(composerInput->mapToScene(QPointF()).x() <= 24);
    const QImage chatFrame = chatWindow->grabWindow();
    QVERIFY(!chatFrame.isNull());
    QCOMPARE(chatFrame.size(), chatWindow->size());
    QVERIFY(saveQaImage(chatFrame, QStringLiteral("chat-window.png")));
    auto* sendButton = chatWindow->findChild<QQuickItem*>(
        QStringLiteral("chatComposerSendButton"), Qt::FindChildrenRecursively);
    QVERIFY(sendButton != nullptr);
    QVERIFY(composerInput->setProperty("text", QStringLiteral("你好，今天想聊聊。")));
    QTRY_VERIFY(sendButton->property("enabled").toBool());
    QTest::qWait(120);
    const QImage readyChatFrame = chatWindow->grabWindow();
    QVERIFY(!readyChatFrame.isNull());
    QVERIFY(saveQaImage(
        readyChatFrame, QStringLiteral("chat-window-ready.png")));
    interaction.closeChat();
    QTRY_VERIFY(!chatWindow->isVisible());
    bubbleController.hide();
    QTRY_VERIFY(!bubbleWindow->isVisible());

    const QPoint petBottomRight(
        available.right() - window->width() + 1,
        available.bottom() - window->height() + 1);
    window->setPosition(petBottomRight);
    interaction.showSettings();
    QTRY_VERIFY(settingsWindow->isVisible());
    QVERIFY(QTest::qWaitForWindowExposed(settingsWindow));
    QTest::qWait(220);
    QTRY_VERIFY(available.contains(settingsWindow->geometry()));
    QVERIFY(settingsWindow->flags().testFlag(Qt::FramelessWindowHint));
    QVERIFY(settingsWindow->flags().testFlag(Qt::NoDropShadowWindowHint));
    auto* settingsGlassCard = settingsWindow->findChild<QQuickItem*>(
        QStringLiteral("settingsGlassCard"), Qt::FindChildrenRecursively);
    QVERIFY(settingsGlassCard != nullptr);
    auto* settingsCloseButton = settingsWindow->findChild<QQuickItem*>(
        QStringLiteral("settingsCloseButton"), Qt::FindChildrenRecursively);
    QVERIFY(settingsCloseButton != nullptr);
    auto* settingsMaxTokensInput = settingsWindow->findChild<QQuickItem*>(
        QStringLiteral("settingsMaxTokensInput"), Qt::FindChildrenRecursively);
    auto* settingsSaveButton = settingsWindow->findChild<QQuickItem*>(
        QStringLiteral("settingsSaveButton"), Qt::FindChildrenRecursively);
    QVERIFY(settingsMaxTokensInput != nullptr);
    QVERIFY(settingsSaveButton != nullptr);
    QTRY_VERIFY(settingsSaveButton->property("enabled").toBool());
    QVERIFY(settingsMaxTokensInput->setProperty("text", QStringLiteral("0")));
    QTRY_VERIFY(!settingsSaveButton->property("enabled").toBool());
    QVERIFY(settingsMaxTokensInput->setProperty("text", QStringLiteral("512")));
    QTRY_VERIFY(settingsSaveButton->property("enabled").toBool());
    const QImage settingsFrame = settingsWindow->grabWindow();
    QVERIFY(!settingsFrame.isNull());
    QCOMPARE(settingsFrame.size(), settingsWindow->size());
    QVERIFY(saveQaImage(settingsFrame, QStringLiteral("settings-window.png")));
    const QPoint settingsStart = settingsWindow->position();
    const QPoint headerDragStart(140, 45);
    const QPoint headerDragEnd = headerDragStart - QPoint(60, 20);
    QTest::mousePress(
        settingsWindow, Qt::LeftButton, Qt::NoModifier, headerDragStart);
    QTest::mouseMove(settingsWindow, headerDragEnd, 50);
    QTest::mouseRelease(
        settingsWindow, Qt::LeftButton, Qt::NoModifier, headerDragEnd);
    QTRY_VERIFY(settingsWindow->position() != settingsStart);
    QVERIFY(available.contains(settingsWindow->geometry()));
    auto* settingsFormScroll = settingsWindow->findChild<QQuickItem*>(
        QStringLiteral("settingsFormScroll"), Qt::FindChildrenRecursively);
    QVERIFY(settingsFormScroll != nullptr);
    settingsWindow->setHeight(420);
    QTRY_VERIFY(available.contains(settingsWindow->geometry()));
    QTRY_VERIFY(settingsFormScroll->property("contentHeight").toReal()
                > settingsFormScroll->height());
    const QPoint closeButtonCenter = settingsCloseButton->mapToScene(
        QPointF(settingsCloseButton->width() / 2,
                settingsCloseButton->height() / 2)).toPoint();
    QTest::mouseClick(
        settingsWindow, Qt::LeftButton, Qt::NoModifier, closeButtonCenter);
    QTRY_VERIFY(!settingsWindow->isVisible());

    const QVector<QPoint> otherEdges {
        available.topLeft(),
        QPoint(available.right() - window->width() + 1, available.top()),
        QPoint(available.left(), available.bottom() - window->height() + 1)
    };
    for (const QPoint& petPosition : otherEdges) {
        window->setPosition(petPosition);
        interaction.showSettings();
        QTRY_VERIFY(settingsWindow->isVisible());
        QTest::qWait(80);
        QTRY_VERIFY(available.contains(settingsWindow->geometry()));
        interaction.closeSettings();
        QTRY_VERIFY(!settingsWindow->isVisible());
    }
}

} // namespace

QTEST_MAIN(PetWindowControllerTest)

#include "PetWindowControllerTest.moc"
