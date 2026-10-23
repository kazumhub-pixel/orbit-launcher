// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "translations/OrbitI18n.h"
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QRegularExpression>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
class OrbitLocalSkins {
    QString m_root;
public:
    explicit OrbitLocalSkins(QString root) : m_root(std::move(root)) {}
    QString path(const QString& id) const {
        if (!QRegularExpression("^[a-fA-F0-9]{32}$").match(id).hasMatch()) return {};
        return QDir(m_root).filePath(id+".png");
    }
    bool importSkin(const QString& id, const QString& source, QString& error) const {
        QImageReader reader(source);
        const auto size=reader.size();
        if (reader.format().toLower()!="png" || (size!=QSize(64,64) && size!=QSize(64,32)) || QFileInfo(source).size()>1024*1024) {
            error=OrbitI18n::tr("Escolha uma skin PNG de 64×64 ou 64×32 pixels, até 1 MB."); return false;
        }
        auto image=reader.read();
        if(image.isNull() || path(id).isEmpty()) { error=OrbitI18n::tr("Não foi possível ler a skin."); return false; }
        if(!QDir().mkpath(m_root)) { error=OrbitI18n::tr("Não foi possível criar a pasta de skins."); return false; }
        QSaveFile file(path(id));
        if(!file.open(QIODevice::WriteOnly) || !image.save(&file,"PNG") || !file.commit()) { error=OrbitI18n::tr("Não foi possível salvar a skin."); return false; }
        return true;
    }
    bool prepareInstance(const QString& id, const QString& nickname, const QString& gameRoot, QString& error) const {
        if(gameRoot.isEmpty() || !QRegularExpression("^[A-Za-z0-9_]{3,16}$").match(nickname).hasMatch()) { error=OrbitI18n::tr("Selecione uma instância e um apelido válido.");return false; }
        const auto source=path(id);QImage image(source);
        if(source.isEmpty() || (image.size()!=QSize(64,64) && image.size()!=QSize(64,32))) { error=OrbitI18n::tr("Importe uma skin válida para este perfil.");return false; }
        const auto target=QDir(gameRoot).filePath("CustomSkinLoader/LocalSkin/skins/"+nickname+".png");
        if(!QDir().mkpath(QFileInfo(target).absolutePath())){error=OrbitI18n::tr("Não foi possível criar a pasta na instância.");return false;}
        QSaveFile file(target);
        if(!file.open(QIODevice::WriteOnly) || !image.save(&file,"PNG") || !file.commit()){error=OrbitI18n::tr("Não foi possível preparar a skin na instância.");return false;}
        const auto orbitPath=QDir(gameRoot).filePath("CustomSkinLoader/OrbitSkins/"+nickname+".png");
        if(!QDir().mkpath(QFileInfo(orbitPath).absolutePath())) {error=OrbitI18n::tr("Não foi possível criar a pasta OrbitSkins.");return false;}
        QSaveFile orbitFile(orbitPath);
        if(!orbitFile.open(QIODevice::WriteOnly) || !image.save(&orbitFile,"PNG") || !orbitFile.commit()){error=OrbitI18n::tr("Não foi possível sincronizar a skin.");return false;}
        const auto extraPath=QDir(gameRoot).filePath("CustomSkinLoader/ExtraList/orbit-local.json");
        if(!QDir().mkpath(QFileInfo(extraPath).absolutePath())){error=OrbitI18n::tr("Não foi possível preparar a prioridade da skin.");return false;}
        const QJsonObject profile{{"name","Orbit Local"},{"type","Legacy"},{"skin","OrbitSkins/{USERNAME}.png"},{"checkPNG",true}};
        QSaveFile extra(extraPath);const auto bytes=QJsonDocument(profile).toJson();
        if(!extra.open(QIODevice::WriteOnly) || extra.write(bytes)!=bytes.size() || !extra.commit()){error=OrbitI18n::tr("Não foi possível salvar a prioridade da skin.");return false;}
        return true;
    }
    bool remove(const QString& id) const { const auto file=path(id); return !file.isEmpty() && (!QFile::exists(file) || QFile::remove(file)); }
};
