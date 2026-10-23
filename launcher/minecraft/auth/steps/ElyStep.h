// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "translations/OrbitI18n.h"
#include "minecraft/auth/AuthStep.h"
#include <QOAuth2AuthorizationCodeFlow>
#include <QOAuth2DeviceAuthorizationFlow>
#include <QOAuthHttpServerReplyHandler>
#include <QPointer>
#include <QTimer>
class ElyStep : public AuthStep {
    Q_OBJECT
    QOAuth2AuthorizationCodeFlow m_flow;
    QOAuth2DeviceAuthorizationFlow m_device;
    QAbstractOAuth2* m_active;
    bool m_deviceMode;
    QOAuthHttpServerReplyHandler* m_handler;
    QNetworkAccessManager* m_network;
    QPointer<QNetworkReply> m_reply;
    QTimer m_timeout;
    bool m_silent, m_done=false;
    int m_networkRetries=0;
    void finish(AccountTaskState state,const QString& message);
    void fetchProfile();
public:
    ElyStep(AccountData* data,QNetworkAccessManager* network,bool silent=false,bool deviceMode=true);
    QString describe() override {return OrbitI18n::tr("Autenticando com Ely.by…");}
    void perform() override;
    void abort() override;
    static bool applyProfile(AccountData* data,const QJsonObject& profile);
signals:
    void authorizeWithBrowser(const QUrl& url);
    void statusMessage(const QString& message);
};
