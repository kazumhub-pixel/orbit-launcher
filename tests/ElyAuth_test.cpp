// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QTimer>
#include <QTemporaryDir>
#include <QTcpServer>
#include <cstring>
#include "minecraft/auth/steps/ElyStep.h"
#include "minecraft/auth/MinecraftAccount.h"
#include "minecraft/auth/ElyAgentCheck.h"
class ElyTestReply : public QNetworkReply {
    QByteArray m_data;qint64 m_pos=0;
public:
    ElyTestReply(const QNetworkRequest& request,QByteArray bytes,QObject* parent,int status=200):QNetworkReply(parent),m_data(std::move(bytes)){
        setRequest(request);setUrl(request.url());setAttribute(QNetworkRequest::HttpStatusCodeAttribute,status);setHeader(QNetworkRequest::ContentTypeHeader,"application/json");if(status==0)setError(QNetworkReply::TimeoutError,"Fixture timeout");open(QIODevice::ReadOnly);
        QTimer::singleShot(0,this,[this]{emit readyRead();setFinished(true);emit finished();});
    }
    void abort() override {setError(QNetworkReply::OperationCanceledError,"Canceled");}
    qint64 bytesAvailable() const override{return m_data.size()-m_pos+QNetworkReply::bytesAvailable();}
protected:
    qint64 readData(char* data,qint64 max) override {const auto count=qMin(max,m_data.size()-m_pos);if(count<=0)return -1;std::memcpy(data,m_data.constData()+m_pos,count);m_pos+=count;return count;}
};
class ElyTestNetwork : public QNetworkAccessManager {
public:
    int tokenRequests=0,profileRequests=0;QByteArray posted;int failedReplies=0;bool boundedRequests=true;int pendingReplies=0;QString verification="http://account.ely.by/code";
protected:
    QNetworkReply* createRequest(Operation,const QNetworkRequest& request,QIODevice* body) override {
        boundedRequests=boundedRequests && request.transferTimeout()==15000
            && !request.attribute(QNetworkRequest::Http2AllowedAttribute,true).toBool()
            && request.attribute(QNetworkRequest::RedirectPolicyAttribute).toInt()==QNetworkRequest::ManualRedirectPolicy;
        QByteArray bytes;
        if(request.url().path().endsWith("/devicecode")){
            bytes=QJsonDocument(QJsonObject{{"device_code","fixture-device"},{"user_code","FIXTURE"},{"verification_uri",verification},{"expires_in",300},{"interval",1}}).toJson();
        } else if(request.url().path().endsWith("/token")){
            ++tokenRequests;posted=body?body->readAll():QByteArray{};
            if(tokenRequests<=failedReplies)return new ElyTestReply(request,"{}",this,0);
            if(tokenRequests<=pendingReplies)return new ElyTestReply(request,R"({"error":"authorization_pending","message":"Pending"})",this,400);
            bytes=R"({"access_token":"ely-fixture-access","refresh_token":"ely-fixture-refresh","token_type":"Bearer","expires_in":3600,"scope":"account_info offline_access minecraft_server_session"})";
        } else {
            ++profileRequests;
            bytes=R"({"uuid":"ffc8fdc9-5824-509e-8a57-c99b940fb996","username":"OrbitEly"})";
        }
        return new ElyTestReply(request,bytes,this);
    }
};
class ElyAuthTest : public QObject {
    Q_OBJECT
private slots:
    void browserPkceCallbackAndEncryptedPersistence(){
        AccountData data;data.type=AccountType::Ely;ElyTestNetwork network;ElyStep step(&data,&network,false,false);
        QUrl authorization;connect(&step,&ElyStep::authorizeWithBrowser,this,[&](const QUrl& url){authorization=url;});
        QSignalSpy finished(&step,&AuthStep::finished);step.perform();QVERIFY(authorization.isValid());
        QUrlQuery query(authorization);QCOMPARE(query.queryItemValue("client_id"),QString("orbit-launcher"));
        QCOMPARE(query.queryItemValue("code_challenge_method"),QString("S256"));QVERIFY(query.queryItemValue("code_challenge").size()>=43);QVERIFY(!query.queryItemValue("state").isEmpty());QVERIFY(!query.hasQueryItem("client_secret"));
        QUrl callback(query.queryItemValue("redirect_uri"));QCOMPARE(callback.host(),QString("127.0.0.1"));
        QUrlQuery callbackQuery;callbackQuery.addQueryItem("code","fixture-code");callbackQuery.addQueryItem("state",query.queryItemValue("state"));callback.setQuery(callbackQuery);
        QNetworkAccessManager local;auto* reply=local.get(QNetworkRequest(callback));connect(reply,&QNetworkReply::finished,reply,&QObject::deleteLater);
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(),1,5000);QCOMPARE(data.profileName(),QString("OrbitEly"));QCOMPARE(data.accessToken(),QString("ely-fixture-access"));QVERIFY(!data.minecraftEntitlement.ownsMinecraft);
        QCOMPARE(network.tokenRequests,1);QCOMPARE(network.profileRequests,1);QVERIFY(network.posted.contains("code_verifier="));QVERIFY(!network.posted.contains("client_secret="));
        const auto json=data.saveState();const auto bytes=QJsonDocument(json).toJson();QVERIFY(!bytes.contains("ely-fixture-access"));QVERIFY(!bytes.contains("ely-fixture-refresh"));QVERIFY(!json.contains("ygg"));
        AccountData restored;QVERIFY(restored.resumeStateFromV3(json));QCOMPARE(restored.type,AccountType::Ely);
#ifdef Q_OS_WIN
        QCOMPARE(restored.accessToken(),data.accessToken());QCOMPARE(restored.yggdrasilToken.refresh_token,QString("ely-fixture-refresh"));
#endif
        QVERIFY(!restored.minecraftEntitlement.ownsMinecraft);
    }
    void deviceAuthorizationUsesOfficialBrowser(){
        AccountData data;data.type=AccountType::Ely;ElyTestNetwork network;ElyStep step(&data,&network);
        QUrl url;connect(&step,&ElyStep::authorizeWithBrowser,this,[&](const QUrl& value){url=value;});QSignalSpy done(&step,&AuthStep::finished);step.perform();
        QTRY_VERIFY_WITH_TIMEOUT(url.isValid(),3000);QCOMPARE(url.host(),QString("account.ely.by"));QCOMPARE(url.scheme(),QString("https"));QCOMPARE(url.path(),QString("/code"));QVERIFY(QUrlQuery(url).hasQueryItem("user_code"));
        QTRY_COMPARE_WITH_TIMEOUT(done.count(),1,8000);QCOMPARE(data.profileName(),QString("OrbitEly"));QVERIFY(network.posted.contains("device_code=fixture-device"));QVERIFY(!network.posted.contains("client_secret="));
    }
    void pendingAuthorizationContinuesPolling(){
        AccountData data;ElyTestNetwork network;network.pendingReplies=2;ElyStep step(&data,&network);
        QSignalSpy done(&step,&AuthStep::finished);step.perform();
        QTRY_COMPARE_WITH_TIMEOUT(done.count(),1,12000);
        QCOMPARE(network.tokenRequests,3);QCOMPARE(data.profileName(),QString("OrbitEly"));
    }
    void recoversFromInterruptedPoll(){
        AccountData data;ElyTestNetwork network;network.failedReplies=1;ElyStep step(&data,&network);
        QSignalSpy done(&step,&AuthStep::finished);step.perform();
        QTRY_COMPARE_WITH_TIMEOUT(done.count(),1,10000);
        QCOMPARE(network.tokenRequests,2);QCOMPARE(data.profileName(),QString("OrbitEly"));QVERIFY(network.boundedRequests);
    }
    void stopsAfterRepeatedNetworkFailures(){
        AccountData data;ElyTestNetwork network;network.failedReplies=10;ElyStep step(&data,&network);
        QSignalSpy done(&step,&AuthStep::finished);step.perform();
        QTRY_COMPARE_WITH_TIMEOUT(done.count(),1,12000);
        QCOMPARE(network.tokenRequests,4);QVERIFY(data.accessToken().isEmpty());
    }
    void livePendingPolling(){
        if(qEnvironmentVariable("ORBIT_ELY_LIVE_TEST")!="1")QSKIP("Optional public endpoint check");
        AccountData data;QNetworkAccessManager network;int pending=0;
        connect(&network,&QNetworkAccessManager::finished,this,[&](QNetworkReply* reply){
            if(reply->url().path().endsWith("/token")){
                qInfo()<<"Public pending poll HTTP"<<reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                if(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==400)++pending;
            }
        });
        ElyStep step(&data,&network);step.perform();
        QTRY_VERIFY_WITH_TIMEOUT(pending>=2,45000);step.abort();QVERIFY(data.accessToken().isEmpty());
    }
    void rejectsUntrustedVerificationAddresses(){
        for(const auto& url:{"https://evil.test/code","http://account.ely.by.evil.test/code","https://user@account.ely.by/code","http://account.ely.by:8080/code"}){
            AccountData data;data.type=AccountType::Ely;ElyTestNetwork network;network.verification=QString::fromLatin1(url);ElyStep step(&data,&network);
            QSignalSpy browser(&step,&ElyStep::authorizeWithBrowser),done(&step,&AuthStep::finished);step.perform();
            QTRY_COMPARE_WITH_TIMEOUT(done.count(),1,3000);QCOMPARE(browser.count(),0);QCOMPARE(network.tokenRequests,0);
        }
    }
    void optionalLiveDeviceAddress(){
        if(qEnvironmentVariable("ORBIT_ELY_LIVE_TEST")!="1")QSKIP("Optional official device endpoint check; no sign in");
        AccountData data;data.type=AccountType::Ely;QNetworkAccessManager network;ElyStep step(&data,&network);QUrl url;
        connect(&step,&ElyStep::authorizeWithBrowser,this,[&](const QUrl& value){url=value;step.abort();});step.perform();
        QTRY_VERIFY_WITH_TIMEOUT(url.isValid(),35000);QCOMPARE(url.scheme(),QString("https"));QCOMPARE(url.host(),QString("account.ely.by"));QCOMPARE(url.path(),QString("/code"));QVERIFY(!QUrlQuery(url).queryItemValue("user_code").isEmpty());
    }
    void refreshWithoutBrowser(){
        AccountData data;data.type=AccountType::Ely;data.yggdrasilToken.refresh_token="fixture-old-refresh";
        ElyTestNetwork network;ElyStep step(&data,&network,true);QSignalSpy browser(&step,&ElyStep::authorizeWithBrowser);QSignalSpy done(&step,&AuthStep::finished);
        step.perform();QTRY_COMPARE_WITH_TIMEOUT(done.count(),1,5000);QCOMPARE(browser.count(),0);QCOMPARE(data.accessToken(),QString("ely-fixture-access"));QVERIFY(network.posted.contains("grant_type=refresh_token"));QVERIFY(!network.posted.contains("client_secret="));
    }
    void forgedStateDoesNotExchangeToken(){
        AccountData data;data.type=AccountType::Ely;ElyTestNetwork network;ElyStep step(&data,&network,false,false);QUrl authorization;
        connect(&step,&ElyStep::authorizeWithBrowser,this,[&](const QUrl& url){authorization=url;});step.perform();
        QUrlQuery query(authorization);QUrl callback(query.queryItemValue("redirect_uri"));callback.setQuery("code=fixture&state=incorrect");
        QNetworkAccessManager local;auto* reply=local.get(QNetworkRequest(callback));connect(reply,&QNetworkReply::finished,reply,&QObject::deleteLater);
        QTest::qWait(100);QCOMPARE(network.tokenRequests,0);QVERIFY(data.accessToken().isEmpty());step.abort();
        QTcpServer probe;QVERIFY(probe.listen(QHostAddress::LocalHost,static_cast<quint16>(callback.port())));
    }
    void rejectsInvalidProfileAndUntrustedAgent(){
        AccountData data;QVERIFY(!ElyStep::applyProfile(&data,{{"uuid","bad"},{"username","test"}}));
        QVERIFY(!ElyStep::applyProfile(&data,{{"uuid","ffc8fdc9-5824-509e-8a57-c99b940fb996"},{"username","../unsafe"}}));
        ElyTestNetwork network;ElyStep refresh(&data,&network,true);QSignalSpy done(&refresh,&AuthStep::finished);refresh.perform();QCOMPARE(done.size(),1);QCOMPARE(network.tokenRequests,0);
        QTemporaryDir temp;QFile bad(temp.filePath("fake.jar"));QVERIFY(bad.open(QIODevice::WriteOnly));bad.write("tampered");bad.close();QVERIFY(!ElyAgentCheck::verify(bad.fileName()));
    }
};
QTEST_MAIN(ElyAuthTest)
#include "ElyAuth_test.moc"
