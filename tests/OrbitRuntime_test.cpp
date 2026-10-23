// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include "ui/widgets/OrbitBackground.h"
#include "ui/widgets/OrbitEcoController.h"

class OrbitRuntimeTest : public QObject {
    Q_OBJECT
private slots:
    void sleepsUntilLastGameFinishes() {
        QWidget window; window.show();
        OrbitEcoController controller(&window);
        bool suspended=false;
        controller.enabled=[] { return true; };
        controller.suspendMedia=[&](bool value){ suspended=value; };
        controller.gameState("a",false);
        QVERIFY(window.isVisible());
        controller.gameState("a",true);
        QVERIFY(controller.sleeping()); QVERIFY(suspended); QVERIFY(!window.isVisible());
        controller.gameState("a",true); // repeated notification must not double count
        controller.gameState("b",true);
        controller.gameState("a",false);
        QVERIFY(controller.sleeping());
        controller.gameState("b",false);
        QVERIFY(!controller.sleeping()); QVERIFY(!suspended); QVERIFY(window.isVisible());
        controller.gameState("b",false);
        QVERIFY(window.isVisible());
    }
    void respectsDisabledModeAndPreviouslyHiddenWindow() {
        QWidget window;window.show();
        OrbitEcoController controller(&window);
        controller.enabled=[] { return false; };
        controller.gameState("a",true);QVERIFY(window.isVisible());QVERIFY(!controller.sleeping());
        controller.gameState("a",false);
        window.hide();controller.enabled=[] { return true; };
        controller.gameState("a",true);controller.gameState("a",false);
        QVERIFY(!window.isVisible());
    }
    void rejectsMissingFilesWithoutStartingDecoder() {
        OrbitScene scene;
        QVERIFY(!scene.setVideo("this-file-does-not-exist.mp4"));
        QVERIFY(!scene.hasVideo());QVERIFY(scene.decoderIdle());
        QVERIFY(!scene.setBackground("missing-image.png"));
    }
    void decodesLoopsAndReleasesOnHide() {
        const auto path=qEnvironmentVariable("ORBIT_TEST_VIDEO");
        if(path.isEmpty())QSKIP("Set ORBIT_TEST_VIDEO to a local synthetic MP4 fixture");
        QVERIFY(QFileInfo::exists(path));
        OrbitScene scene;scene.resize(320,180);scene.show();
        QString error;scene.onError=[&](const QString& value){error=value;};
        QVERIFY(scene.setVideo(path));
        QTRY_VERIFY_WITH_TIMEOUT(scene.m_frames>2 || !error.isEmpty(),10000);
        QVERIFY2(error.isEmpty(),qPrintable(error));
        QVERIFY(scene.m_frames>2);
        QTRY_VERIFY_WITH_TIMEOUT(scene.m_frames>16,10000); // crosses the one-second loop boundary
        scene.setSuspended(true);QVERIFY(scene.decoderIdle());
        const auto frames=scene.m_frames;QTest::qWait(250);QCOMPARE(scene.m_frames,frames);
        scene.setSuspended(false);
        QTRY_VERIFY_WITH_TIMEOUT(scene.m_frames>frames,10000);
        scene.hide();QVERIFY(scene.decoderIdle());
    }
};
QTEST_MAIN(OrbitRuntimeTest)
#include "OrbitRuntime_test.moc"
