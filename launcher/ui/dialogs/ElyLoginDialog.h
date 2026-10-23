// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "translations/OrbitI18n.h"
#include <QDialog>
#include <QClipboard>
#include <QGuiApplication>
#include <QUrlQuery>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include "DesktopServices.h"
#include "minecraft/auth/MinecraftAccount.h"
#include "minecraft/auth/AuthFlow.h"
class ElyLoginDialog : public QDialog {
    MinecraftAccountPtr m_account;
    shared_qobject_ptr<AuthFlow> m_task;
    QLabel* m_status;
    QPushButton* m_open;
    QUrl m_url;
    QPushButton* m_copy;
    QString m_code;
public:
    explicit ElyLoginDialog(QWidget* parent=nullptr):QDialog(parent),m_account(makeShared<MinecraftAccount>()) {
        m_account->accountData()->type=AccountType::Ely;
        setWindowTitle(OrbitI18n::tr("Entrar com Ely.by · Orbit"));setMinimumSize(500,420);
        auto* layout=new QVBoxLayout(this);layout->setContentsMargins(24,24,24,24);layout->setSpacing(16);
        auto* intro=new QLabel(OrbitI18n::tr("Entre no site oficial do Ely.by pelo navegador. Sua senha não passa pelo Orbit."),this);intro->setWordWrap(true);layout->addWidget(intro);
        m_status=new QLabel(OrbitI18n::tr("Preparando autorização…"),this);m_status->setWordWrap(true);m_status->setTextFormat(Qt::PlainText);m_status->setMinimumHeight(120);m_status->setAlignment(Qt::AlignTop|Qt::AlignLeft);layout->addWidget(m_status);
        m_open=new QPushButton(OrbitI18n::tr("Abrir login no navegador"),this);m_open->setEnabled(false);layout->addWidget(m_open);
        m_copy=new QPushButton(OrbitI18n::tr("Copiar código de autorização"),this);m_copy->setEnabled(false);layout->addWidget(m_copy);
        auto* cancel=new QPushButton(OrbitI18n::tr("Cancelar"),this);layout->addWidget(cancel);
        m_task=m_account->login();
        connect(m_task.get(),&AuthFlow::authorizeWithBrowser,this,[this](const QUrl& url){
            if(url.scheme()!="https" || url.host()!="account.ely.by")return;
            m_url=url;m_open->setEnabled(true);m_code=QUrlQuery(url).queryItemValue("user_code");m_copy->setEnabled(!m_code.isEmpty());
            m_status->setText(m_code.isEmpty()?OrbitI18n::tr("Clique abaixo, autorize o Orbit e volte a esta janela."):OrbitI18n::tr("Código: %1\n\nAbra o navegador e confirme este código no site Ely.by. Depois autorize o Orbit.").arg(m_code));
        });
        connect(m_task.get(),&Task::status,this,[this](const QString& message){m_status->setText(message);});
        connect(m_copy,&QPushButton::clicked,this,[this]{if(!m_code.isEmpty())QGuiApplication::clipboard()->setText(m_code);});
        connect(m_open,&QPushButton::clicked,this,[this]{if(m_url.isValid())DesktopServices::openUrl(m_url);});
        connect(m_task.get(),&Task::succeeded,this,&QDialog::accept);
        connect(m_task.get(),&Task::failed,this,[this](const QString& error){m_status->setText(error);m_open->setEnabled(false);m_copy->setEnabled(false);m_code.clear();});
        connect(cancel,&QPushButton::clicked,this,&QDialog::reject);
        QTimer::singleShot(0,this,[this]{m_task->start();});
    }
    ~ElyLoginDialog() override {if(m_task && m_task->isRunning())m_task->abort();}
    MinecraftAccountPtr account() const {return m_account;}
};
