// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include <QTemporaryDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTimer>
#include <cstring>
#include "updater/OrbitUpdateService.h"

namespace {
const QString Repo="owner/orbit-launcher";
QJsonObject release(const QByteArray& body="installer fixture"){
    return {{"draft",false},{"prerelease",false},{"tag_name","v1.1.0"},{"published_at","2026-10-23T10:00:00Z"},
        {"assets",QJsonArray{QJsonObject{{"name","OrbitLauncher-Setup-1.1.0-x64.exe"},{"state","uploaded"},
        {"browser_download_url","https://github.com/owner/orbit-launcher/releases/download/v1.1.0/OrbitLauncher-Setup-1.1.0-x64.exe"},
        {"size",body.size()},{"digest","sha256:"+QString::fromLatin1(QCryptographicHash::hash(body,QCryptographicHash::Sha256).toHex())}}}}};
}
class Reply : public QNetworkReply {
    QByteArray m_body;qint64 m_pos=0;
public:
    Reply(const QNetworkRequest& req,QByteArray body,QObject* parent,int status=200):QNetworkReply(parent),m_body(std::move(body)){
        setRequest(req);setUrl(req.url());setAttribute(QNetworkRequest::HttpStatusCodeAttribute,status);
        if(status==302)setAttribute(QNetworkRequest::RedirectionTargetAttribute,QUrl("https://evil.test/payload.exe"));
        open(QIODevice::ReadOnly);QTimer::singleShot(0,this,[this]{emit readyRead();setFinished(true);emit finished();});
    }
    void abort() override {setError(OperationCanceledError,"fixture cancelled");}
    qint64 bytesAvailable()const override{return m_body.size()-m_pos+QNetworkReply::bytesAvailable();}
    qint64 readData(char* data,qint64 max)override{auto size=qMin(max,m_body.size()-m_pos);if(size<=0)return -1;std::memcpy(data,m_body.constData()+m_pos,size);m_pos+=size;return size;}
};
class Network : public QNetworkAccessManager {
public:
    QJsonObject metadata=release();QByteArray body="installer fixture";int requests=0;bool evilRedirect=false;
    QNetworkReply* createRequest(Operation,const QNetworkRequest& req,QIODevice*)override{
        ++requests;
        if(req.url().host()=="api.github.com")return new Reply(req,QJsonDocument(metadata).toJson(),this);
        return new Reply(req,body,this,evilRedirect?302:200);
    }
};
}
class OrbitUpdateTest:public QObject {
    Q_OBJECT
private slots:
    void stableReleasePolicy(){
        QVERIFY(OrbitUpdateService::selectAsset(release(),Repo,"1.0.0"));
        QVERIFY(!OrbitUpdateService::selectAsset(release(),Repo,"1.1.0"));
        QVERIFY(!OrbitUpdateService::selectAsset(release(),Repo,"2.0.0"));
        for(const auto key:{"draft","prerelease"}){auto data=release();data[key]=true;QVERIFY(!OrbitUpdateService::selectAsset(data,Repo,"1.0.0"));}
        for(const auto tag:{"v1.2.0-beta","main","v01.2.0","1.2.0"}){auto data=release();data["tag_name"]=tag;QVERIFY(!OrbitUpdateService::selectAsset(data,Repo,"1.0.0"));}
        auto unpublished=release();unpublished.remove("published_at");QVERIFY(!OrbitUpdateService::selectAsset(unpublished,Repo,"1.0.0"));
    }
    void rejectsInvalidAssets(){
        for(const auto key:{"digest","state","size","browser_download_url"}){
            auto data=release();auto asset=data["assets"].toArray()[0].toObject();asset.remove(key);data["assets"]=QJsonArray{asset};
            QVERIFY(!OrbitUpdateService::selectAsset(data,Repo,"1.0.0"));
        }
        QVERIFY(!OrbitUpdateService::allowedDownload(QUrl("http://github.com/owner/orbit-launcher/releases/download/v1.1.0/test.exe"),Repo));
        QVERIFY(!OrbitUpdateService::allowedDownload(QUrl("https://github.com/attacker/project/releases/download/v1.1.0/test.exe"),Repo));
        QVERIFY(!OrbitUpdateService::allowedDownload(QUrl("https://user@github.com/owner/orbit-launcher/releases/download/v1.1.0/test.exe"),Repo));
        QVERIFY(!OrbitUpdateService::allowedDownload(QUrl("https://evil.test/test.exe"),Repo,true));
        QVERIFY(OrbitUpdateService::allowedDownload(QUrl("https://release-assets.githubusercontent.com/fixture"),Repo,true));
        QVERIFY(!OrbitUpdateService::validRepository("owner/../attacker"));
    }
    void verifiesDownloadBeforeHandover(){
        QTemporaryDir temp;Network net;OrbitUpdateService service(&net,Repo,"1.0.0",temp.path(),true);
        QSignalSpy ready(&service,&OrbitUpdateService::ready),done(&service,&OrbitUpdateService::finished);service.check();
        QTRY_COMPARE(done.count(),1);QCOMPARE(ready.count(),1);QCOMPARE(net.requests,2);
        QFile file(ready[0][0].toString());QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),net.body);
    }
    void corruptedAndOversizedNeverExecute(){
        for(const auto body:{QByteArray("corrupt"),QByteArray(2048,'x')}){
            QTemporaryDir temp;Network net;net.body=body;OrbitUpdateService service(&net,Repo,"1.0.0",temp.path(),true);
            QSignalSpy ready(&service,&OrbitUpdateService::ready),done(&service,&OrbitUpdateService::finished);service.check();
            QTRY_COMPARE(done.count(),1);QCOMPARE(ready.count(),0);
            QVERIFY(!QFile::exists(temp.filePath("1.1.0/OrbitLauncher-Setup-1.1.0-x64.exe")));
        }
    }
    void untrustedRedirectNeverExecutes(){
        QTemporaryDir temp;Network net;net.evilRedirect=true;OrbitUpdateService service(&net,Repo,"1.0.0",temp.path(),true);
        QSignalSpy ready(&service,&OrbitUpdateService::ready),done(&service,&OrbitUpdateService::finished);service.check();
        QTRY_COMPARE(done.count(),1);QCOMPARE(ready.count(),0);QCOMPARE(net.requests,2);
    }
    void developmentBuildNeverChecks(){
        QTemporaryDir temp;Network net;OrbitUpdateService service(&net,Repo,"1.0.0",temp.path(),false);
        service.check();QTest::qWait(50);QCOMPARE(net.requests,0);
    }
    void cancellationNeverApplies(){
        QTemporaryDir temp;Network net;OrbitUpdateService service(&net,Repo,"1.0.0",temp.path(),true);
        QSignalSpy ready(&service,&OrbitUpdateService::ready);service.check();service.cancel();QTest::qWait(100);QCOMPARE(ready.count(),0);
    }
};
QTEST_MAIN(OrbitUpdateTest)
#include "OrbitUpdate_test.moc"
