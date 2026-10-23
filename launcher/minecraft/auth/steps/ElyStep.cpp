// SPDX-License-Identifier: GPL-3.0-only
#include "ElyStep.h"
#include "translations/OrbitI18n.h"
#include <QHostAddress>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUuid>
#include <QUrlQuery>
ElyStep::ElyStep(AccountData* data,QNetworkAccessManager* network,bool silent,bool deviceMode)
    :AuthStep(data),m_flow(network,this),m_device(network,this),m_active(deviceMode?static_cast<QAbstractOAuth2*>(&m_device):static_cast<QAbstractOAuth2*>(&m_flow)),m_deviceMode(deviceMode),m_network(network),m_silent(silent) {
    m_handler=new QOAuthHttpServerReplyHandler(QHostAddress::LocalHost,0,this);
    m_handler->setCallbackHost("127.0.0.1");
    m_handler->setCallbackPath("/oauth/ely/callback");
    m_handler->setCallbackText(OrbitI18n::tr("Autorização recebida. Volte ao Orbit Launcher para concluir."));
    if(m_silent || m_deviceMode)m_handler->close();
    m_flow.setReplyHandler(m_handler);
    m_flow.setAuthorizationUrl(QUrl("https://account.ely.by/oauth2/v1"));
    m_flow.setTokenUrl(QUrl("https://account.ely.by/api/oauth2/v1/token"));
    m_flow.setClientIdentifier("orbit-launcher");
    m_flow.setRequestedScopeTokens({"account_info","offline_access","minecraft_server_session"});
    m_flow.setPkceMethod(QOAuth2AuthorizationCodeFlow::PkceMethod::S256,64);
    m_flow.setAutoRefresh(false);
    m_device.setAuthorizationUrl(QUrl("https://account.ely.by/api/oauth2/v1/devicecode"));
    m_device.setTokenUrl(QUrl("https://account.ely.by/api/oauth2/v1/token"));
    m_device.setClientIdentifier("orbit-launcher");
    m_device.setRequestedScopeTokens({"account_info","offline_access","minecraft_server_session"});m_device.setAutoRefresh(false);
    // Bound every OAuth request and avoid intermittent HTTP/2 stalls at the provider.
    const auto configureRequest=[](QNetworkRequest& request,QAbstractOAuth2::Stage){
        request.setTransferTimeout(15000);
        request.setAttribute(QNetworkRequest::Http2AllowedAttribute,false);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
    };
    m_device.setNetworkRequestModifier(this,configureRequest);
    m_flow.setNetworkRequestModifier(this,configureRequest);
    connect(&m_device,&QOAuth2DeviceAuthorizationFlow::authorizeWithUserCode,this,[this](const QUrl& verification,const QString& code,const QUrl& complete){
        QUrl url=complete.isValid() && !complete.isEmpty()?complete:verification;
        if(url.scheme()=="http" && url.host()=="account.ely.by" && url.userInfo().isEmpty() && (url.port()==-1 || url.port()==80)){url.setScheme("https");url.setPort(-1);}
        if(url.scheme()!="https" || url.host()!="account.ely.by" || !url.userInfo().isEmpty() || (url.port()!=-1 && url.port()!=443)){finish(AccountTaskState::STATE_FAILED_HARD,OrbitI18n::tr("URL Ely.by inválida."));return;}
        if(complete.isEmpty()){QUrlQuery query(url);query.addQueryItem("user_code",code);url.setQuery(query);}
        emit authorizeWithBrowser(url);if(!m_done)m_device.startTokenPolling();
    });
    connect(&m_device,&QAbstractOAuth::granted,this,&ElyStep::fetchProfile);
    connect(&m_device,&QAbstractOAuth::requestFailed,this,[this](QAbstractOAuth::Error error){
        if(m_done)return;
        if(error==QAbstractOAuth::Error::NetworkError && !m_silent && !m_device.userCode().isEmpty() && ++m_networkRetries<=3){
            emit statusMessage(OrbitI18n::tr("Conexão interrompida. Tentando consultar a autorização novamente…"));
            // Qt stops polling after emitting requestFailed. Restart on the next event turn.
            QTimer::singleShot(1000,this,[this]{if(!m_done)m_device.startTokenPolling();});
            return;
        }
        finish(error==QAbstractOAuth::Error::NetworkError?AccountTaskState::STATE_OFFLINE:AccountTaskState::STATE_FAILED_HARD,
               error==QAbstractOAuth::Error::NetworkError?OrbitI18n::tr("Sem resposta do Ely.by. Confira a conexão e tente novamente."):OrbitI18n::tr("O código Ely.by expirou ou a autorização foi recusada. Tente novamente."));
    });

    m_timeout.setSingleShot(true);m_timeout.setInterval(silent?45000:300000);
    connect(&m_timeout,&QTimer::timeout,this,[this]{finish(AccountTaskState::STATE_FAILED_SOFT,OrbitI18n::tr("O login Ely.by expirou. Tente novamente em Perfil."));});
    connect(&m_flow,&QAbstractOAuth::authorizeWithBrowser,this,&ElyStep::authorizeWithBrowser);
    connect(&m_flow,&QAbstractOAuth::granted,this,&ElyStep::fetchProfile);
    connect(&m_flow,&QAbstractOAuth::requestFailed,this,[this](QAbstractOAuth::Error error){
        finish(error==QAbstractOAuth::Error::NetworkError?AccountTaskState::STATE_OFFLINE:AccountTaskState::STATE_FAILED_HARD,OrbitI18n::tr("Não foi possível autenticar com Ely.by. Confira a conexão e entre novamente em Perfil."));
    });
    connect(&m_flow,&QAbstractOAuth2::serverReportedErrorOccurred,this,[this](const QString&,const QString&,const QUrl&){finish(AccountTaskState::STATE_FAILED_HARD,OrbitI18n::tr("O Ely.by recusou a autorização. Entre novamente em Perfil."));});
}
void ElyStep::perform() {
    m_timeout.start();
    if(m_silent){
        if(m_data->yggdrasilToken.refresh_token.isEmpty()){finish(AccountTaskState::STATE_FAILED_HARD,OrbitI18n::tr("Entre novamente na conta Ely.by em Perfil."));return;}
        m_active->setRefreshToken(m_data->yggdrasilToken.refresh_token);m_active->refreshTokens();
    } else {
        if(m_deviceMode){m_device.grant();return;}
        if(!m_handler->isListening()){finish(AccountTaskState::STATE_FAILED_SOFT,OrbitI18n::tr("Não foi possível abrir o retorno local do login Ely.by."));return;}
        m_flow.grant();
    }
}
bool ElyStep::applyProfile(AccountData* data,const QJsonObject& profile){
    const auto uuid=QUuid(profile["uuid"].toString());const auto name=profile["username"].toString();
    if(uuid.isNull() || !QRegularExpression("^[A-Za-z0-9_]{3,16}$").match(name).hasMatch())return false;
    data->type=AccountType::Ely;data->minecraftProfile.id=uuid.toString(QUuid::Id128);data->minecraftProfile.name=name;data->minecraftProfile.validity=Validity::Certain;
    data->minecraftEntitlement.canPlayMinecraft=true;data->minecraftEntitlement.ownsMinecraft=false;data->minecraftEntitlement.validity=Validity::Certain;return true;
}
void ElyStep::fetchProfile(){
    if(m_done)return;
    if(m_active->token().isEmpty()){finish(AccountTaskState::STATE_FAILED_HARD,OrbitI18n::tr("O Ely.by não retornou uma sessão válida."));return;}
    emit statusMessage(OrbitI18n::tr("Autorização recebida. Carregando seu perfil…"));
    QNetworkRequest request(QUrl("https://account.ely.by/api/account/v1/info"));request.setTransferTimeout(15000);request.setAttribute(QNetworkRequest::Http2AllowedAttribute,false);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
    request.setRawHeader("Authorization","Bearer "+m_active->token().toUtf8());
    m_reply=m_network->get(request);auto reply=m_reply;
    connect(reply,&QNetworkReply::finished,this,[this,reply]{
        if(!reply)return;
        const bool ok=reply->error()==QNetworkReply::NoError && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200;
        const auto bytes=reply->readAll();reply->deleteLater();m_reply.clear();if(m_done)return;
        if(!ok || bytes.size()>65536 || !applyProfile(m_data,QJsonDocument::fromJson(bytes).object())){finish(AccountTaskState::STATE_FAILED_HARD,OrbitI18n::tr("Não foi possível confirmar o perfil Ely.by."));return;}
        auto& token=m_data->yggdrasilToken;token.token=m_active->token();
        if(!m_active->refreshToken().isEmpty())token.refresh_token=m_active->refreshToken();
        token.issueInstant=QDateTime::currentDateTimeUtc();token.notAfter=m_active->expirationAt();token.validity=Validity::Certain;
        m_data->networkError=QNetworkReply::NoError;finish(AccountTaskState::STATE_WORKING,OrbitI18n::tr("Perfil Ely.by confirmado."));
    });
}
void ElyStep::finish(AccountTaskState state,const QString& message){
    if(m_done)return;m_done=true;m_timeout.stop();m_handler->close();m_device.stopTokenPolling();
    if(m_reply){m_reply->disconnect(this);m_reply->abort();m_reply->deleteLater();m_reply.clear();}
    emit finished(state,message);
}
void ElyStep::abort(){m_done=true;m_timeout.stop();m_handler->close();m_device.stopTokenPolling();if(m_reply){m_reply->disconnect(this);m_reply->abort();m_reply->deleteLater();m_reply.clear();}}
