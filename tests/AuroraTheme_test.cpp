// SPDX-License-Identifier: GPL-3.0-only
#include "ui/themes/AuroraTheme.h"
#include <QApplication>
#include <QComboBox>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QtTest>
#include "minecraft/auth/MinecraftAccount.h"
#include "minecraft/auth/AuthSession.h"
#include "ui/dialogs/AuroraThemeDialog.h"

class AuroraThemeTest : public QObject {
    Q_OBJECT
   private slots:
    void packageRoundTrip()
    {
        const auto source = AuroraStyle::preset(1);
        AuroraStyle parsed;
        QString error;
        QVERIFY2(AuroraStyle::parse(source.package(), parsed, error), qPrintable(error));
        QCOMPARE(parsed.tokens(), source.tokens());
    }
    void rejectInvalidPackages()
    {
        auto root = AuroraStyle().package();
        auto tokens = root["tokens"].toObject();
        tokens["accent"] = "not-a-color";
        root["tokens"] = tokens;
        AuroraStyle parsed;
        QString error;
        QVERIFY(!AuroraStyle::parse(root, parsed, error));
        root = AuroraStyle().package();
        tokens = root["tokens"].toObject();
        tokens["spacing"] = 10000;
        root["tokens"] = tokens;
        QVERIFY(!AuroraStyle::parse(root, parsed, error));
    }
    void atomicInstallation()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        AuroraStyle style;
        style.name = "../../nome de tema";
        QString id, error;
        QVERIFY2(style.install(QDir(dir.path()), id, error), qPrintable(error));
        QVERIFY(id.startsWith("aurora-"));
        QVERIFY(!id.contains('/'));
        const auto folder = QDir(dir.path()).absoluteFilePath(id);
        QFile f(folder + "/theme.json");
        QVERIFY(f.open(QIODevice::ReadOnly));
        const auto json = QJsonDocument::fromJson(f.readAll()).object();
        QCOMPARE(json["name"].toString(), style.name);
        QVERIFY(QFile::exists(folder + "/themeStyle.css"));
        QVERIFY(QFile::exists(folder + "/aurora.json"));
        const auto before = QDir(dir.path()).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        QCOMPARE(before.size(), 1);
    }
    void localIdentityDoesNotClaimOwnership()
    {
        const auto a = MinecraftAccount::createOffline("AuroraTest");
        const auto b = MinecraftAccount::createOffline("AuroraTest");
        QCOMPARE(a->accountType(), AccountType::Offline);
        QVERIFY(!a->ownsMinecraft());
        QCOMPARE(a->profileId(), b->profileId());
        const auto reloaded = MinecraftAccount::loadFromJsonV3(a->saveToJson());
        QVERIFY(reloaded);
        QCOMPARE(reloaded->profileName(), QString("AuroraTest"));
        QVERIFY(!reloaded->ownsMinecraft());
    }
    void localSessionKeepsDownloadPermission()
    {
        AuthSession session;
        session.launchMode = LaunchMode::Offline;
        QVERIFY(!session.allowGameDownloads);
        session.allowGameDownloads = true;
        QVERIFY(session.MakeOffline("AuroraTest"));
        QVERIFY(session.allowGameDownloads);
        QCOMPARE(session.launchMode, LaunchMode::Offline);
        QCOMPARE(session.access_token, QString("0"));
    }
    void editorPreservesPresetValues()
    {
        QTemporaryDir dir;
        AuroraThemeDialog dialog(QDir(dir.path()));
        auto combos = dialog.findChildren<QComboBox*>();
        QComboBox* presets = nullptr;
        for (auto* combo : combos)
            if (combo->findText("Orbit Light") >= 0)
                presets = combo;
        QVERIFY(presets);
        presets->setCurrentIndex(1);
        const auto controls = dialog.findChildren<QSpinBox*>();
        QCOMPARE(controls.size(), 3);
        QCOMPARE(controls[0]->value(), 10);
        QCOMPARE(controls[1]->value(), 8);
        QCOMPARE(controls[2]->value(), 8);
        dialog.resize(900, 650);
        dialog.show();
        QTest::qWait(50);
        const auto screenshot = qEnvironmentVariable("AURORA_THEME_SCREENSHOT");
        if (!screenshot.isEmpty())
            QVERIFY(dialog.grab().save(screenshot));
    }
};
QTEST_MAIN(AuroraThemeTest)
#include "AuroraTheme_test.moc"
