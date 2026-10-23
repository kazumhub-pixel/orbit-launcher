// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include <QComboBox>
#include <QFontDatabase>
#include <QRawFont>
#include <QDir>
#include <QListWidget>
#include <QTemporaryDir>
#include "translations/OrbitI18n.h"
#include "ui/widgets/OrbitShell.h"
#include "ui/widgets/OrbitProfilesDialog.h"
#include "ui/dialogs/AuroraThemeDialog.h"

class OrbitI18nTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase(){Q_INIT_RESOURCE(AuroraLauncher);}
    void allSixLanguages_data(){
        QTest::addColumn<QString>("language");
        for(const auto* code:{"en_US","pt_BR","ru","zh","es","ar"})QTest::newRow(code)<<QString(code);
    }
    void allSixLanguages(){
        QFETCH(QString,language);
        QTranslator orbit,base,qt;
        QVERIFY(OrbitI18n::load(orbit,language));
        if(language!="en_US"){
            QVERIFY(base.load(":/orbit/i18n/mmc_"+language+".qm"));QVERIFY(qApp->installTranslator(&base));
            QVERIFY(qt.load(":/orbit/i18n/qtbase_"+language+".qm"));QVERIFY(qApp->installTranslator(&qt));
        }
        QVERIFY(qApp->installTranslator(&orbit));
        const auto direction=QLocale(language).textDirection();qApp->setLayoutDirection(direction);
        QLocale::setDefault(QLocale(language=="zh"?QString("pt_BR"):language));
        // Regional formatting can remain Portuguese while the interface is Chinese.
        qApp->setPalette(AuroraStyle::preset(0).palette());
        qApp->setStyleSheet(AuroraStyle::preset(0).qss());
        QApplication::setStyle("Fusion");
        QVERIFY(!OrbitI18n::tr("Início").isEmpty());
        if(language!="pt_BR")QVERIFY(OrbitI18n::tr("Sua biblioteca")!=QString::fromUtf8("Sua biblioteca"));
        const auto instruction=OrbitI18n::tr("Código: %1\n\nAbra o navegador e confirme este código no site Ely.by. Depois autorize o Orbit.");
        QVERIFY(instruction.contains("%1"));QCOMPARE(instruction.count('\n'),2);
        for(int count:{0,1,2,3,11,21,100}){
            auto text=QCoreApplication::translate("Orbit","%n instância(s)",nullptr,count);
            QVERIFY(!text.contains("%n"));QVERIFY(!text.contains("(s)"));
        }
#ifdef Q_OS_WIN
        if(language=="zh"){
            const auto raw=QRawFont::fromFont(QFont(OrbitI18n::chineseFontFamily()));
            QVERIFY(raw.supportsCharacter(uint(0x4f60)));QVERIFY(raw.supportsCharacter(uint(0x9875)));
        }
#endif
        QAction add,play,stop,edit,settings,accounts;QMenu advanced;
        OrbitActions actions;actions.add=&add;actions.play=&play;actions.stop=&stop;actions.edit=&edit;
        actions.settings=&settings;actions.accounts=&accounts;actions.advanced=&advanced;actions.language=language;
        QString saved;actions.setLanguage=[&](const QString& key){saved=key;};
        OrbitShell shell(new QListWidget,actions);shell.resize(1220,840);shell.show();
        QCOMPARE(shell.layoutDirection(),direction);
        auto* combo=shell.findChild<QComboBox*>("orbitLanguage");QVERIFY(combo);QCOMPARE(combo->count(),6);
        QCOMPARE(combo->currentData().toString(),language);
        combo->setCurrentIndex((combo->currentIndex()+1)%6);QCOMPARE(saved,combo->currentData().toString());
        combo->setCurrentIndex(combo->findData(language));
        const auto folder=qEnvironmentVariable("ORBIT_I18N_GALLERY");
        if(!folder.isEmpty()){
            QDir().mkpath(folder);
            auto* pages=shell.findChild<QStackedWidget*>();QVERIFY(pages);
            for(int page:{0,3,4}){
                pages->setCurrentIndex(page);QTest::qWait(60);
                QVERIFY(shell.grab().save(folder+"/"+language+"-"+QString::number(page)+".png"));
            }
            QTemporaryDir temp;AccountList list;
            OrbitProfilesDialog profiles(&list,temp.filePath("skins"));profiles.show();QTest::qWait(40);
            QVERIFY(profiles.grab().save(folder+"/"+language+"-profiles.png"));
            AuroraThemeDialog theme(QDir(temp.path()));theme.show();QTest::qWait(40);
            QVERIFY(theme.grab().save(folder+"/"+language+"-editor.png"));
        }
        qApp->removeTranslator(&orbit);qApp->removeTranslator(&base);qApp->removeTranslator(&qt);
        qApp->setLayoutDirection(Qt::LeftToRight);
    }
};
QTEST_MAIN(OrbitI18nTest)
#include "OrbitI18n_test.moc"
