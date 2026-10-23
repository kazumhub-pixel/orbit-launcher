// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include <QTemporaryDir>
#include "ui/widgets/OrbitProfilesDialog.h"
class OrbitProfilesTest : public QObject {
    Q_OBJECT
private slots:
    void localProfilePersistsAndRejectsDuplicates() {
        QTemporaryDir temp;QVERIFY(temp.isValid());
        AccountList accounts;accounts.setListFilePath(temp.filePath("accounts.json"),false);
        bool enabled=false;
        OrbitProfilesDialog dialog(&accounts,temp.filePath("skins"),nullptr,temp.filePath("game"),false,[&](bool value){enabled=value;});
        auto* automatic=dialog.findChild<QCheckBox*>("automaticSkins");QVERIFY(automatic);automatic->setChecked(true);QVERIFY(enabled);
        auto* name=dialog.findChild<QLineEdit*>("localNickname");
        auto* create=dialog.findChild<QPushButton*>("createLocal");
        QVERIFY(name && create);QVERIFY(!create->isEnabled());
        name->setText("bad name");QVERIFY(!create->isEnabled());
        name->setText("OrbitPlayer");QVERIFY(create->isEnabled());create->click();
        QCOMPARE(accounts.count(),1);QVERIFY(accounts.defaultAccount());
        QCOMPARE(accounts.defaultAccount()->profileName(),QString("OrbitPlayer"));
        QCOMPARE(accounts.defaultAccount()->accountType(),AccountType::Offline);
        QVERIFY(!accounts.defaultAccount()->ownsMinecraft());
        name->setText("orbitplayer");QVERIFY(!create->isEnabled());
        AccountList restored;restored.setListFilePath(temp.filePath("accounts.json"),false);QVERIFY(restored.loadList());
        QCOMPARE(restored.count(),1);QVERIFY(restored.defaultAccount());
        QCOMPARE(restored.defaultAccount()->profileId(),accounts.defaultAccount()->profileId());
        const auto output=qEnvironmentVariable("ORBIT_GALLERY_DIR");
        if(!output.isEmpty()){dialog.show();QTest::qWait(60);QVERIFY(dialog.grab().save(output+"/profiles.png"));}
    }
    void skinValidationAndPersistence() {
        QTemporaryDir temp;QVERIFY(temp.isValid());
        OrbitLocalSkins skins(temp.filePath("skins"));
        const auto id=MinecraftAccount::uuidFromUsername("OrbitPlayer").toString(QUuid::Id128);
        QImage valid(64,64,QImage::Format_ARGB32);valid.fill(QColor("#aacdf4"));
        const auto source=temp.filePath("skin.png");QVERIFY(valid.save(source));QString error;
        QVERIFY(skins.importSkin(id,source,error));QCOMPARE(QImage(skins.path(id)).size(),QSize(64,64));
        QVERIFY(!skins.importSkin("../outside",source,error));
        QImage invalid(2048,2048,QImage::Format_ARGB32);invalid.fill(Qt::white);
        QVERIFY(invalid.save(temp.filePath("bad.png")));
        QVERIFY(!skins.importSkin(id,temp.filePath("bad.png"),error));
        QVERIFY(valid.save(temp.filePath("fake.png"),"BMP"));
        QVERIFY(!skins.importSkin(id,temp.filePath("fake.png"),error));
        QCOMPARE(QImage(skins.path(id)).size(),QSize(64,64));
        QVERIFY(skins.prepareInstance(id,"OrbitPlayer",temp.filePath("minecraft"),error));
        QCOMPARE(QImage(temp.filePath("minecraft/CustomSkinLoader/LocalSkin/skins/OrbitPlayer.png")).size(),QSize(64,64));
        QVERIFY(!skins.prepareInstance(id,"../escape",temp.filePath("minecraft"),error));
        QVERIFY(skins.remove(id));QVERIFY(!QFile::exists(skins.path(id)));
    }
};
QTEST_MAIN(OrbitProfilesTest)
#include "OrbitProfiles_test.moc"
