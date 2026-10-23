// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QPainter>
#include <QPushButton>

class OrbitContentTile : public QPushButton {
    QString m_title, m_detail, m_symbol;
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        const bool rtl=layoutDirection()==Qt::RightToLeft;
        const auto alignment=(rtl?Qt::AlignRight:Qt::AlignLeft)|Qt::AlignAbsolute;
        const auto box = QRectF(rect()).adjusted(1,1,-1,-1);
        p.setBrush(isDown() ? QColor(41,51,54,205) : underMouse() ? QColor(32,43,48,175) : QColor(21,31,37,125));
        p.setPen(QPen(hasFocus() ? QColor("#d6e7fc") : QColor("#354046"), hasFocus() ? 2 : 1));
        p.drawRoundedRect(box, 12,12);
        p.setPen(isEnabled() ? QColor("#b7d2f4") : QColor("#6d7775"));
        QFont iconFont = font(); iconFont.setPointSize(22); p.setFont(iconFont);
        p.drawText(QRect(rtl?width()-64:20,14,44,36), Qt::AlignVCenter|alignment, m_symbol);
        QFont titleFont = font(); titleFont.setPointSize(12); titleFont.setBold(true); p.setFont(titleFont);
        p.setPen(isEnabled() ? QColor("#f1f1e9") : QColor("#7f8a87"));
        p.drawText(QRect(20,58,width()-40,26), Qt::AlignVCenter|alignment, m_title);
        QFont detailFont = font(); detailFont.setPointSize(10); detailFont.setBold(false); p.setFont(detailFont);
        p.setPen(QColor("#a2b0ac"));
        p.drawText(QRect(20,90,width()-40,40), alignment|Qt::TextWordWrap, m_detail);
        p.setPen(QColor("#7d8d88"));
        p.drawText(QRect(rtl?20:width()-40,19,20,25),Qt::AlignCenter,rtl?QStringLiteral("↖"):QStringLiteral("↗"));
    }
public:
    OrbitContentTile(const QString& title, const QString& detail, const QString& symbol, QWidget* parent=nullptr)
        : QPushButton(title,parent), m_title(title), m_detail(detail), m_symbol(symbol) {
        setFixedHeight(148); setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
        setCursor(Qt::PointingHandCursor); setFocusPolicy(Qt::StrongFocus);
        setAccessibleName(title + ": " + detail); setToolTip(detail);
    }
};
