#include <QtTest>

#include "memory/ExplicitMemoryParser.h"
#include "memory/PromptBuilder.h"

class PromptBuilderTest final : public QObject {
    Q_OBJECT

private slots:
    void parsesExplicitMemoriesAndProfiles();
    void ordersPromptAndSelectsRelevantMemory();
    void clipsConversationToConfiguredBudget();
};

void PromptBuilderTest::parsesExplicitMemoriesAndProfiles()
{
    const auto preference = lingnest::memory::ExplicitMemoryParser::parse(
        QStringLiteral("请记住：我喜欢雨天散步。"));
    QVERIFY(preference.has_value());
    QCOMPARE(preference->type, QStringLiteral("Preference"));
    QCOMPARE(preference->content, QStringLiteral("我喜欢雨天散步"));

    const auto profile = lingnest::memory::ExplicitMemoryParser::parse(
        QStringLiteral("记住，我叫小林"));
    QVERIFY(profile.has_value());
    QCOMPARE(profile->profileKey, QStringLiteral("name"));
    QCOMPARE(profile->profileValue, QStringLiteral("小林"));
    QCOMPARE(profile->key, QStringLiteral("profile:name"));

    QVERIFY(!lingnest::memory::ExplicitMemoryParser::parse(
        QStringLiteral("我今天只是随便聊聊")).has_value());
    QVERIFY(!lingnest::memory::ExplicitMemoryParser::parse(
        QStringLiteral("记住：")).has_value());
}

void PromptBuilderTest::ordersPromptAndSelectsRelevantMemory()
{
    lingnest::memory::PromptBuilder::Options options;
    options.maxContextCharacters = 2000;
    options.maxRecentMessages = 4;
    options.maxMemories = 1;
    lingnest::memory::PromptBuilder builder(options);

    QVector<lingnest::ai::AIChatMessage> history {
        {lingnest::ai::ChatRole::User, QStringLiteral("上一轮问题")},
        {lingnest::ai::ChatRole::Assistant, QStringLiteral("上一轮回答")}
    };
    QVector<lingnest::memory::MemoryEntry> memories;
    lingnest::memory::MemoryEntry unrelated;
    unrelated.type = QStringLiteral("Fact");
    unrelated.content = QStringLiteral("用户养了一盆薄荷");
    memories.append(unrelated);
    lingnest::memory::MemoryEntry relevant;
    relevant.type = QStringLiteral("Preference");
    relevant.content = QStringLiteral("用户喜欢吃竹子味的小点心");
    memories.append(relevant);
    QHash<QString, QString> profile {
        {QStringLiteral("name"), QStringLiteral("小林")}
    };

    const auto request = builder.build(
        QStringLiteral("你是渝爱。"),
        history,
        memories,
        profile,
        QStringLiteral("我之前提过竹子吗？"));

    QCOMPARE(request.messages.size(), 5);
    QVERIFY(request.messages.at(0).role == lingnest::ai::ChatRole::System);
    QCOMPARE(request.messages.at(0).content, QStringLiteral("你是渝爱。"));
    QVERIFY(request.messages.at(1).role == lingnest::ai::ChatRole::System);
    QVERIFY(request.messages.at(1).content.contains(QStringLiteral("称呼：小林")));
    QVERIFY(request.messages.at(1).content.contains(QStringLiteral("竹子味")));
    QVERIFY(!request.messages.at(1).content.contains(QStringLiteral("薄荷")));
    QVERIFY(request.messages.at(2).role == lingnest::ai::ChatRole::User);
    QVERIFY(request.messages.at(3).role == lingnest::ai::ChatRole::Assistant);
    QVERIFY(request.messages.at(4).role == lingnest::ai::ChatRole::User);
    QCOMPARE(
        request.messages.at(4).content,
        QStringLiteral("我之前提过竹子吗？"));
}

void PromptBuilderTest::clipsConversationToConfiguredBudget()
{
    lingnest::memory::PromptBuilder::Options options;
    options.maxContextCharacters = 512;
    options.maxRecentMessages = 100;
    options.maxMemories = 10;
    lingnest::memory::PromptBuilder builder(options);

    QVector<lingnest::ai::AIChatMessage> history;
    for (int index = 0; index < 50; ++index) {
        history.append({
            index % 2 == 0
                ? lingnest::ai::ChatRole::User
                : lingnest::ai::ChatRole::Assistant,
            QString(100, QLatin1Char('h'))});
    }
    QVector<lingnest::memory::MemoryEntry> memories;
    lingnest::memory::MemoryEntry memory;
    memory.type = QStringLiteral("Fact");
    memory.content = QString(500, QLatin1Char('m'));
    memories.append(memory);

    const auto request = builder.build(
        QString(1000, QLatin1Char('s')),
        history,
        memories,
        {},
        QString(1000, QLatin1Char('u')));

    int totalCharacters = 0;
    for (const auto& message : request.messages) {
        totalCharacters += message.content.size();
    }
    QVERIFY(totalCharacters <= options.maxContextCharacters);
    QVERIFY(!request.messages.isEmpty());
    QVERIFY(request.messages.constLast().role == lingnest::ai::ChatRole::User);
    QVERIFY(request.messages.constLast().content.endsWith(QStringLiteral("…")));
}

QTEST_GUILESS_MAIN(PromptBuilderTest)

#include "PromptBuilderTest.moc"
