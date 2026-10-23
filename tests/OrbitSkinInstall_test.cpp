// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include "minecraft/auth/MinecraftAccount.h"
#include <QTemporaryDir>
#include <QTimer>
#include <cstring>
#include "minecraft/OrbitSkinInstallTask.h"
class SkinTestReply : public QNetworkReply {
    QByteArray m_data; qint64 m_pos=0;bool m_done=false;
public:
    SkinTestReply(const QNetworkRequest& request,QByteArray data,QObject* parent):QNetworkReply(parent),m_data(std::move(data)){
        setRequest(request);setUrl(request.url());setAttribute(QNetworkRequest::HttpStatusCodeAttribute,200);open(QIODevice::ReadOnly);
        QTimer::singleShot(0,this,[this]{if(m_done)return;m_done=true;emit readyRead();setFinished(true);emit finished();});
    }
    void abort() override {if(m_done)return;m_done=true;setError(QNetworkReply::OperationCanceledError,"Canceled");setFinished(true);emit finished();}
    qint64 bytesAvailable() const override {return m_data.size()-m_pos+QNetworkReply::bytesAvailable();}
protected:
    qint64 readData(char* data,qint64 max) override {const auto count=qMin(max,m_data.size()-m_pos);if(count<=0)return -1;std::memcpy(data,m_data.constData()+m_pos,count);m_pos+=count;return count;}
};
class SkinTestNetwork : public QNetworkAccessManager {
public:
    QByteArray jar="verified fixture jar";bool corrupt=false;int requests=0;
protected:
    QNetworkReply* createRequest(Operation,const QNetworkRequest& request,QIODevice*) override {
        ++requests;QByteArray bytes;
        if(request.url().host()=="api.modrinth.com") {
            QJsonObject file{{"filename","CustomSkinLoader-test.jar"},{"url","https://cdn.modrinth.com/test.jar"},{"primary",true},{"size",jar.size()},{"hashes",QJsonObject{{"sha512",QString::fromLatin1(QCryptographicHash::hash(jar,QCryptographicHash::Sha512).toHex())}}}};
            bytes=QJsonDocument(QJsonArray{QJsonObject{{"version_type","release"},{"game_versions",QJsonArray{"26.1.2"}},{"loaders",QJsonArray{"fabric"}},{"files",QJsonArray{file}}}}).toJson();
        } else bytes=corrupt?QByteArray("bad data"):jar;
        return new SkinTestReply(request,bytes,this);
    }
};
class OrbitSkinInstallTest : public QObject {
    Q_OBJECT
private slots:
    void downloadsVerifiesAndReusesOfflineCache() {
        QTemporaryDir temp;QVERIFY(temp.isValid());SkinTestNetwork network;
        const auto id=MinecraftAccount::uuidFromUsername("OrbitPlayer").toString(QUuid::Id128);
        const auto skinRoot=temp.filePath("skins");OrbitLocalSkins skins(skinRoot);QImage image(64,64,QImage::Format_ARGB32);image.fill(Qt::blue);
        QVERIFY(image.save(temp.filePath("skin.png")));QString error;QVERIFY(skins.importSkin(id,temp.filePath("skin.png"),error));
        OrbitSkinInstallTask task(&network,temp.filePath("game"),temp.filePath("cache"),skinRoot,"26.1.2","fabric","OrbitPlayer",id,true);
        task.start();QTRY_VERIFY(task.isFinished());QVERIFY2(task.wasSuccessful(),qPrintable(task.failReason()));QCOMPARE(network.requests,2);
        QFile mod(temp.filePath("game/mods/CustomSkinLoader-test.jar"));QVERIFY(mod.open(QIODevice::ReadOnly));QCOMPARE(mod.readAll(),network.jar);mod.close();
        QFile extra(temp.filePath("game/CustomSkinLoader/ExtraList/orbit-local.json"));QVERIFY(extra.open(QIODevice::ReadOnly));QCOMPARE(QJsonDocument::fromJson(extra.readAll()).object()["skin"].toString(),QString("OrbitSkins/{USERNAME}.png"));extra.close();
        QVERIFY(QFile::remove(mod.fileName()));
        OrbitSkinInstallTask offline(&network,temp.filePath("game"),temp.filePath("cache"),skinRoot,"26.1.2","fabric","OrbitPlayer",id,false);
        offline.start();QVERIFY(offline.wasSuccessful());QCOMPARE(network.requests,2);QVERIFY(QFile::exists(mod.fileName()));
    }
    void rejectsCorruptDownloadAndUnsupportedLoader() {
        QTemporaryDir temp;SkinTestNetwork network;network.corrupt=true;
        OrbitSkinInstallTask bad(&network,temp.filePath("game"),temp.filePath("cache"),temp.filePath("skins"),"26.1.2","fabric","OrbitPlayer",QString(32,'a'),true);
        bad.start();QTRY_VERIFY(bad.isFinished());QVERIFY(!bad.wasSuccessful());QVERIFY(!QFile::exists(temp.filePath("game/mods/CustomSkinLoader-test.jar")));
        OrbitSkinInstallTask vanilla(&network,temp.filePath("vanilla"),temp.filePath("cache"),temp.filePath("skins"),"26.1.2",{},"OrbitPlayer",QString(32,'a'),true);
        vanilla.start();QVERIFY(vanilla.isFinished());QVERIFY(!vanilla.wasSuccessful());QCOMPARE(network.requests,2);
    }
    void cancelsWithoutInstalling() {
        QTemporaryDir temp;SkinTestNetwork network;
        OrbitSkinInstallTask task(&network,temp.filePath("game"),temp.filePath("cache"),temp.filePath("skins"),"26.1.2","fabric","OrbitPlayer",QString(32,'a'),true);
        QSignalSpy failed(&task,&Task::failed);task.start();QVERIFY(task.abort());QTest::qWait(20);
        QCOMPARE(task.getState(),Task::State::AbortedByUser);QCOMPARE(failed.size(),0);QVERIFY(!QDir(temp.filePath("game/mods")).exists());
    }
    void optionalLiveDownload() {
        if(qEnvironmentVariable("ORBIT_SKIN_NETWORK_TEST")!="1")QSKIP("Optional real Modrinth download");
        QTemporaryDir temp;QNetworkAccessManager network;
        OrbitSkinInstallTask task(&network,temp.filePath("game"),temp.filePath("cache"),temp.filePath("skins"),"26.1.2","fabric","OrbitPlayer",QString(32,'a'),true);
        task.start();QTRY_VERIFY_WITH_TIMEOUT(task.isFinished(),65000);QVERIFY2(task.wasSuccessful(),qPrintable(task.failReason()));
        QVERIFY(!QDir(temp.filePath("game/mods")).entryList({"*.jar"},QDir::Files).isEmpty());
    }
};
QTEST_MAIN(OrbitSkinInstallTest)
#include "OrbitSkinInstall_test.moc"
