// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "translations/OrbitI18n.h"

#include <QAction>
#include <QButtonGroup>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <functional>

#include "OrbitBackground.h"
#include "OrbitContentTile.h"
#include <QCheckBox>
#include <QComboBox>
#include <QMessageBox>

class OrbitNameLabel : public QLabel {
    QString m_name;
    void fit() { setText(fontMetrics().elidedText(m_name, Qt::ElideRight, qMax(20, width()))); }
protected:
    void resizeEvent(QResizeEvent* event) override { QLabel::resizeEvent(event); fit(); }
public:
    OrbitNameLabel() {
        setObjectName("orbitInstanceName");
        setWordWrap(false);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        setFixedHeight(48);
    }
    void setName(const QString& name) { m_name = name; setToolTip(name); fit(); }
};

struct OrbitActions {
    QString language="pt_BR";
    std::function<void(const QString&)> setLanguage;
    QAction* add = nullptr;
    QAction* play = nullptr;
    QAction* stop = nullptr;
    QAction* edit = nullptr;
    QAction* settings = nullptr;
    QAction* accounts = nullptr;
    QMenu* advanced = nullptr;
    std::function<void(const QString&)> openContent;
    std::function<void()> openThemes;
    std::function<void(int)> applyPreset;
    std::function<void(const QString&)> filter;
    std::function<void(bool)> homeVisible;
};

class OrbitShell : public QWidget {
    OrbitActions m_actions;
    bool m_ecoSleeping=false, m_minimized=false;
    QStackedWidget* m_pages;
    OrbitScene* m_scene;
    QFrame* m_header;
    OrbitNameLabel* m_instance;
    QLabel* m_description;
    QLabel* m_account;
    QLabel* m_count;
    QLabel* m_contentSelection;
    QLabel* m_backgroundStatus;
    QButtonGroup* m_navigation;
    QList<QWidget*> m_instanceTools;

    static QLabel* label(const QString& text, const char* name, QWidget* parent = nullptr) {
        auto* result = new QLabel(text, parent);
        result->setObjectName(name);
        result->setWordWrap(true);
        result->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        result->setTextFormat(Qt::PlainText);
        return result;
    }
    QToolButton* actionButton(QAction* action, const QString& text, const QString& name = {}) {
        auto* button = new QToolButton;
        button->setDefaultAction(action);
        // Keep enabled/icon/text state synchronized with the original QAction.
        auto sync = [button, action, text] {
            button->setText(text);
            button->setEnabled(action->isEnabled());
            button->setToolTip(action->toolTip());
        };
        connect(action, &QAction::changed, button, sync);
        sync();
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setObjectName(name);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        button->setMinimumHeight(42);
        return button;
    }
    QPushButton* button(const QString& text, std::function<void()> callback) {
        auto* result = new QPushButton(text);
        result->setMinimumHeight(44);
        result->setFocusPolicy(Qt::StrongFocus);
        result->setCursor(Qt::PointingHandCursor);
        connect(result, &QPushButton::clicked, this, std::move(callback));
        return result;
    }
    QWidget* page(const QString& title, const QString& subtitle, QVBoxLayout*& layout) {
        auto* result = new QFrame;
        result->setObjectName("orbitPanel");
        layout = new QVBoxLayout(result);
        layout->setContentsMargins(28, 24, 28, 24);
        layout->setSpacing(12);
        layout->addWidget(label(title, "orbitPageTitle"));
        layout->addWidget(label(subtitle, "orbitMuted"));
        return result;
    }
    void showPage(int index) {
        m_pages->setCurrentIndex(index);
        m_header->setVisible(true);
        if (m_actions.homeVisible) m_actions.homeVisible(true);
        if (auto* nav = m_navigation->button(index)) nav->setChecked(true);
    }
public:
    OrbitShell(QWidget* library, OrbitActions actions, QWidget* parent = nullptr) : QWidget(parent), m_actions(std::move(actions)) {
        setObjectName("orbitShell");
        setStyleSheet(R"(
            #orbitShell { background: palette(window); color: palette(window-text); }
            #orbitHeroCopy { color: #d2dcd9; font-size: 13px; }
            #orbitPageTitle { font-size: 26px; }
            #orbitMuted { color: #b8c3ba; font-size: 13px; }
            #orbitSearch { padding: 10px 14px; border-radius: 5px; }
            #orbitHeader, #orbitPages { background: transparent; border: 0; }
            #orbitHeader QLabel, #orbitPages QLabel { background: transparent; }
            #orbitSeal { color: #b8d8fc; font-size: 46px; font-weight: 300; }
            #orbitWordmark { color: #faf4e8; font-size: 14px; font-weight: 600; }
            #orbitProfile { color: #e3e7e2; font-size: 12px; }
            #orbitHeader #orbitTab { background: transparent; color: #c5cecb; border: 0; border-bottom: 2px solid transparent; border-radius: 0; padding: 9px 11px; }
            #orbitHeader #orbitTab:checked { color: #e0edff; border-bottom: 2px solid #aacdf4; }
            #orbitHeader #orbitTab:hover { color: white; background: rgba(255,255,255,14); }
            #orbitHeader #orbitTab:focus { border: 1px solid #aacdf4; }
            #orbitScene #orbitQuiet { background: transparent; color: #e8ede9; border: 1px solid rgba(220,230,222,45); border-radius: 5px; padding: 7px 12px; }
            #orbitScene #orbitQuiet:hover { background: rgba(255,255,255,24); }
            #orbitScene #orbitQuiet:disabled { color: #818b87; border-color: #4c5652; }
            #orbitScene #orbitTextButton { background: transparent; border: 0; color: #c7ddf7; text-align: left; padding: 0; }
            #orbitScene #orbitPlay { background: #aacdf4; color: #132137; border: 1px solid #d7e8ff; border-radius: 4px; min-height: 58px; font-size: 16px; font-weight: 600; letter-spacing: 2px; }
            #orbitScene #orbitPlay:hover { background: #d0e5ff; }
            #orbitScene #orbitPlay:pressed { background: #84addc; }
            #orbitScene #orbitPlay:disabled { background: #484b45; color: #8e958e; border-color: #686e65; }
            #orbitScene #orbitPlay:focus { border: 2px solid white; }
            #orbitWelcomeTitle { color: #faf8f3; font-size: 34px; font-weight: 400; }
            #orbitEyebrow { color: #bdd6f5; font-size: 10px; letter-spacing: 3px; }
            #orbitSmall { color: #cad0c8; font-size: 9px; letter-spacing: 1px; }
            #orbitInstanceName { color: #faf8f3; font-size: 28px; font-weight: 400; }
            #orbitRule { background: rgba(230,237,227,60); border: 0; }
            #orbitPanel { background: rgba(12,19,24,155); border: 1px solid rgba(220,230,222,35); border-radius: 10px; }
            #orbitPanel #orbitPageTitle { color: #f6f4ee; font-size: 26px; font-weight: 400; }
            #orbitPanel #orbitMuted { color: #b8c3ba; }
            #orbitContentButton { text-align: left; padding: 20px; min-height: 60px; }
            #orbitPanel { background: rgba(9,16,21,148); border: 1px solid rgba(220,230,222,32); border-radius: 14px; }
            #orbitPanel #orbitPageTitle { color: #f0eee7; font-size: 25px; font-weight: 500; }
            #orbitPanel #orbitMuted { color: #aab7b3; font-size: 12px; }
            #orbitPanel #orbitSelection { color: #bdd5f3; font-size: 12px; padding: 8px 0; }
            #orbitPanel QToolButton, #orbitPanel QPushButton { padding: 8px 16px; border-radius: 7px; }
            #orbitPanel #orbitPrimary { background: #aacdf4; color: #132137; font-weight: 600; border: 0; }
            #orbitPanel #orbitPreset { min-height: 46px; text-align: center; border-bottom: 3px solid #aacdf4; }
            #orbitPanel QCheckBox { color: #ecefe7; padding: 9px 0; }
        )");
        if(layoutDirection()==Qt::RightToLeft)setStyleSheet(styleSheet().replace("text-align: left","text-align: right"));
        auto* outer = new QVBoxLayout(this);
        outer->setContentsMargins(0, 0, 0, 0);
        m_scene = new OrbitScene;
        m_scene->setObjectName("orbitScene");
        outer->addWidget(m_scene);
        auto* root = new QVBoxLayout(m_scene);
        root->setContentsMargins(36, 26, 36, 26);
        root->setSpacing(22);

        m_header = new QFrame;
        m_header->setObjectName("orbitHeader");
        auto* top = new QHBoxLayout(m_header);
        top->setContentsMargins(0, 0, 0, 0);
        top->setSpacing(8);
        auto* seal = label("◎", "orbitSeal");
        seal->setFixedSize(52, 52);
        seal->setAlignment(Qt::AlignCenter);
        top->addWidget(seal);
        top->addWidget(label("O R B I T", "orbitWordmark"));
        top->addSpacing(24);
        m_navigation = new QButtonGroup(this);
        const QStringList sections {OrbitI18n::tr("Início"), OrbitI18n::tr("Jogar"), OrbitI18n::tr("Instâncias"), OrbitI18n::tr("Conteúdo"), OrbitI18n::tr("Temas")};
        for (int i = 0; i < sections.size(); ++i) {
            auto* item = button(sections[i], [this, i] { showPage(i); });
            item->setCheckable(true);
            item->setObjectName("orbitTab");
            m_navigation->addButton(item, i);
            top->addWidget(item);
        }
        top->addStretch();
        m_account = label(OrbitI18n::tr("Seu perfil"), "orbitProfile");
        m_account->setMaximumWidth(130);
        m_account->setToolTip(m_account->text());
        top->addWidget(m_account);
        auto* profile = actionButton(m_actions.accounts, OrbitI18n::tr("Perfil"));
        profile->setObjectName("orbitQuiet");
        profile->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Fixed);
        profile->setMinimumWidth(72);profile->setMaximumWidth(220);
        top->addWidget(profile);
        auto* settings = actionButton(m_actions.settings, "⚙");
        settings->setAccessibleName(OrbitI18n::tr("Configurações"));
        settings->setFixedWidth(44);
        settings->setObjectName("orbitQuiet");
        top->addWidget(settings);
        auto* advanced = new QToolButton;
        advanced->setText("···");
        advanced->setAccessibleName(OrbitI18n::tr("Todas as funções"));
        advanced->setToolTip(OrbitI18n::tr("Todas as funções"));
        advanced->setObjectName("orbitQuiet");
        advanced->setMinimumSize(44, 44);
        advanced->setMenu(m_actions.advanced);
        advanced->setPopupMode(QToolButton::InstantPopup);
        top->addWidget(advanced);
        root->addWidget(m_header);
        m_pages = new QStackedWidget;
        m_pages->setObjectName("orbitPages");
        root->addWidget(m_pages, 1);

        auto* landing = new QWidget;
        auto* welcome = new QVBoxLayout(landing);
        welcome->setContentsMargins(12, 0, 12, 16);
        welcome->addStretch();
        auto* welcomeRow = new QHBoxLayout;
        auto* welcomeCopy = new QVBoxLayout;
        welcomeCopy->setSpacing(8);
        welcomeCopy->setAlignment(Qt::AlignBottom);
        welcomeCopy->addWidget(label(OrbitI18n::tr("BEM-VINDO AO ORBIT"), "orbitEyebrow"));
        welcomeCopy->addSpacing(8);
        welcomeCopy->addWidget(label(OrbitI18n::tr("Um novo mundo.\nDo seu jeito."), "orbitWelcomeTitle"));
        welcomeCopy->addSpacing(8);
        welcomeCopy->addWidget(label(OrbitI18n::tr("Seus mundos e suas próximas aventuras, juntos."), "orbitHeroCopy"));
        welcomeRow->addLayout(welcomeCopy, 1);
        auto* welcomeAction = new QVBoxLayout;
        welcomeAction->addStretch();
        auto* enter = button(OrbitI18n::tr("VAMOS JOGAR  →"), [this] { showPage(1); });
        enter->setObjectName("orbitPlay");
        enter->setFixedWidth(238);
        welcomeAction->addWidget(enter);
        auto* below = label(OrbitI18n::tr("ESCOLHA UMA INSTÂNCIA E COMECE"), "orbitSmall");
        below->setAlignment(Qt::AlignCenter);
        welcomeAction->addWidget(below);
        welcomeRow->addLayout(welcomeAction);
        welcome->addLayout(welcomeRow);
        m_pages->addWidget(landing);

        auto* playing = new QWidget;
        auto* home = new QVBoxLayout(playing);
        home->setContentsMargins(12, 0, 12, 16);
        home->addStretch();
        auto* rule = new QFrame;
        rule->setObjectName("orbitRule");
        rule->setFixedHeight(1);
        home->addWidget(rule);
        home->addSpacing(22);
        auto* launchRow = new QHBoxLayout;
        launchRow->setSpacing(24);
        auto* identity = new QVBoxLayout;
        identity->addWidget(label(OrbitI18n::tr("SUA PRÓXIMA PARTIDA"), "orbitEyebrow"));
        m_instance = new OrbitNameLabel;
        m_instance->setName(OrbitI18n::tr("Escolha uma instância"));
        identity->addWidget(m_instance);
        m_description = label(OrbitI18n::tr("Adicione ou importe na aba Instâncias."), "orbitHeroCopy");
        identity->addWidget(m_description);
        auto* choose = button(OrbitI18n::tr("Trocar instância  ↗"), [this] { showPage(2); });
        choose->setObjectName("orbitTextButton");
        choose->setMaximumWidth(200);
        identity->addWidget(choose);
        launchRow->addLayout(identity, 1);
        auto* manage = actionButton(m_actions.edit, OrbitI18n::tr("Gerenciar"));
        manage->setObjectName("orbitQuiet");
        manage->setMinimumWidth(104);
        launchRow->addWidget(manage, 0, Qt::AlignBottom);
        auto* stop = actionButton(m_actions.stop, OrbitI18n::tr("Parar"));
        stop->setObjectName("orbitQuiet");
        stop->setMinimumWidth(68);
        launchRow->addWidget(stop, 0, Qt::AlignBottom);
        auto* play = actionButton(m_actions.play, OrbitI18n::tr("JOGAR"), "orbitPlay");
        play->setFixedWidth(238);
        launchRow->addWidget(play, 0, Qt::AlignBottom);
        home->addLayout(launchRow);
        m_pages->addWidget(playing);

        QVBoxLayout* libraryLayout;
        auto* libraryPage = page(OrbitI18n::tr("Sua biblioteca"), OrbitI18n::tr("Cada instância tem seus próprios mundos, mods e configurações."), libraryLayout);
        auto* searchRow = new QHBoxLayout;
        auto* search = new QLineEdit;
        search->setObjectName("orbitSearch");
        search->setPlaceholderText(OrbitI18n::tr("Buscar instâncias…"));
        search->setAccessibleName(OrbitI18n::tr("Buscar instâncias"));
        connect(search, &QLineEdit::textChanged, this, [this](const QString& text) { if (m_actions.filter) m_actions.filter(text); });
        searchRow->addWidget(search, 1);
        searchRow->addWidget(actionButton(m_actions.add, OrbitI18n::tr("+ Adicionar")));
        libraryLayout->addLayout(searchRow);
        m_count = label("", "orbitMuted");
        libraryLayout->addWidget(m_count);
        library->setObjectName("orbitLibrary");
        library->setStyleSheet("background: transparent; border: 0;");
        libraryLayout->addWidget(library, 1);
        auto* libraryActions = new QHBoxLayout;
        libraryActions->addWidget(actionButton(m_actions.edit, OrbitI18n::tr("Gerenciar seleção")));
        libraryActions->addStretch();
        libraryActions->addWidget(actionButton(m_actions.play, OrbitI18n::tr("Jogar seleção"), "orbitPrimary"));
        libraryLayout->addLayout(libraryActions);
        m_pages->addWidget(libraryPage);

        QVBoxLayout* contentLayout;
        auto* contentPage = page(OrbitI18n::tr("Tudo para o seu mundo"), OrbitI18n::tr("O conteúdo é gerenciado na instância selecionada. Escolha uma na biblioteca."), contentLayout);
        auto* selectionRow = new QHBoxLayout;
        m_contentSelection = label(OrbitI18n::tr("Selecione uma instância para gerenciar"), "orbitSelection");
        selectionRow->addWidget(m_contentSelection, 1);
        selectionRow->addWidget(button(OrbitI18n::tr("Escolher instância"), [this] { showPage(2); }));
        contentLayout->addLayout(selectionRow);
        struct ContentEntry { QString title, detail, symbol, id; };
        const QList<ContentEntry> content {
            {OrbitI18n::tr("Mods"), OrbitI18n::tr("Instale, atualize e organize seus mods."), "⊞", "mods"},
            {OrbitI18n::tr("Texturas"), OrbitI18n::tr("Pacotes de recursos e novos detalhes."), "▧", "resourcepacks"},
            {OrbitI18n::tr("Shaders"), OrbitI18n::tr("Luz, sombras e atmosfera."), "☼", "shaderpacks"},
            {OrbitI18n::tr("Mundos"), OrbitI18n::tr("Seus saves e próximas aventuras."), "◎", "worlds"},
            {OrbitI18n::tr("Componentes"), OrbitI18n::tr("Minecraft, Fabric, Forge e versões."), "◇", "version"},
            {OrbitI18n::tr("Console"), OrbitI18n::tr("Logs, execução e diagnóstico."), "›_", "console"}
        };
        auto* contentGrid = new QGridLayout;
        contentGrid->setSpacing(12);
        int contentIndex = 0;
        for (const auto& entry : content) {
            auto* item = new OrbitContentTile(entry.title, entry.detail, entry.symbol);
            connect(item, &QPushButton::clicked, this, [this, id=entry.id] { if (m_actions.openContent) m_actions.openContent(id); });
            contentGrid->addWidget(item, contentIndex / 3, contentIndex % 3);
            ++contentIndex;
            m_instanceTools.append(item);
        }
        contentLayout->addLayout(contentGrid);
        contentLayout->addStretch();
        m_pages->addWidget(contentPage);

        QVBoxLayout* appearanceLayout;
        auto* appearance = page(OrbitI18n::tr("Seu Orbit. Sua atmosfera."), OrbitI18n::tr("Escolha um estilo, personalize o cenário e ajuste o comportamento durante o jogo."), appearanceLayout);
        auto* presets = new QHBoxLayout;
        int presetIndex = 0;
        for (const auto& name : {OrbitI18n::tr("Orbit Dark"),OrbitI18n::tr("Orbit Light"),OrbitI18n::tr("Orbit Forest")}) {
            const int index = presetIndex++;
            auto* preset = button(name, [this,index] { if (m_actions.applyPreset) m_actions.applyPreset(index); });
            preset->setObjectName("orbitPreset");
            const QStringList fills{"#202936","#eeeae0","#1b342d"};
            const QStringList inks{"#eff2ed","#202a31","#e0f2e7"};
            preset->setStyleSheet(QString("QPushButton { background: %1; color: %2; border: 1px solid #66736c; border-bottom: 3px solid #aacdf4; min-height: 56px; } QPushButton:hover { border: 2px solid #d2e4fb; }").arg(fills[index],inks[index]));

            presets->addWidget(preset);
        }
        appearanceLayout->addLayout(presets);
        auto* languageRow=new QHBoxLayout;
        auto* languageLabel=label(OrbitI18n::tr("Idioma da interface"),"orbitMuted");
        languageRow->addWidget(languageLabel);
        auto* language=new QComboBox;
        language->setObjectName("orbitLanguage");language->setMinimumWidth(190);
        const QList<QPair<QString,QString>> languages{{"English","en_US"},{"Português (Brasil)","pt_BR"},{"Русский","ru"},{"简体中文","zh"},{"Español","es"},{"العربية","ar"}};
        for(const auto& entry:languages)language->addItem(entry.first,entry.second);
        language->setCurrentIndex(language->findData(OrbitI18n::canonical(m_actions.language)));
        language->setAccessibleName(OrbitI18n::tr("Idioma da interface"));
        languageRow->addWidget(language);languageRow->addStretch();appearanceLayout->addLayout(languageRow);
        auto* restartHint=label(OrbitI18n::tr("Reinicie o Orbit para aplicar o idioma em todas as telas."),"orbitMuted");
        restartHint->setVisible(false);appearanceLayout->addWidget(restartHint);
        connect(language,&QComboBox::currentIndexChanged,this,[this,language,restartHint](int){
            if(m_actions.setLanguage)m_actions.setLanguage(language->currentData().toString());
            restartHint->show();
        });
        auto* themeOptions = new QHBoxLayout;
        themeOptions->addWidget(label(OrbitI18n::tr("Ajuste cores, fontes, cantos e espaçamento."), "orbitMuted"),1);
        themeOptions->addWidget(button(OrbitI18n::tr("Personalizar tema  ↗"), [this] { if(m_actions.openThemes)m_actions.openThemes(); }));
        appearanceLayout->addLayout(themeOptions);
        auto* mediaRow = new QHBoxLayout;
        auto* preview = new QLabel;
        preview->setPixmap(QPixmap(":/orbit/landscape.jpg").scaled(230,130,Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation));
        preview->setFixedSize(230,130); preview->setScaledContents(true);
        mediaRow->addWidget(preview);
        auto* mediaOptions = new QVBoxLayout;
        mediaOptions->addWidget(label(OrbitI18n::tr("FUNDO DO LAUNCHER"), "orbitEyebrow"));
        m_backgroundStatus = label(OrbitI18n::tr("Kingdom Hearts · Fundo padrão"), "orbitMuted");
        mediaOptions->addWidget(m_backgroundStatus);
        auto* mediaButtons = new QHBoxLayout;
        mediaButtons->addWidget(button(OrbitI18n::tr("Imagem…"), [this] {
            const auto file=QFileDialog::getOpenFileName(this,OrbitI18n::tr("Escolher imagem"),{},OrbitI18n::tr("Imagens (*.jpg *.jpeg *.png *.webp)"));
            if(file.isEmpty())return;
            if(!m_scene->setBackground(file)){QMessageBox::warning(this,OrbitI18n::tr("Imagem inválida"),OrbitI18n::tr("Não foi possível abrir essa imagem."));return;}
            QSettings().setValue("Orbit/Background",file);QSettings().remove("Orbit/Video");
            m_backgroundStatus->setText(QFileInfo(file).fileName()+OrbitI18n::tr(" · Imagem"));
        }));
        mediaButtons->addWidget(button(OrbitI18n::tr("Vídeo…"), [this] {
            const auto file=QFileDialog::getOpenFileName(this,OrbitI18n::tr("Escolher vídeo"),{},OrbitI18n::tr("Vídeos (*.mp4 *.m4v *.webm *.mov *.mkv)"));
            if(file.isEmpty())return;
            if(!m_scene->setVideo(file)){QMessageBox::warning(this,OrbitI18n::tr("Vídeo indisponível"),OrbitI18n::tr("O arquivo não pode ser lido."));return;}
            QSettings().setValue("Orbit/Video",file);
            m_backgroundStatus->setText(QFileInfo(file).fileName()+OrbitI18n::tr(" · Vídeo em loop, sem áudio"));
        }));
        mediaButtons->addWidget(button(OrbitI18n::tr("Restaurar"), [this] {
            m_scene->setBackground(":/orbit/landscape.jpg");QSettings().remove("Orbit/Background");QSettings().remove("Orbit/Video");
            m_backgroundStatus->setText(OrbitI18n::tr("Kingdom Hearts · Fundo padrão"));
        }));
        mediaOptions->addLayout(mediaButtons);
        mediaOptions->addWidget(label(OrbitI18n::tr("Vídeos pausam e liberam o decodificador quando a janela fica oculta."), "orbitMuted"));
        mediaRow->addLayout(mediaOptions,1);appearanceLayout->addLayout(mediaRow);
        auto* eco = new QCheckBox(OrbitI18n::tr("Modo econômico durante o Minecraft"));
        eco->setObjectName("orbitEcoMode");eco->setChecked(QSettings().value("Orbit/EcoMode",true).toBool());
        eco->setAccessibleDescription(OrbitI18n::tr("Oculta a interface e interrompe o vídeo durante a execução do Minecraft."));
        connect(eco,&QCheckBox::toggled,this,[](bool enabled){QSettings().setValue("Orbit/EcoMode",enabled);});
        appearanceLayout->addWidget(eco);
        appearanceLayout->addWidget(label(OrbitI18n::tr("Oculta o Orbit quando o jogo inicia e reabre ao encerrar. O processo continua supervisionando o Minecraft."), "orbitMuted"));
        appearanceLayout->addStretch();
        auto* appearanceScroll=new QScrollArea;
        appearanceScroll->setWidgetResizable(true);appearanceScroll->setFrameShape(QFrame::NoFrame);
        appearanceScroll->setStyleSheet("QScrollArea, QScrollArea > QWidget > QWidget { background: transparent; }");
        appearanceScroll->setWidget(appearance);m_pages->addWidget(appearanceScroll);
        m_scene->onError=[this](const QString& error){m_backgroundStatus->setText(OrbitI18n::tr("Vídeo não suportado · Imagem mantida. ")+error);};
        const auto background=QSettings().value("Orbit/Background").toString();
        if(!background.isEmpty())m_scene->setBackground(background);
        const auto video=QSettings().value("Orbit/Video").toString();
        if(!video.isEmpty()){
            if(m_scene->setVideo(video))m_backgroundStatus->setText(QFileInfo(video).fileName()+OrbitI18n::tr(" · Vídeo em loop"));
            else m_backgroundStatus->setText(OrbitI18n::tr("Vídeo não encontrado · Escolha outro arquivo"));
        }
        setSelection({}, {}, false);
        showPage(0);
    }
    void setSelection(const QString& name, const QString& description, bool editable) {
        m_instance->setName(name.isEmpty() ? OrbitI18n::tr("Escolha uma instância") : name);
        m_contentSelection->setText(name.isEmpty() ? OrbitI18n::tr("Selecione uma instância para gerenciar") : OrbitI18n::tr("GERENCIANDO  ·  ")+name);
        m_description->setText(name.isEmpty() ? OrbitI18n::tr("Crie ou importe uma instância para começar.") : description);
        for (auto* tool : m_instanceTools) tool->setEnabled(editable);
    }
    void setMediaSuspended(bool value) { m_ecoSleeping=value; m_scene->setSuspended(m_ecoSleeping||m_minimized); }
    void setWindowMinimized(bool value) { m_minimized=value; m_scene->setSuspended(m_ecoSleeping||m_minimized); }
    void setAccount(const QString& name) { m_account->setText(name.isEmpty() ? OrbitI18n::tr("Seu perfil") : name); m_account->setToolTip(name); }
    void setInstanceCount(int count) { m_count->setText(QCoreApplication::translate("Orbit","%n instância(s)",nullptr,count)); }
};
