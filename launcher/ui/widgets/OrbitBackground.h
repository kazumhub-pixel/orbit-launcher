// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QFileInfo>
#include <QHideEvent>
#include <QMediaPlayer>
#include <QPainter>
#include <QShowEvent>
#include <QTimer>
#include <QVideoFrame>
#include <QVideoSink>
#include <functional>

class OrbitScene : public QWidget {
    QPixmap m_image{":/orbit/landscape.jpg"};
    QImage m_frame;
    QMediaPlayer* m_player = nullptr;
    QVideoSink* m_sink = nullptr;
    QString m_video;
    bool m_suspended = false;
    bool m_failed = false;
    void unloadVideo() {
        if (m_player) { m_player->stop(); m_player->setSource({}); }
        m_frame = {};
        update();
    }
    void resumeVideo() {
        if (m_video.isEmpty() || m_suspended || m_failed || !isVisible()) return;
        if (!m_player) {
            m_player = new QMediaPlayer(this);
            m_sink = new QVideoSink(this);
            m_player->setVideoSink(m_sink);
            m_player->setLoops(QMediaPlayer::Infinite);
            // No audio output: backgrounds never play sound.
            connect(m_sink, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame& frame) {
                if (m_suspended || !isVisible()) return;
                const auto image = frame.toImage();
                if (!image.isNull()) { m_frame = image; ++m_frames; update(); }
            });
            connect(m_player, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString& error) {
                m_failed = true;
                if (onError) onError(error);
                QTimer::singleShot(0, this, [this] { unloadVideo(); });
            });
        }
        if (m_player->source().isEmpty()) m_player->setSource(QUrl::fromLocalFile(m_video));
        m_player->play();
    }
protected:
    void hideEvent(QHideEvent* event) override { unloadVideo(); QWidget::hideEvent(event); }
    void showEvent(QShowEvent* event) override { QWidget::showEvent(event); resumeVideo(); }
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.fillRect(rect(), QColor("#090e13"));
        const QSize original = m_frame.isNull() ? m_image.size() : m_frame.size();
        if (!original.isEmpty()) {
            const QSize scaled = original.scaled(size(), Qt::KeepAspectRatioByExpanding);
            const QRect target((width()-scaled.width())/2, (height()-scaled.height())/2, scaled.width(), scaled.height());
            if (m_frame.isNull()) p.drawPixmap(target, m_image); else p.drawImage(target, m_frame);
        }
        QLinearGradient shade(0, 0, width(), height());
        shade.setColorAt(0, QColor(10,18,31,195)); shade.setColorAt(.55, QColor(10,18,31,70));
        shade.setColorAt(1, QColor(7,13,20,205)); p.fillRect(rect(), shade);
        QLinearGradient edges(0,0,0,height());
        edges.setColorAt(0,QColor(8,15,26,175)); edges.setColorAt(.25,Qt::transparent);
        edges.setColorAt(.65,Qt::transparent); edges.setColorAt(1,QColor(5,10,14,125));
        p.fillRect(rect(), edges);
    }
public:
    std::function<void(const QString&)> onError;
    quint64 m_frames = 0;
    explicit OrbitScene(QWidget* parent = nullptr) : QWidget(parent) {}
    bool setBackground(const QString& path) {
        QPixmap image(path);
        if (image.isNull()) return false;
        m_video.clear(); unloadVideo(); m_image = image; update(); return true;
    }
    bool setVideo(const QString& file) {
        QFileInfo info(file);
        if (!info.isFile() || !info.isReadable()) return false;
        unloadVideo(); m_video = info.absoluteFilePath(); m_failed = false; resumeVideo(); return true;
    }
    void setSuspended(bool value) { m_suspended = value; if (value) unloadVideo(); else resumeVideo(); }
    bool isSuspended() const { return m_suspended; }
    bool decoderIdle() const { return !m_player || m_player->source().isEmpty(); }
    bool hasVideo() const { return !m_video.isEmpty(); }
};
