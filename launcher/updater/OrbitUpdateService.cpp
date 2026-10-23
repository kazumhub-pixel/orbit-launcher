// SPDX-License-Identifier: GPL-3.0-only
#include "OrbitUpdateService.h"
#include <QDateTime>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QVersionNumber>

namespace {
constexpr qint64 MaxInstaller=512LL*1024*1024;
QNetworkRequest requestFor(const QUrl& url){
    QNetworkRequest request(url);request.setTransferTimeout(15000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
    request.setRawHeader("User-Agent","OrbitLauncher-Updater");
    return request;
}
}

OrbitUpdateService::OrbitUpdateService(QNetworkAccessManager* network,QString repository,QString currentVersion,
                                     QString cache,bool official,QObject* parent)
    :QObject(parent),m_network(network),m_repository(std::move(repository)),m_current(std::move(currentVersion)),
      m_cache(std::move(cache)),m_official(official){}

bool OrbitUpdateService::validRepository(const QString& repository){
    return QRegularExpression("^[A-Za-z0-9][A-Za-z0-9-]{0,38}/[A-Za-z0-9][A-Za-z0-9_.-]{0,99}$").match(repository).hasMatch();
}
bool OrbitUpdateService::allowedDownload(const QUrl& url,const QString& repository,bool redirected){
    if(!validRepository(repository) || url.scheme()!="https" || !url.userInfo().isEmpty() || (url.port()!=-1 && url.port()!=443))return false;
    if(url.host()=="github.com")return url.path().startsWith("/"+repository+"/releases/download/") && url.query().isEmpty();
    return redirected && url.host()=="release-assets.githubusercontent.com";
}
std::optional<OrbitUpdateAsset> OrbitUpdateService::selectAsset(const QJsonObject& release,const QString& repository,const QString& currentVersion){
    if(!validRepository(repository) || !release.contains("draft") || !release.contains("prerelease") ||
       !release["draft"].isBool() || !release["prerelease"].isBool() || release["draft"].toBool() || release["prerelease"].toBool())return {};
    const auto tag=release["tag_name"].toString();
    if(!QRegularExpression("^v(0|[1-9][0-9]{0,4})\\.(0|[1-9][0-9]{0,4})\\.(0|[1-9][0-9]{0,4})$").match(tag).hasMatch())return {};
    const auto version=tag.mid(1);
    if(QVersionNumber::fromString(version)<=QVersionNumber::fromString(currentVersion) ||
       !QDateTime::fromString(release["published_at"].toString(),Qt::ISODate).isValid())return {};
    const auto name="OrbitLauncher-Setup-"+version+"-x64.exe";
    std::optional<OrbitUpdateAsset> result;
    for(const auto& value:release["assets"].toArray()){
        const auto asset=value.toObject();if(asset["name"].toString()!=name)continue;
        if(result)return {}; // Ambiguous release assets are never executed.
        const auto digest=asset["digest"].toString();const auto size=asset["size"].toInteger();
        const QUrl url(asset["browser_download_url"].toString());
        if(asset["state"].toString()!="uploaded" || size<=0 || size>MaxInstaller ||
           !QRegularExpression("^sha256:[a-f0-9]{64}$").match(digest).hasMatch() || !allowedDownload(url,repository) ||
           url.path()!="/"+repository+"/releases/download/"+tag+"/"+name)return {};
        result=OrbitUpdateAsset{version,name,url,QByteArray::fromHex(digest.mid(7).toLatin1()),size};
    }
    return result;
}
void OrbitUpdateService::check(){
    if(m_started || m_stopped || !m_official || !validRepository(m_repository))return;
    m_started=true;emit checking();fetchMetadata();
}
void OrbitUpdateService::fetchMetadata(){
    auto request=requestFor(QUrl("https://api.github.com/repos/"+m_repository+"/releases/latest"));
    request.setRawHeader("Accept","application/vnd.github+json");
    auto* reply=m_network->get(request);m_reply=reply;reply->setReadBufferSize(65536);
    connect(reply,&QNetworkReply::readyRead,this,[this,reply]{
        m_metadata+=reply->readAll();if(m_metadata.size()>1024*1024)fail();
    });
    connect(reply,&QNetworkReply::finished,this,[this,reply]{
        if(m_stopped || m_reply!=reply)return;
        m_metadata+=reply->readAll();m_reply.clear();reply->deleteLater();
        if(reply->error()!=QNetworkReply::NoError || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()!=200 || m_metadata.size()>1024*1024){fail();return;}
        m_asset=selectAsset(QJsonDocument::fromJson(m_metadata).object(),m_repository,m_current);m_metadata.clear();
        if(!m_asset){m_stopped=true;emit finished(false);return;}
        const auto folder=QDir(m_cache).filePath(m_asset->version);
        if(!QDir().mkpath(folder)){fail();return;}
        m_file=std::make_unique<QSaveFile>(QDir(folder).filePath(m_asset->name));
        if(!m_file->open(QIODevice::WriteOnly)){fail();return;}
        emit downloading(m_asset->version);download(m_asset->url);
    });
}
bool OrbitUpdateService::consumeDownload(){
    if(!m_reply || !m_file || !m_asset)return false;
    if(m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()!=200){m_reply->readAll();return true;}
    const auto bytes=m_reply->readAll();m_received+=bytes.size();
    if(m_received>m_asset->size || m_received>MaxInstaller || m_file->write(bytes)!=bytes.size()){fail();return false;}
    m_hash.addData(bytes);return true;
}
void OrbitUpdateService::download(const QUrl& url,int redirects){
    if(m_stopped)return;
    if(redirects>5 || !allowedDownload(url,m_repository,redirects>0)){fail();return;}
    auto* reply=m_network->get(requestFor(url));m_reply=reply;reply->setReadBufferSize(65536);
    connect(reply,&QNetworkReply::readyRead,this,[this]{consumeDownload();});
    connect(reply,&QNetworkReply::finished,this,[this,reply,url,redirects]{
        if(m_stopped || m_reply!=reply)return;
        const int status=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if(status>=300 && status<400){
            const auto target=url.resolved(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
            m_reply.clear();reply->deleteLater();download(target,redirects+1);return;
        }
        if(!consumeDownload())return;
        m_reply.clear();reply->deleteLater();
        if(reply->error()!=QNetworkReply::NoError || status!=200 || m_received!=m_asset->size || m_hash.result()!=m_asset->sha256){fail();return;}
        const auto path=m_file->fileName();
        if(!m_file->commit()){fail();return;}
        m_file.reset();m_stopped=true;emit ready(path);emit finished(true);
    });
}
void OrbitUpdateService::cancel(){
    m_stopped=true;
    if(m_reply){m_reply->disconnect(this);m_reply->abort();m_reply->deleteLater();m_reply.clear();}
    if(m_file){m_file->cancelWriting();m_file.reset();}
}
void OrbitUpdateService::fail(){cancel();emit finished(false);}
