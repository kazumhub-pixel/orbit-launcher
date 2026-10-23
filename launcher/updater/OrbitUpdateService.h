// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QObject>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QSaveFile>
#include <QCryptographicHash>
#include <optional>
#include <memory>

struct OrbitUpdateAsset {
    QString version,name;
    QUrl url;
    QByteArray sha256;
    qint64 size=0;
};

class OrbitUpdateService : public QObject {
    Q_OBJECT
    QNetworkAccessManager* m_network;
    QString m_repository,m_current,m_cache;
    bool m_official,m_started=false,m_stopped=false;
    QPointer<QNetworkReply> m_reply;
    QByteArray m_metadata;
    std::optional<OrbitUpdateAsset> m_asset;
    std::unique_ptr<QSaveFile> m_file;
    QCryptographicHash m_hash{QCryptographicHash::Sha256};
    qint64 m_received=0;
    void fetchMetadata();
    void download(const QUrl& url,int redirects=0);
    bool consumeDownload();
    void fail();
public:
    OrbitUpdateService(QNetworkAccessManager* network,QString repository,QString currentVersion,
                       QString cache,bool official,QObject* parent=nullptr);
    void check();
    void cancel();
    static bool validRepository(const QString& repository);
    static bool allowedDownload(const QUrl& url,const QString& repository,bool redirected=false);
    static std::optional<OrbitUpdateAsset> selectAsset(const QJsonObject& release,const QString& repository,const QString& currentVersion);
signals:
    void checking();
    void downloading(QString version);
    void ready(QString installer);
    void finished(bool updateReady);
};
