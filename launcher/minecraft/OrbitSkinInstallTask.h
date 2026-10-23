// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "translations/OrbitI18n.h"
#include <QCryptographicHash>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QUrlQuery>
#include "tasks/Task.h"
#include "minecraft/OrbitLocalSkins.h"

class OrbitSkinInstallTask : public Task {
    QNetworkAccessManager* m_network;
    QString m_game, m_cache, m_skins, m_version, m_loader, m_name, m_uuid;
    bool m_online;
    QPointer<QNetworkReply> m_reply;
    QByteArray m_bytes;
    QJsonObject m_file;
    QString cacheIndex() const {
        const auto key=QCryptographicHash::hash((m_version+"/"+m_loader).toUtf8(),QCryptographicHash::Sha256).toHex();
        return QDir(m_cache).filePath(QString::fromLatin1(key)+".json");
    }
    bool write(const QString& path,const QByteArray& bytes) {
        if(!QDir().mkpath(QFileInfo(path).absolutePath()))return false;
        QSaveFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size() && file.commit();
    }
    QByteArray read(const QString& path) const { QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>32*1024*1024)return {};return file.readAll(); }
    bool validFile(const QJsonObject& file) const {
        const auto filename=file["filename"].toString();const QUrl url(file["url"].toString());
        return QRegularExpression("^[A-Za-z0-9_.+\\-]+\\.jar$").match(filename).hasMatch()
            && url.scheme()=="https" && url.host()=="cdn.modrinth.com" && url.userInfo().isEmpty()
            && QRegularExpression("^[0-9a-f]{128}$").match(file["hashes"].toObject()["sha512"].toString()).hasMatch()
            && file["size"].toDouble()>0 && file["size"].toDouble()<=32*1024*1024;
    }
    bool validBytes(const QByteArray& bytes) const {
        return bytes.size()==m_file["size"].toInt() && QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha512).toHex())==m_file["hashes"].toObject()["sha512"].toString();
    }
    QString jarCache() const { return QDir(m_cache).filePath(m_file["hashes"].toObject()["sha512"].toString()+".jar"); }
    void finishInstall(const QByteArray& jar) {
        if(!isRunning())return;
        const auto mods=QDir(m_game).filePath("mods");const auto target=QDir(mods).filePath(m_file["filename"].toString());
        for(const auto& existing:QDir(mods).entryList({"CustomSkinLoader*.jar"},QDir::Files)) {
            if(existing!=m_file["filename"].toString()) {emitFailed(OrbitI18n::tr("Há outra versão do CustomSkinLoader na instância. Desative a versão antiga em Mods e tente novamente."));return;}
        }
        if(read(target)!=jar && !write(target,jar)){emitFailed(OrbitI18n::tr("Não foi possível instalar o mod de skins. Verifique a permissão da pasta mods."));return;}
        OrbitLocalSkins skins(m_skins);
        if(QFile::exists(skins.path(m_uuid))){QString error;if(!skins.prepareInstance(m_uuid,m_name,m_game,error)){emitFailed(error);return;}}
        setStatus(OrbitI18n::tr("Skins automáticas prontas para esta instância."));emitSucceeded();
    }
    void fetch(const QUrl& url,qint64 limit,std::function<void(QByteArray)> done) {
        m_bytes.clear();QNetworkRequest request(url);request.setTransferTimeout(30000);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::SameOriginRedirectPolicy);
        request.setRawHeader("User-Agent","OrbitLauncher/1.0 (automatic skins)");
        m_reply=m_network->get(request);auto reply=m_reply;
        connect(reply,&QNetworkReply::readyRead,this,[this,reply,limit]{
            if(!reply)return;m_bytes+=reply->readAll();
            if(m_bytes.size()>limit){reply->abort();}
        });
        connect(reply,&QNetworkReply::downloadProgress,this,[this](qint64 current,qint64 total){if(total>0)setProgress(current,total);});
        connect(reply,&QNetworkReply::finished,this,[this,reply,limit,done]{
            if(!reply)return;
            m_bytes+=reply->readAll();const auto bytes=m_bytes;const bool ok=reply->error()==QNetworkReply::NoError && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200 && bytes.size()<=limit;
            reply->deleteLater();m_reply.clear();
            if(!isRunning())return;
            if(!ok){emitFailed(OrbitI18n::tr("Falha ao baixar o mod de skins. Confira a conexão ou desative Skins automáticas nesta instância."));return;}
            done(bytes);
        });
    }
protected:
    void executeTask() override {
        setStatus(OrbitI18n::tr("Preparando skins automáticas…"));
        if(m_loader.isEmpty()){emitFailed(OrbitI18n::tr("Skins automáticas precisam de Fabric, Quilt, Forge ou NeoForge. Instale um carregador em Versão ou desative a opção em Perfil."));return;}
        const auto cache=QJsonDocument::fromJson(read(cacheIndex())).object();
        m_file=cache["file"].toObject();
        if(cache["minecraft"]==m_version && cache["loader"]==m_loader && validFile(m_file)) {
            const auto jar=read(jarCache());if(validBytes(jar)){finishInstall(jar);return;}
        }
        if(!m_online){emitFailed(OrbitI18n::tr("O mod de skins ainda não está no cache. Inicie uma vez com Internet ou desative Skins automáticas."));return;}
        QUrl url("https://api.modrinth.com/v2/project/customskinloader/version");QUrlQuery query;
        query.addQueryItem("loaders",QString::fromUtf8(QJsonDocument(QJsonArray{m_loader}).toJson(QJsonDocument::Compact)));
        query.addQueryItem("game_versions",QString::fromUtf8(QJsonDocument(QJsonArray{m_version}).toJson(QJsonDocument::Compact)));url.setQuery(query);
        fetch(url,2*1024*1024,[this](const QByteArray& bytes){
            const auto versions=QJsonDocument::fromJson(bytes).array();m_file={};
            for(const auto& entry:versions){const auto version=entry.toObject();
                if(version["version_type"]!="release" || !version["game_versions"].toArray().contains(m_version) || !version["loaders"].toArray().contains(m_loader))continue;
                for(const auto& candidate:version["files"].toArray())if(candidate.toObject()["primary"].toBool() && validFile(candidate.toObject())){m_file=candidate.toObject();break;}
                if(!m_file.isEmpty())break;
            }
            if(m_file.isEmpty()){emitFailed(OrbitI18n::tr("Não há CustomSkinLoader estável compatível com esta versão e carregador. Desative Skins automáticas para continuar sem ele."));return;}
            fetch(QUrl(m_file["url"].toString()),32*1024*1024,[this](const QByteArray& jar){
                if(!validBytes(jar)){emitFailed(OrbitI18n::tr("O arquivo de skins não passou na verificação SHA-512. Nada foi instalado."));return;}
                const auto index=QJsonDocument(QJsonObject{{"minecraft",m_version},{"loader",m_loader},{"file",m_file}}).toJson();
                if(!write(jarCache(),jar) || !write(cacheIndex(),index)){emitFailed(OrbitI18n::tr("Não foi possível salvar o cache de skins."));return;}
                finishInstall(jar);
            });
        });
    }
public:
    OrbitSkinInstallTask(QNetworkAccessManager* network,QString game,QString cache,QString skins,QString version,QString loader,QString name,QString uuid,bool online)
        :m_network(network),m_game(std::move(game)),m_cache(std::move(cache)),m_skins(std::move(skins)),m_version(std::move(version)),m_loader(std::move(loader)),m_name(std::move(name)),m_uuid(std::move(uuid)),m_online(online){setAbortable(true);}
    ~OrbitSkinInstallTask() override {if(m_reply){m_reply->disconnect(this);m_reply->abort();m_reply->deleteLater();}}
    bool abort() override {if(!isRunning())return false;emitAborted();if(m_reply)m_reply->abort();return true;}
};
