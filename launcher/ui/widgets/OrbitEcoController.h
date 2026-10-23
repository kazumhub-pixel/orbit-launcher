// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QObject>
#include <QSet>
#include <QWidget>
#include <functional>

// The game process stays under normal launcher supervision. Only the UI sleeps.
class OrbitEcoController : public QObject {
    QWidget* m_window;
    QSet<QString> m_games;
    bool m_sleeping = false;
    bool m_restore = false;
public:
    std::function<bool()> enabled;
    std::function<void(bool)> suspendMedia;
    explicit OrbitEcoController(QWidget* window) : QObject(window), m_window(window) {}
    bool sleeping() const { return m_sleeping; }
    void gameState(const QString& id, bool active) {
        if (active) m_games.insert(id); else m_games.remove(id);
        if (!m_games.isEmpty() && !m_sleeping && enabled && enabled()) {
            m_restore = m_window->isVisible();
            m_sleeping = true;
            if (suspendMedia) suspendMedia(true);
            m_window->hide();
        } else if (m_games.isEmpty() && m_sleeping) {
            m_sleeping = false;
            if (suspendMedia) suspendMedia(false);
            if (m_restore) { m_window->show(); m_window->raise(); }
            m_restore = false;
        }
    }
};
