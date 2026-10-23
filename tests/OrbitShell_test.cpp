// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include <QListWidget>
#include <QDir>
#include "ui/widgets/PageContainer.h"
#include "ui_LogPage.h"
#include "ui/instanceview/InstanceDelegate.h"
#include "ui/themes/AuroraTheme.h"
#include "ui/widgets/OrbitShell.h"

class OrbitPreviewPage : public QWidget, public BasePage {
    QString m_id, m_name;
public:
    OrbitPreviewPage(QString id, QString name) : m_id(id), m_name(name) {
        if (id == "log") {
            Ui::LogPage ui;
            ui.setupUi(this);
            ui.gridLayout->setVerticalSpacing(12);
            ui.text->setPlaceholderText("Inicie o Minecraft para acompanhar os registros aqui.");
            ui.text->setStyleSheet("QPlainTextEdit { padding: 10px; }");
            ui.trackLogCheckbox->setText("Manter atualizado");
            ui.wrapCheckbox->setText("Quebra de linha");
            ui.colorCheckbox->setText("Linhas de cor");
            ui.btnCopy->setText("Copiar"); ui.btnPaste->setText("Enviar"); ui.btnClear->setText("Apagar");
            ui.searchBar->setPlaceholderText("Pesquisar nos registros…");
            ui.findButton->setText("Encontrar"); ui.btnBottom->setText("Final");
        }
    }
    QString id() const override { return m_id; }
    QString displayName() const override { return m_name; }
    QIcon icon() const override { return {}; }
};

class OrbitShellTest : public QObject {
    Q_OBJECT
private slots:
    void renderInstanceConsole() {
        const auto output=qEnvironmentVariable("ORBIT_GALLERY_DIR");
        if(output.isEmpty())QSKIP("Optional local UI gallery");
        QApplication::setPalette(AuroraStyle::preset(0).palette());
        qApp->setStyleSheet(AuroraStyle::preset(0).qss());
        QApplication::setStyle("Fusion");
        GenericPageProvider provider("Orbit");
        const QList<QPair<QString,QString>> entries{
            {"log","Registro do Minecraft"},{"version","Versão"},{"mods","Mods"},
            {"resourcepacks","Pacotes de Recursos"},{"shaderpacks","Pacotes de Shader"},
            {"notes","Notas"},{"worlds","Mundos"},{"servers","Servidores"},
            {"screenshots","Capturas de tela"},{"settings","Configurações"},{"otherlogs","Outros Registros"}};
        for(const auto& entry:entries)provider.addPageCreator([entry]{return new OrbitPreviewPage(entry.first,entry.second);});
        PageContainer container(&provider,"log");
        container.useOrbitStyle("26.1.2 · Fabric");
        auto* footer=new QHBoxLayout;
        for(const auto& title:{"Ajuda","Iniciar","Encerrar","Fechar"}){
            auto* button=new QPushButton(QString::fromUtf8(title));
            if(QString::fromUtf8(title)=="Iniciar")button->setObjectName("orbitInstanceLaunch");
            if(QString::fromUtf8(title)=="Encerrar")button->setEnabled(false);
            footer->addWidget(button);
            if(QString::fromUtf8(title)=="Ajuda")footer->addStretch();
        }
        container.addButtons(footer);container.resize(1100,780);container.show();
        QTest::qWait(100);
        QVERIFY(container.grab().save(output+"/console.png"));
    }
    void renderReorganizedPages() {
        Q_INIT_RESOURCE(AuroraLauncher);
        const auto output=qEnvironmentVariable("ORBIT_GALLERY_DIR");
        if(output.isEmpty())QSKIP("Optional local UI gallery");
        QDir().mkpath(output);
        QApplication::setPalette(AuroraStyle::preset(0).palette());
        qApp->setStyleSheet(AuroraStyle::preset(0).qss());
        QApplication::setStyle("Fusion");
        QAction add,play,stop,edit,settings,accounts;QMenu advanced;
        stop.setEnabled(false);
        OrbitActions actions;actions.add=&add;actions.play=&play;actions.stop=&stop;
        actions.edit=&edit;actions.settings=&settings;actions.accounts=&accounts;actions.advanced=&advanced;
        auto* library=new QListWidget;
        library->setViewMode(QListView::IconMode);library->setResizeMode(QListView::Adjust);
        library->setGridSize(QSize(252,176));library->setSpacing(8);
        library->setItemDelegate(new ListViewDelegate(library));
        for(const auto& name:{QString("26.1.2"),QString("Survival com amigos"),QString("Create · Exploração")}){
            auto* item=new QListWidgetItem(QApplication::style()->standardIcon(QStyle::SP_DirHomeIcon),name,library);
            item->setToolTip(name);
        }
        library->setCurrentRow(0);
        OrbitShell shell(library,actions);shell.resize(1220,780);
        shell.setSelection("26.1.2","Pronto para jogar",true);shell.setAccount("Jogador");shell.setInstanceCount(3);shell.show();
        QTest::qWait(100);
        for(const auto& name:{QString("Instâncias"),QString("Conteúdo"),QString("Temas")}){
            QPushButton* target=nullptr;
            for(auto* button:shell.findChildren<QPushButton*>())if(button->text()==name)target=button;
            QVERIFY(target);target->setFocus();target->click();QTest::qWait(50);
            const auto filename=name=="Instâncias"?"instances.png":name=="Conteúdo"?"content.png":"themes.png";
            QVERIFY(shell.grab().save(output+"/"+filename));
        }
    }

    void routesActionsAndRespectsSelection() {
        QAction add, play, stop, edit, settings, accounts;
        QMenu advanced;
        OrbitActions actions;
        actions.add = &add; actions.play = &play; actions.stop = &stop;
        actions.edit = &edit; actions.settings = &settings; actions.accounts = &accounts;
        actions.advanced = &advanced;
        QString page, query;
        actions.openContent = [&](const QString& value) { page = value; };
        actions.filter = [&](const QString& value) { query = value; };
        OrbitShell shell(new QListWidget, actions);
        auto* pages = shell.findChild<QStackedWidget*>();
        QVERIFY(pages);
        QCOMPARE(pages->currentIndex(), 0);
        auto* enter = shell.findChild<QPushButton*>("orbitPlay");
        QVERIFY(enter);
        enter->click();
        QCOMPARE(pages->currentIndex(), 1);
        auto* playButton = shell.findChild<QToolButton*>("orbitPlay");
        QVERIFY(playButton);
        QSignalSpy launches(&play, &QAction::triggered);
        play.setEnabled(false);
        QVERIFY(!playButton->isEnabled());
        play.setEnabled(true);
        playButton->click();
        QCOMPARE(launches.size(), 1);
        QPushButton* mods = nullptr;
        QPushButton* navigation = nullptr;
        for (auto* button : shell.findChildren<QPushButton*>()) {
            if (button->text().startsWith("Mods")) mods = button;
            if (button->text() == "Conteúdo") navigation = button;
        }
        QVERIFY(mods && navigation);
        navigation->click();
        QVERIFY(!mods->isEnabled());
        shell.setSelection("Fabric", "Minecraft", true);
        QVERIFY(mods->isEnabled());
        mods->click();
        QCOMPARE(page, QString("mods"));
        shell.setSelection({}, {}, false);
        QVERIFY(!mods->isEnabled());
        auto* search = shell.findChild<QLineEdit*>("orbitSearch");
        QVERIFY(search);
        search->setText("Fabric");
        QCOMPARE(query, QString("Fabric"));
        shell.setAccount("OrbitTest");
        shell.setInstanceCount(2);
    }
};
QTEST_MAIN(OrbitShellTest)
#include "OrbitShell_test.moc"

