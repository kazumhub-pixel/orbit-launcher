/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QEvent>
#include <QListView>
#include <QPainter>
#include <QScrollBar>
#include <QStyledItemDelegate>

class BasePage;
const int pageIconSize = 24;

class PageViewDelegate : public QStyledItemDelegate {
   public:
    PageViewDelegate(QObject* parent) : QStyledItemDelegate(parent) {}
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        if (!parent()->property("orbitNavigation").toBool()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        const auto box = QRectF(option.rect).adjusted(1, 1, -1, -1);
        const bool selected = option.state & QStyle::State_Selected;
        const bool hover = option.state & QStyle::State_MouseOver;
        painter->setBrush(selected ? QColor(170,205,244,28) : hover ? QColor(255,255,255,12) : Qt::transparent);
        painter->setPen(selected ? QPen(QColor(170,205,244,80)) : QPen(Qt::NoPen));
        painter->drawRoundedRect(box, 7, 7);
        if (selected) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor("#aacdf4"));
            painter->drawRoundedRect(QRectF(box.left()+2,box.top()+10,3,box.height()-20),1.5,1.5);
        }
        const auto id = index.data(Qt::UserRole).toString();
        QString symbol = QStringLiteral("◇");
        if ((id == "log" || id == "console")) symbol = QStringLiteral("›_");
        else if (id.contains("mod")) symbol = QStringLiteral("⊞");
        else if (id == "version") symbol = QStringLiteral("◈");
        else if (id.contains("world")) symbol = QStringLiteral("◎");
        else if (id.contains("setting")) symbol = QStringLiteral("✧");
        else if (id.contains("pack")) symbol = QStringLiteral("▧");
        painter->setFont(option.font);
        painter->setPen(selected ? QColor("#c7def8") : QColor("#8fa79c"));
        painter->drawText(option.rect.adjusted(14,0,-170,0), Qt::AlignCenter, symbol);
        painter->setPen(selected ? QColor("#e0edff") : QColor("#b9c8c1"));
        const auto text = option.fontMetrics.elidedText(index.data().toString(), Qt::ElideRight, option.rect.width()-64);
        painter->drawText(option.rect.adjusted(48,0,-10,0), Qt::AlignVCenter|Qt::AlignLeft, text);
        if(option.state & QStyle::State_HasFocus) {
            painter->setPen(QPen(QColor("#e4ce9d"),1,Qt::DashLine));
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(box.adjusted(3,3,-3,-3),5,5);
        }
        painter->restore();
    }
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(qMax(size.height(), parent()->property("orbitNavigation").toBool() ? 42 : 32));
        return size;
    }
};

class PageModel : public QAbstractListModel {
   public:
    PageModel(QObject* parent = 0) : QAbstractListModel(parent)
    {
        QPixmap empty(pageIconSize, pageIconSize);
        empty.fill(Qt::transparent);
        m_emptyIcon = QIcon(empty);
    }
    virtual ~PageModel() {}

    int rowCount(const QModelIndex& parent = QModelIndex()) const { return parent.isValid() ? 0 : m_pages.size(); }
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const
    {
        switch (role) {
            case Qt::UserRole:
                return m_pages.at(index.row())->id();
            case Qt::ToolTipRole:
            case Qt::DisplayRole:
                return m_pages.at(index.row())->displayName();
            case Qt::DecorationRole: {
                QIcon icon = m_pages.at(index.row())->icon();
                if (icon.isNull())
                    icon = m_emptyIcon;
                // HACK: fixes icon stretching on windows. TODO: report Qt bug for this
                return QIcon(icon.pixmap(QSize(48, 48)));
            }
        }
        return QVariant();
    }

    void setPages(const QList<BasePage*>& pages)
    {
        beginResetModel();
        m_pages = pages;
        endResetModel();
    }
    const QList<BasePage*>& pages() const { return m_pages; }

    BasePage* findPageEntryById(QString id)
    {
        for (auto page : m_pages) {
            if (page->id() == id)
                return page;
        }
        return nullptr;
    }

    QList<BasePage*> m_pages;
    QIcon m_emptyIcon;
};

class PageView : public QListView {
   public:
    PageView(QWidget* parent = 0) : QListView(parent)
    {
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
        setItemDelegate(new PageViewDelegate(this));
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    virtual QSize sizeHint() const
    {
        int width = sizeHintForColumn(0) + frameWidth() * 2 + 5;
        if (verticalScrollBar()->isVisible())
            width += verticalScrollBar()->width();
        return QSize(width, 100);
    }

    virtual bool eventFilter(QObject* obj, QEvent* event)
    {
        if (obj == verticalScrollBar() && (event->type() == QEvent::Show || event->type() == QEvent::Hide))
            updateGeometry();
        return QListView::eventFilter(obj, event);
    }
};
