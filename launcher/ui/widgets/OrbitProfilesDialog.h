// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "translations/OrbitI18n.h"
#include <QDialog>
#include <QCheckBox>
#include <functional>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QSaveFile>
#include <QVBoxLayout>
#include "minecraft/auth/AccountList.h"
#include "ui/dialogs/ElyLoginDialog.h"

#include "minecraft/OrbitLocalSkins.h"

class OrbitSkinPreview : public QWidget {
    QImage m_skin;
    QString m_emptyText=OrbitI18n::tr("Sua skin\n\nImporte um PNG");
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(),QColor("#101b2b"));
        if(m_skin.isNull()) { p.setPen(QColor("#91a9c5"));p.drawText(rect().adjusted(16,16,-16,-16),Qt::AlignCenter|Qt::TextWordWrap,m_emptyText);return; }
        const int scale=qMax(1,qMin((width()-24)/16,(height()-24)/32));
        p.translate((width()-16*scale)/2,(height()-32*scale)/2);p.scale(scale,scale);
        p.setRenderHint(QPainter::SmoothPixmapTransform,false);
        p.drawImage(QRect(4,0,8,8),m_skin,QRect(8,8,8,8));
        p.drawImage(QRect(4,0,8,8),m_skin,QRect(40,8,8,8));
        p.drawImage(QRect(4,8,8,12),m_skin,QRect(20,20,8,12));
        p.drawImage(QRect(0,8,4,12),m_skin,QRect(44,20,4,12));
        p.drawImage(QRect(4,20,4,12),m_skin,QRect(4,20,4,12));
        if(m_skin.height()==64) {
            p.drawImage(QRect(12,8,4,12),m_skin,QRect(36,52,4,12));
            p.drawImage(QRect(8,20,4,12),m_skin,QRect(20,52,4,12));
            p.drawImage(QRect(4,8,8,12),m_skin,QRect(20,36,8,12));
            p.drawImage(QRect(0,8,4,12),m_skin,QRect(44,36,4,12));
            p.drawImage(QRect(12,8,4,12),m_skin,QRect(52,52,4,12));
            p.drawImage(QRect(4,20,4,12),m_skin,QRect(4,36,4,12));
            p.drawImage(QRect(8,20,4,12),m_skin,QRect(4,52,4,12));
        } else {
            p.save();p.translate(16,0);p.scale(-1,1);
            p.drawImage(QRect(0,8,4,12),m_skin,QRect(44,20,4,12));
            p.drawImage(QRect(4,20,4,12),m_skin,QRect(4,20,4,12));p.restore();
        }
    }
public:
    explicit OrbitSkinPreview(QWidget* parent=nullptr):QWidget(parent){setMinimumSize(150,240);setAccessibleName(OrbitI18n::tr("Prévia frontal da skin"));}
    void setEmptyText(QString text){m_emptyText=std::move(text);update();}
    void setSkin(const QString& path){m_skin=QImage(path);update();}
};

class OrbitProfilesDialog : public QDialog {
    AccountList* m_accounts;
    OrbitLocalSkins m_skins;
    QListWidget* m_list;
    QLineEdit* m_name;
    QLabel* m_hint;
    QLabel* m_skinInfo;
    QPushButton* m_create;
    QPushButton* m_use;
    QPushButton* m_import;
    QPushButton* m_remove;
    QPushButton* m_prepare;
    QString m_gameRoot;
    OrbitSkinPreview* m_preview;
    MinecraftAccountPtr selected() const {
        auto* item=m_list->currentItem();
        if(!item)return MinecraftAccountPtr{};
        const int index=m_accounts->findAccountByProfileId(item->data(Qt::UserRole).toString());
        return index<0?MinecraftAccountPtr{}:m_accounts->at(index);
    }
    void refresh() {
        const auto current=selected(); const auto active=m_accounts->defaultAccount();
        const auto id=current?current->profileId():active?active->profileId():QString();
        m_list->clear();
        for(int i=0;i<m_accounts->count();++i){
            const auto account=m_accounts->at(i);
            auto* item=new QListWidgetItem(account->profileName()+"   ·   "+(account->accountType()==AccountType::Offline?OrbitI18n::tr("Local"):account->accountType()==AccountType::Ely?"Ely.by":"Microsoft")+(active==account?"   ✓":""),m_list);
            item->setData(Qt::UserRole,account->profileId());
            if(account->profileId()==id)m_list->setCurrentItem(item);
        }
        updateSelection();validate();
    }
    void updateSelection() {
        const auto account=selected();m_use->setEnabled(bool(account));
        const bool local=account && account->accountType()==AccountType::Offline;
        const bool ely=account && account->accountType()==AccountType::Ely;
        m_import->setEnabled(local || ely);m_import->setText(ely?OrbitI18n::tr("Skins Ely.by ↗"):OrbitI18n::tr("Importar skin PNG"));
        m_preview->setEmptyText(ely?OrbitI18n::tr("Skin Ely.by\n\nGerenciada no site"):OrbitI18n::tr("Sua skin\n\nImporte um PNG"));
        m_skinInfo->setText(ely?OrbitI18n::tr("Gerencie a skin no site Ely.by. O jogo usa o perfil autenticado."):OrbitI18n::tr("Skin local por perfil. Preparar requer CustomSkinLoader; Skins automáticas instala e sincroniza antes de jogar."));m_remove->setEnabled(local && QFile::exists(m_skins.path(account->profileId())));
        m_prepare->setEnabled(local && !m_gameRoot.isEmpty() && QFile::exists(m_skins.path(account->profileId())));
        m_preview->setSkin(account?m_skins.path(account->profileId()):QString());
    }
    void validate() {
        bool duplicate=false;
        for(const auto& name:m_accounts->profileNames())if(name.compare(m_name->text(),Qt::CaseInsensitive)==0)duplicate=true;
        m_create->setEnabled(m_name->hasAcceptableInput() && !duplicate);
        m_hint->setText(duplicate?OrbitI18n::tr("Esse apelido já existe. Selecione o perfil na lista."):OrbitI18n::tr("3 a 16 caracteres: letras, números e sublinhado."));
    }
    bool save() {
        if(m_accounts->saveList())return true;
        QMessageBox::warning(this,OrbitI18n::tr("Perfil não salvo"),OrbitI18n::tr("A alteração está na memória, mas não foi salva no disco. Verifique a permissão da pasta do launcher e tente novamente."));return false;
    }
public:
    explicit OrbitProfilesDialog(AccountList* accounts,QString skinRoot,QWidget* parent=nullptr, QString gameRoot={}, bool autoSkins=false, std::function<void(bool)> saveAutoSkins={})
        :QDialog(parent),m_accounts(accounts),m_skins(std::move(skinRoot)),m_gameRoot(std::move(gameRoot)) {
        setWindowTitle(OrbitI18n::tr("Perfis e skins · Orbit"));resize(780,610);setMinimumSize(710,570);
        setObjectName("orbitProfiles");
        setStyleSheet(R"(
            #orbitProfiles { background:#111c2b; color:#edf3fc; }
            #orbitProfiles QLabel { color:#c1d0e3; background:transparent; }
            #orbitProfiles #profileTitle { font-size:25px;color:#eef5ff; }
            #orbitProfiles QLineEdit, #orbitProfiles QListWidget { background:#0c1523;color:#e5eefb;border:1px solid #33475f;border-radius:8px;padding:9px;selection-background-color:#2d496a; }
            #orbitProfiles QListWidget::item { padding:12px 8px;border-radius:5px; }
            #orbitProfiles QListWidget::item:selected { background:#2d496a;color:#e2efff; }
            #orbitProfiles QPushButton { background:#203149;color:#e1edff;border:1px solid #415974;border-radius:7px;padding:9px 14px; }
            #orbitProfiles QPushButton:hover { background:#2c4462; }
            #orbitProfiles QPushButton:focus, #orbitProfiles QLineEdit:focus { border:1px solid #b7d6ff; }
            #orbitProfiles QPushButton:disabled { color:#647890;background:#182536;border-color:#2b3b51; }
            #orbitProfiles #createLocal { background:#b5d5fa;color:#13223a;font-weight:600; }
            #orbitProfiles #createLocal:disabled { background:#25374d;color:#778ca6; }
        )");
        auto* layout=new QVBoxLayout(this);layout->setContentsMargins(26,24,26,24);layout->setSpacing(14);
        auto* title=new QLabel(OrbitI18n::tr("Seu perfil. Seu estilo."),this);title->setObjectName("profileTitle");layout->addWidget(title);
        auto* info=new QLabel(OrbitI18n::tr("Perfis locais para single-player e servidores que aceitam jogadores offline."),this);info->setWordWrap(true);layout->addWidget(info);
        auto* body=new QHBoxLayout;layout->addLayout(body,1);
        auto* left=new QVBoxLayout;body->addLayout(left,1);
        m_list=new QListWidget(this);m_list->setAccessibleName(OrbitI18n::tr("Perfis salvos"));left->addWidget(m_list,1);
        m_use=new QPushButton(OrbitI18n::tr("Usar perfil selecionado"),this);m_use->setObjectName("useProfile");left->addWidget(m_use);
        auto* caption=new QLabel(OrbitI18n::tr("NOVO PERFIL LOCAL"),this);left->addWidget(caption);
        m_name=new QLineEdit(this);m_name->setObjectName("localNickname");m_name->setPlaceholderText(OrbitI18n::tr("Seu apelido no Minecraft"));m_name->setMaxLength(16);
        m_name->setValidator(new QRegularExpressionValidator(QRegularExpression("^[A-Za-z0-9_]{3,16}$"),m_name));left->addWidget(m_name);
        m_hint=new QLabel(this);m_hint->setWordWrap(true);left->addWidget(m_hint);
        m_create=new QPushButton(OrbitI18n::tr("Criar e usar perfil"),this);m_create->setObjectName("createLocal");left->addWidget(m_create);
        auto* right=new QVBoxLayout;body->addLayout(right);
        m_preview=new OrbitSkinPreview(this);right->addWidget(m_preview,1);
        m_import=new QPushButton(OrbitI18n::tr("Importar skin PNG"),this);m_remove=new QPushButton(OrbitI18n::tr("Remover skin"),this);right->addWidget(m_import);right->addWidget(m_remove);
        m_prepare=new QPushButton(OrbitI18n::tr("Preparar para a instância"),this);right->addWidget(m_prepare);
        m_skinInfo=new QLabel(OrbitI18n::tr("Skin salva por perfil.\nPreparar copia para a instância selecionada. Requer CustomSkinLoader compatível; visível só para você."),this);m_skinInfo->setWordWrap(true);m_skinInfo->setMaximumWidth(180);right->addWidget(m_skinInfo);
        auto* automatic=new QCheckBox(OrbitI18n::tr("Skins automáticas nesta instância"),this);
        automatic->setObjectName("automaticSkins");automatic->setChecked(autoSkins);automatic->setEnabled(!m_gameRoot.isEmpty() && bool(saveAutoSkins));
        automatic->setToolTip(OrbitI18n::tr("Antes de jogar, instala o CustomSkinLoader compatível e sincroniza a skin do perfil ativo. Vale para todos os mundos desta instância. Requer Fabric, Quilt, Forge ou NeoForge."));
        layout->addWidget(automatic);
        connect(automatic,&QCheckBox::toggled,this,[saveAutoSkins](bool enabled){if(saveAutoSkins)saveAutoSkins(enabled);});
        auto* elyLogin=new QPushButton(OrbitI18n::tr("Entrar com Ely.by"),this);
        connect(elyLogin,&QPushButton::clicked,this,[this]{ElyLoginDialog login(this);if(login.exec()==QDialog::Accepted){m_accounts->addAccount(login.account());m_accounts->setDefaultAccount(login.account());save();refresh();}});
        auto* footer=new QHBoxLayout;layout->addLayout(footer);footer->addWidget(elyLogin);
        auto* advanced=new QPushButton(OrbitI18n::tr("Microsoft / opções avançadas"),this);footer->addWidget(advanced);footer->addStretch();
        auto* close=new QPushButton(OrbitI18n::tr("Concluir"),this);footer->addWidget(close);
        connect(close,&QPushButton::clicked,this,&QDialog::accept);
        connect(advanced,&QPushButton::clicked,this,[this]{done(2);});
        connect(m_name,&QLineEdit::textChanged,this,[this]{validate();});
        connect(m_list,&QListWidget::currentRowChanged,this,[this]{updateSelection();});
        connect(m_use,&QPushButton::clicked,this,[this]{if(auto account=selected()){m_accounts->setDefaultAccount(account);save();refresh();}});
        connect(m_create,&QPushButton::clicked,this,[this]{
            validate();if(!m_create->isEnabled())return;
            auto account=MinecraftAccount::createOffline(m_name->text());if(!account)return;
            account->login()->start();m_accounts->addAccount(account);m_accounts->setDefaultAccount(account);save();m_name->clear();refresh();
            for(int i=0;i<m_list->count();++i)if(m_list->item(i)->data(Qt::UserRole).toString()==account->profileId())m_list->setCurrentRow(i);
        });
        connect(m_import,&QPushButton::clicked,this,[this]{
            auto account=selected();if(!account)return;
            if(account->accountType()==AccountType::Ely){DesktopServices::openUrl(QUrl("https://ely.by/skins"));return;}
            if(account->accountType()!=AccountType::Offline)return;
            const auto file=QFileDialog::getOpenFileName(this,OrbitI18n::tr("Escolher skin"),{},OrbitI18n::tr("Skin PNG (*.png)"));if(file.isEmpty())return;
            QString error;if(!m_skins.importSkin(account->profileId(),file,error))QMessageBox::warning(this,OrbitI18n::tr("Skin inválida"),error);updateSelection();
        });
        connect(m_prepare,&QPushButton::clicked,this,[this]{
            auto account=selected();if(!account || account->accountType()!=AccountType::Offline)return;
            QString error;
            if(!m_skins.prepareInstance(account->profileId(),account->profileName(),m_gameRoot,error))QMessageBox::warning(this,OrbitI18n::tr("Skin"),error);
            else QMessageBox::information(this,OrbitI18n::tr("Skin preparada"),OrbitI18n::tr("PNG copiada para a instância selecionada. Para aparecer no jogo, instale o CustomSkinLoader compatível com a versão do Minecraft e o carregador de mods. Depois reabra o mundo. A skin local é visível somente para você."));
        });
        connect(m_remove,&QPushButton::clicked,this,[this]{if(auto account=selected()){if(!m_skins.remove(account->profileId()))QMessageBox::warning(this,OrbitI18n::tr("Skin"),OrbitI18n::tr("Não foi possível remover a skin."));updateSelection();}});
        connect(m_accounts,&AccountList::listChanged,this,[this]{refresh();});
        connect(m_accounts,&AccountList::defaultAccountChanged,this,[this]{refresh();});
        refresh();
    }
};
