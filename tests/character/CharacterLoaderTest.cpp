#include <QtTest>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "character/CharacterLoader.h"

namespace {

bool writeFile(const QString& path, const QByteArray& contents)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    return file.write(contents) == contents.size();
}

class CharacterLoaderTest final : public QObject {
    Q_OBJECT

private slots:
    void loadsYuaiPackage();
    void rejectsMissingPackage();
    void rejectsPathTraversal();
};

void CharacterLoaderTest::loadsYuaiPackage()
{
    QString errorMessage;
    const auto definition = lingnest::character::CharacterLoader::loadFromDirectory(
        QStringLiteral(LINGNEST_TEST_CHARACTER_DIR), &errorMessage);

    QVERIFY2(definition.has_value(), qPrintable(errorMessage));
    QCOMPARE(definition->id, QStringLiteral("yuai"));
    QCOMPARE(definition->displayName, QStringLiteral("渝爱"));
    QCOMPARE(definition->defaultAnimation, QStringLiteral("idle"));
    QCOMPARE(definition->scale, 1.0);
    QCOMPARE(definition->availableAnimations.size(), 10);
    QVERIFY(definition->availableAnimations.contains(QStringLiteral("sleep")));
    QCOMPARE(definition->stateAnimations.value(QStringLiteral("daze")),
        QStringLiteral("daze_prone"));
    QCOMPARE(definition->stateAnimations.value(QStringLiteral("wave")),
        QStringLiteral("wave"));
    QVERIFY(QFileInfo::exists(definition->defaultFrameUrl.toLocalFile()));
    QVERIFY(QFileInfo::exists(definition->avatarUrl.toLocalFile()));
    QVERIFY(QFileInfo::exists(definition->promptFilePath));
}

void CharacterLoaderTest::rejectsMissingPackage()
{
    QString errorMessage;
    const auto definition = lingnest::character::CharacterLoader::loadFromDirectory(
        QStringLiteral("Z:/a-directory-that-does-not-exist/yuai"), &errorMessage);

    QVERIFY(!definition.has_value());
    QVERIFY(!errorMessage.isEmpty());
}

void CharacterLoaderTest::rejectsPathTraversal()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString packageDirectory =
        QDir(temporaryDirectory.path()).filePath(QStringLiteral("yuai"));
    QVERIFY(QDir().mkpath(packageDirectory));

    QVERIFY(writeFile(
        QDir(temporaryDirectory.path()).filePath(QStringLiteral("outside.png")),
        QByteArrayLiteral("not-an-image")));
    QVERIFY(writeFile(
        QDir(packageDirectory).filePath(QStringLiteral("avatar.png")),
        QByteArrayLiteral("not-an-image")));
    QVERIFY(writeFile(
        QDir(packageDirectory).filePath(QStringLiteral("prompt.md")),
        QByteArrayLiteral("test")));
    QVERIFY(writeFile(
        QDir(packageDirectory).filePath(QStringLiteral("animations.json")),
        QByteArrayLiteral(R"({
            "actions": {
                "idle": {
                    "files": ["../outside.png"],
                    "durations_ms": [100]
                }
            }
        })")));
    QVERIFY(writeFile(
        QDir(packageDirectory).filePath(QStringLiteral("character.json")),
        QByteArrayLiteral(R"({
            "id": "yuai",
            "name": "yuai",
            "displayName": "Yuai",
            "avatar": "avatar.png",
            "prompt": "prompt.md",
            "animations": "animations.json",
            "defaultAnimation": "idle",
            "scale": 1.0,
            "stateAnimations": {
                "idle": "idle",
                "blink": "idle",
                "walkLeft": "idle",
                "walkRight": "idle",
                "talk": "idle",
                "happy": "idle",
                "wave": "idle",
                "daze": "idle",
                "sleep": "idle",
                "thinking": "idle"
            },
            "availableAnimations": ["idle"]
        })")));

    QString errorMessage;
    const auto definition = lingnest::character::CharacterLoader::loadFromDirectory(
        packageDirectory, &errorMessage);

    QVERIFY(!definition.has_value());
    QVERIFY2(errorMessage.contains(QStringLiteral("escapes")), qPrintable(errorMessage));
}

} // namespace

QTEST_APPLESS_MAIN(CharacterLoaderTest)

#include "CharacterLoaderTest.moc"
