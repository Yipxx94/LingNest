#include <QtTest>

#include <utility>

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "animation/AnimationCatalog.h"
#include "animation/PngSequenceAnimationPlayer.h"

namespace {

bool writeFile(const QString& path, const QByteArray& contents)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    return file.write(contents) == contents.size();
}

class AnimationPlayerTest final : public QObject {
    Q_OBJECT

private slots:
    void catalogLoadsFrameTiming();
    void oncePlaybackFinishes();
    void loopPlaybackContinuesUntilStopped();

private:
    [[nodiscard]] std::optional<lingnest::animation::AnimationCatalog> createCatalog(
        QTemporaryDir& temporaryDirectory,
        QString* errorMessage = nullptr);
};

std::optional<lingnest::animation::AnimationCatalog> AnimationPlayerTest::createCatalog(
    QTemporaryDir& temporaryDirectory,
    QString* errorMessage)
{
    if (!temporaryDirectory.isValid()) {
        return std::nullopt;
    }

    const QString packageDirectory = temporaryDirectory.path();
    if (!writeFile(
            QDir(packageDirectory).filePath(QStringLiteral("frame-0.png")),
            QByteArrayLiteral("frame-zero"))
        || !writeFile(
            QDir(packageDirectory).filePath(QStringLiteral("frame-1.png")),
            QByteArrayLiteral("frame-one"))) {
        return std::nullopt;
    }

    const QString manifestPath =
        QDir(packageDirectory).filePath(QStringLiteral("animations.json"));
    if (!writeFile(manifestPath, QByteArrayLiteral(R"({
        "actions": {
            "fast": {
                "frame_count": 2,
                "loop": true,
                "loop_start_frame": 1,
                "durations_ms": [20, 30],
                "files": ["frame-0.png", "frame-1.png"]
            }
        }
    })"))) {
        return std::nullopt;
    }

    return lingnest::animation::AnimationCatalog::load(
        manifestPath, packageDirectory, errorMessage);
}

void AnimationPlayerTest::catalogLoadsFrameTiming()
{
    QTemporaryDir temporaryDirectory;
    QString errorMessage;
    const auto catalog = createCatalog(temporaryDirectory, &errorMessage);

    QVERIFY2(catalog.has_value(), qPrintable(errorMessage));
    QCOMPARE(catalog->size(), 1);
    const auto* clip = catalog->clip(QStringLiteral("fast"));
    QVERIFY(clip != nullptr);
    QCOMPARE(clip->frames.size(), 2);
    QCOMPARE(clip->durationsMs, QVector<int>({20, 30}));
    QVERIFY(clip->loop);
    QCOMPARE(clip->loopStartFrame, 1);
}

void AnimationPlayerTest::oncePlaybackFinishes()
{
    QTemporaryDir temporaryDirectory;
    QString errorMessage;
    auto catalog = createCatalog(temporaryDirectory, &errorMessage);
    QVERIFY2(catalog.has_value(), qPrintable(errorMessage));

    lingnest::animation::PngSequenceAnimationPlayer player(std::move(*catalog));
    QSignalSpy frameSpy(&player,
        &lingnest::animation::PngSequenceAnimationPlayer::currentFrameChanged);
    QSignalSpy finishedSpy(&player,
        &lingnest::animation::IAnimationPlayer::animationFinished);

    QVERIFY(player.play(
        QStringLiteral("fast"),
        lingnest::animation::IAnimationPlayer::PlaybackMode::Once));
    QCOMPARE(player.currentAnimation(), QStringLiteral("fast"));
    QVERIFY(player.isPlaying());
    QTRY_COMPARE(finishedSpy.count(), 1);
    QVERIFY(!player.isPlaying());
    QVERIFY(frameSpy.count() >= 2);
    QCOMPARE(finishedSpy.constFirst().constFirst().toString(), QStringLiteral("fast"));
}

void AnimationPlayerTest::loopPlaybackContinuesUntilStopped()
{
    QTemporaryDir temporaryDirectory;
    QString errorMessage;
    auto catalog = createCatalog(temporaryDirectory, &errorMessage);
    QVERIFY2(catalog.has_value(), qPrintable(errorMessage));

    lingnest::animation::PngSequenceAnimationPlayer player(std::move(*catalog));
    QSignalSpy finishedSpy(&player,
        &lingnest::animation::IAnimationPlayer::animationFinished);
    QSignalSpy frameSpy(&player,
        &lingnest::animation::PngSequenceAnimationPlayer::currentFrameChanged);

    QVERIFY(player.play(
        QStringLiteral("fast"),
        lingnest::animation::IAnimationPlayer::PlaybackMode::Loop));
    QTest::qWait(130);
    QVERIFY(player.isPlaying());
    QCOMPARE(finishedSpy.count(), 0);
    QCOMPARE(frameSpy.count(), 2);
    player.stop();
    QVERIFY(!player.isPlaying());
}

} // namespace

QTEST_GUILESS_MAIN(AnimationPlayerTest)

#include "AnimationPlayerTest.moc"
