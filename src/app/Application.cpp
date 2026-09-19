#include "app/Application.h"
#include "app/SingleInstanceGuard.h"

#include <cstdlib>
#include <utility>

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>
#include <QUrl>
#include <QtGlobal>

#include "animation/AnimationCatalog.h"
#include "animation/PngSequenceAnimationPlayer.h"
#include "ai/OpenAICompatibleProvider.h"
#include "character/CharacterLoader.h"
#include "config/ConfigManager.h"
#include "interaction/InteractionController.h"
#include "memory/MemoryRepository.h"
#include "memory/PromptBuilder.h"
#include "pet/PetScheduler.h"
#include "pet/PetStateMachine.h"
#include "platform/windows/WindowsCredentialStore.h"
#include "ui/AISettingsController.h"
#include "ui/ChatController.h"
#include "ui/PetWindowController.h"
#include "ui/SpeechBubbleController.h"
#include "ui/SystemTrayController.h"

namespace lingnest::app {

int run(int argc, char* argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QCoreApplication::setOrganizationName(QStringLiteral("LingNest"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("lingnest.local"));
    QCoreApplication::setApplicationName(QStringLiteral("LingNest"));
    QCoreApplication::setApplicationVersion(QStringLiteral(LINGNEST_VERSION));

    QApplication application(argc, argv);
    application.setQuitOnLastWindowClosed(false);

    // Use the exact same multi-resolution ICO for the executable, every Qt
    // window and the notification area. This avoids Windows rendering two
    // subtly different application identities from separate image sources.
    QIcon applicationIcon(QStringLiteral(":/icons/lingnest.ico"));
    if (applicationIcon.isNull()) {
        for (const int size : {16, 20, 24, 32, 48, 64, 128, 256}) {
            applicationIcon.addFile(
                QStringLiteral(":/icons/lingnest_tray_%1.png").arg(size),
                QSize(size, size));
        }
    }
    application.setWindowIcon(applicationIcon);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("LingNest multi-character AI desktop pet"));
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption smokeTestOption(
        QStringLiteral("smoke-test"),
        QStringLiteral("Load the application shell and exit immediately."));
    parser.addOption(smokeTestOption);

    const QCommandLineOption configFileOption(
        QStringLiteral("config-file"),
        QStringLiteral("Use an alternate config file (intended for testing)."),
        QStringLiteral("path"));
    parser.addOption(configFileOption);
    parser.process(application);

    SingleInstanceGuard instanceGuard;
    if (!parser.isSet(smokeTestOption) && !instanceGuard.tryAcquire()) {
        qInfo("LingNest is already running; the duplicate launch will exit.");
        return EXIT_SUCCESS;
    }

    const QString characterDirectory = QDir(QCoreApplication::applicationDirPath())
                                           .filePath(QStringLiteral("characters/yuai"));
    QString characterError;
    auto character = character::CharacterLoader::loadFromDirectory(
        characterDirectory, &characterError);
    if (!character.has_value()) {
        qCritical().noquote() << characterError;
        return EXIT_FAILURE;
    }

    QFile promptFile(character->promptFilePath);
    if (!promptFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical().noquote()
            << QStringLiteral("Cannot read character prompt %1: %2")
                   .arg(
                       QDir::toNativeSeparators(character->promptFilePath),
                       promptFile.errorString());
        return EXIT_FAILURE;
    }
    const QString characterPrompt = QString::fromUtf8(promptFile.readAll()).trimmed();

    QString animationError;
    auto animationCatalog = animation::AnimationCatalog::load(
        character->animationManifestPath,
        character->packageDirectory,
        &animationError);
    if (!animationCatalog.has_value()) {
        qCritical().noquote() << animationError;
        return EXIT_FAILURE;
    }

    pet::PetAnimationSet petAnimations;
    petAnimations.idle = character->stateAnimations.value(QStringLiteral("idle"));
    petAnimations.blink = character->stateAnimations.value(QStringLiteral("blink"));
    petAnimations.walkLeft = character->stateAnimations.value(QStringLiteral("walkLeft"));
    petAnimations.walkRight = character->stateAnimations.value(QStringLiteral("walkRight"));
    petAnimations.talk = character->stateAnimations.value(QStringLiteral("talk"));
    petAnimations.happy = character->stateAnimations.value(QStringLiteral("happy"));
    petAnimations.wave = character->stateAnimations.value(QStringLiteral("wave"));
    petAnimations.daze = character->stateAnimations.value(QStringLiteral("daze"));
    petAnimations.sleep = character->stateAnimations.value(QStringLiteral("sleep"));
    petAnimations.thinking = character->stateAnimations.value(QStringLiteral("thinking"));

    const QString configFilePath = parser.isSet(configFileOption)
        ? QFileInfo(parser.value(configFileOption)).absoluteFilePath()
        : config::ConfigManager::defaultFilePath();
    config::ConfigManager configManager(configFilePath);
    config::ConfigManager* activeConfig = nullptr;
    if (!parser.isSet(smokeTestOption)) {
        QString configError;
        if (!configManager.loadWithRecovery(&configError)) {
            qWarning().noquote() << configError;
        } else if (!configError.isEmpty()) {
            qWarning().noquote() << configError;
        }
        activeConfig = &configManager;
    }

    memory::MemoryRepository memoryRepository;
    memory::MemoryRepository* activeMemory = nullptr;
    if (!parser.isSet(smokeTestOption)) {
        QString memoryError;
        if (memoryRepository.openWithRecovery(&memoryError)) {
            activeMemory = &memoryRepository;
            if (!memoryError.isEmpty()) {
                qWarning().noquote() << memoryError;
            }
        } else {
            qWarning().noquote() << memoryError;
        }
    }
    memory::PromptBuilder promptBuilder;

    ai::OpenAICompatibleProvider aiProvider({}, &application);
    platform::windows::WindowsCredentialStore credentialStore;
    ui::AISettingsController aiSettingsController(
        &configManager,
        &credentialStore,
        &aiProvider,
        !parser.isSet(smokeTestOption),
        &application);
    ui::ChatController chatController(
        *character,
        &aiProvider,
        characterPrompt,
        &application,
        activeMemory,
        &promptBuilder);
    ui::PetWindowController petWindowController(
        std::move(*character), activeConfig, &application);
    animation::PngSequenceAnimationPlayer animationPlayer(
        std::move(*animationCatalog), &application);
    pet::PetStateMachine stateMachine(
        std::move(petAnimations), &animationPlayer, &application);
    pet::PetScheduler scheduler(&stateMachine, &application);
    ui::SpeechBubbleController bubbleController(&application);
    interaction::InteractionController interactionController(
        &stateMachine, &scheduler, &bubbleController, &application);
    ui::SystemTrayController trayController(applicationIcon, &application);
    trayController.setCurrentCharacterName(petWindowController.displayName());

    QObject::connect(
        &petWindowController,
        &ui::PetWindowController::userActivity,
        &scheduler,
        &pet::PetScheduler::notifyUserActivity);
    QObject::connect(
        &stateMachine,
        &pet::PetStateMachine::walkStarted,
        &petWindowController,
        &ui::PetWindowController::startWalking);
    QObject::connect(
        &stateMachine,
        &pet::PetStateMachine::walkStopped,
        &petWindowController,
        &ui::PetWindowController::stopWalking);
    QObject::connect(
        &stateMachine,
        &pet::PetStateMachine::walkDirectionChanged,
        &petWindowController,
        &ui::PetWindowController::setWalkingDirection);
    QObject::connect(
        &petWindowController,
        &ui::PetWindowController::walkBoundaryTurned,
        &stateMachine,
        &pet::PetStateMachine::setWalkDirection);
    QObject::connect(
        &stateMachine,
        &pet::PetStateMachine::animationError,
        &application,
        [](const QString& message) {
            qWarning().noquote() << message;
        });
    QObject::connect(
        &interactionController,
        &interaction::InteractionController::quitRequested,
        &application,
        &QCoreApplication::quit);
    QObject::connect(
        &trayController,
        &ui::SystemTrayController::chatRequested,
        &interactionController,
        &interaction::InteractionController::openChat);
    QObject::connect(
        &trayController,
        &ui::SystemTrayController::characterSwitchRequested,
        &interactionController,
        &interaction::InteractionController::showCharacterSwitcher);
    QObject::connect(
        &trayController,
        &ui::SystemTrayController::settingsRequested,
        &interactionController,
        &interaction::InteractionController::showSettings);
    QObject::connect(
        &trayController,
        &ui::SystemTrayController::waveRequested,
        &interactionController,
        [&interactionController]() {
            interactionController.playDebugAction(QStringLiteral("wave"));
        });
    QObject::connect(
        &trayController,
        &ui::SystemTrayController::pauseToggleRequested,
        &interactionController,
        &interaction::InteractionController::togglePaused);
    QObject::connect(
        &interactionController,
        &interaction::InteractionController::pausedChanged,
        &trayController,
        [&interactionController, &trayController]() {
            trayController.setPaused(interactionController.isPaused());
        });
    QObject::connect(
        &trayController,
        &ui::SystemTrayController::quitRequested,
        &application,
        &QCoreApplication::quit);
    QObject::connect(
        &chatController,
        &ui::ChatController::userMessageSubmitted,
        &scheduler,
        [&scheduler,
         &stateMachine,
         &interactionController,
         &bubbleController](const QString&) {
            scheduler.notifyUserActivity();
            stateMachine.beginConversation();
            interactionController.closeChat();
            bubbleController.showThinking();
        });
    QObject::connect(
        &chatController,
        &ui::ChatController::assistantMessageReady,
        &bubbleController,
        [&bubbleController](const QString& message) {
            const int durationMs = qBound(5000, message.size() * 85, 20000);
            bubbleController.showMessage(message, durationMs);
        });
    QObject::connect(
        &chatController,
        &ui::ChatController::assistantErrorReady,
        &bubbleController,
        [&bubbleController](const QString& message) {
            bubbleController.showMessage(message, 8000);
        });
    QObject::connect(
        &chatController,
        &ui::ChatController::assistantResponseReady,
        &stateMachine,
        &pet::PetStateMachine::assistantResponseReady);
    QObject::connect(
        &chatController,
        &ui::ChatController::assistantResponseFailed,
        &stateMachine,
        &pet::PetStateMachine::forceIdle);

    stateMachine.start();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(
        QStringLiteral("petController"), &petWindowController);
    engine.rootContext()->setContextProperty(
        QStringLiteral("petAnimation"), &animationPlayer);
    engine.rootContext()->setContextProperty(
        QStringLiteral("petInteraction"), &interactionController);
    engine.rootContext()->setContextProperty(
        QStringLiteral("bubbleController"), &bubbleController);
    engine.rootContext()->setContextProperty(
        QStringLiteral("chatController"), &chatController);
    engine.rootContext()->setContextProperty(
        QStringLiteral("aiSettings"), &aiSettingsController);

    const QUrl mainQml(QStringLiteral("qrc:/qml/Main.qml"));

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &application,
        [mainQml](QObject* object, const QUrl& objectUrl) {
            if (object == nullptr && objectUrl == mainQml) {
                QCoreApplication::exit(EXIT_FAILURE);
            }
        },
        Qt::QueuedConnection);

    engine.load(mainQml);
    if (engine.rootObjects().isEmpty()) {
        return EXIT_FAILURE;
    }

    auto* petWindow = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
    if (petWindow == nullptr) {
        qCritical("The QML root object must be a QQuickWindow");
        return EXIT_FAILURE;
    }

    for (QWindow* window : QGuiApplication::allWindows()) {
        if (window != nullptr) {
            window->setIcon(applicationIcon);
        }
    }

    petWindowController.attachWindow(petWindow);
    QObject::connect(
        &trayController,
        &ui::SystemTrayController::petVisibilityToggleRequested,
        petWindow,
        [petWindow, &petWindowController, &trayController]() {
            const bool shouldShow = !petWindow->isVisible();
            petWindow->setVisible(shouldShow);
            trayController.setPetVisible(shouldShow);
            if (shouldShow) {
                petWindowController.restorePosition();
                petWindow->raise();
                petWindow->requestActivate();
            }
        });
    QObject::connect(
        &trayController,
        &ui::SystemTrayController::petShowRequested,
        petWindow,
        [petWindow, &petWindowController, &trayController]() {
            if (!petWindow->isVisible()) {
                petWindow->setVisible(true);
                petWindowController.restorePosition();
                trayController.setPetVisible(true);
            }
            petWindow->raise();
            petWindow->requestActivate();
        });
    QObject::connect(
        petWindow,
        &QWindow::visibleChanged,
        &trayController,
        &ui::SystemTrayController::setPetVisible);
    trayController.setPetVisible(petWindow->isVisible());
    auto* bubbleWindow = petWindow->findChild<QQuickWindow*>(
        QStringLiteral("speechBubbleWindow"), Qt::FindChildrenRecursively);
    if (bubbleWindow == nullptr) {
        qCritical("Cannot find the speech bubble QML window");
        return EXIT_FAILURE;
    }
    bubbleController.attachWindows(petWindow, bubbleWindow);
    scheduler.start();
    if (!parser.isSet(smokeTestOption) && !trayController.show()) {
        qWarning("Windows system tray is not available; tray icon was not shown.");
    }
    QTimer::singleShot(0, &petWindowController, [&petWindowController]() {
        petWindowController.restorePosition();
    });
    QTimer::singleShot(700, &stateMachine, [&stateMachine]() {
        stateMachine.requestWave();
    });

    QObject::connect(
        &application,
        &QCoreApplication::aboutToQuit,
        &petWindowController,
        &ui::PetWindowController::savePosition);

    if (parser.isSet(smokeTestOption)) {
        QTimer::singleShot(0, &application, [&application]() {
            application.quit();
        });
    }

    return application.exec();
}

} // namespace lingnest::app
