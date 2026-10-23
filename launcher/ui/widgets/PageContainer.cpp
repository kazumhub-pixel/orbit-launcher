// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "PageContainer.h"
#include "BuildConfig.h"
#include "PageContainer_p.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStackedLayout>
#include <QStyledItemDelegate>
#include <QUrl>
#include <utility>

#include "settings/SettingsObject.h"

#include "ui/widgets/IconLabel.h"

#include "Application.h"
#include "DesktopServices.h"

class PageEntryFilterModel : public QSortFilterProxyModel {
   public:
    explicit PageEntryFilterModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

   protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
    {
        const QString pattern = filterRegularExpression().pattern();
        auto* const model = static_cast<PageModel*>(sourceModel());
        auto* const page = model->pages().at(sourceRow);
        if (!page->shouldDisplay()) {
            return false;
        }
        // Regular contents check, then check page-filter.
        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }
};

PageContainer::PageContainer(BasePageProvider* pageProvider, QString defaultId, QWidget* parent)
    : QWidget(parent)
    , m_proxyModel(new PageEntryFilterModel(this))
    , m_model(new PageModel(this))
{
    createUI();
    useSidebarStyle(true);

    int counter = 0;
    auto pages = pageProvider->getPages();
    for (auto* page : pages) {
        auto* widget = dynamic_cast<QWidget*>(page);
        widget->setParent(this);
        page->stackIndex = m_pageStack->addWidget(widget);
        page->listIndex = counter;
        page->setParentContainer(this);
        counter++;
        page->updateExtraInfo = [this](const QString& id, const QString& info) {
            if (m_currentPage && id == m_currentPage->id()) {
                m_header->setText(m_currentPage->displayName() + info);
            }
        };
    }
    m_model->setPages(pages);

    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_pageList->setIconSize(QSize(pageIconSize, pageIconSize));
    m_pageList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pageList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_pageList->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_pageList->setModel(m_proxyModel);
    connect(m_pageList->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &PageContainer::currentChanged);
    m_pageStack->setStackingMode(QStackedLayout::StackOne);
    m_pageList->setFocus();
    selectPage(std::move(defaultId));
}

bool PageContainer::selectPage(QString pageId)
{
    // now find what we want to have selected...
    auto* page = m_model->findPageEntryById(pageId);
    QModelIndex index;
    if (page) {
        index = m_proxyModel->mapFromSource(m_model->index(page->listIndex));
    }
    if (!index.isValid()) {
        index = m_proxyModel->index(0, 0);
    }
    if (index.isValid()) {
        m_pageList->setCurrentIndex(index);
        return true;
    }
    return false;
}

BasePage* PageContainer::getPage(QString pageId)
{
    return m_model->findPageEntryById(pageId);
}

BasePage* PageContainer::selectedPage() const
{
    return m_currentPage;
}

const QList<BasePage*>& PageContainer::getPages() const
{
    return m_model->pages();
}

void PageContainer::refreshContainer()
{
    m_proxyModel->invalidate();
    if (!m_currentPage->shouldDisplay()) {
        auto index = m_proxyModel->index(0, 0);
        if (index.isValid()) {
            m_pageList->setCurrentIndex(index);
        } else {
            // FIXME: unhandled corner case: what to do when there's no page to select?
        }
    }
}

void PageContainer::createUI()
{
    m_pageStack = new QStackedLayout;
    m_pageList = new PageView;
    m_header = new QLabel();

    QFont headerLabelFont = m_header->font();
    headerLabelFont.setBold(true);
    const int pointSize = headerLabelFont.pointSize();
    if (pointSize > 0) {
        headerLabelFont.setPointSize(pointSize + 2);
    }
    m_header->setFont(headerLabelFont);

    auto* headerHLayout = new QHBoxLayout;
    const int leftMargin = APPLICATION->style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
    headerHLayout->addSpacerItem(new QSpacerItem(leftMargin, 0, QSizePolicy::Fixed, QSizePolicy::Ignored));
    headerHLayout->addWidget(m_header);
    headerHLayout->setContentsMargins(0, 6, 0, 0);

    m_pageStack->setContentsMargins(0, 0, 0, 0);
    m_pageStack->addWidget(new QWidget(this));

    m_layout = new QGridLayout;
    m_layout->addLayout(headerHLayout, 0, 1, 1, 1);
    m_layout->addWidget(m_pageList, 0, 0, 3, 1);
    m_layout->addLayout(m_pageStack, 1, 1, 1, 1);
    m_layout->setColumnStretch(1, 4);
    m_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(m_layout);
}

void PageContainer::retranslate()
{
    if (m_currentPage) {
        m_header->setText(m_currentPage->displayName());
    }

    for (auto* page : m_model->pages()) {
        page->retranslate();
    }
}

void PageContainer::addButtons(QWidget* buttons)
{
    m_layout->addWidget(buttons, 2, 1, 1, 2);
}

void PageContainer::addButtons(QLayout* buttons)
{
    m_layout->addLayout(buttons, 2, 1, 1, 2);
}

void PageContainer::useOrbitStyle(const QString& instanceName)
{
    setObjectName("orbitInstancePages");
    setAttribute(Qt::WA_StyledBackground);
    m_pageList->setObjectName("orbitInstanceNavigation");
    m_pageList->setProperty("orbitNavigation", true);
    m_pageList->setFixedWidth(216);
    m_pageList->setSpacing(3);
    m_pageList->setFrameShape(QFrame::NoFrame);
    m_header->setObjectName("orbitInstanceTitle");
    m_header->setContentsMargins(0, 8, 0, 12);
    m_layout->setContentsMargins(20, 18, 24, 18);
    m_layout->setHorizontalSpacing(24);
    m_layout->setVerticalSpacing(14);
    m_layout->removeWidget(m_pageList);
    auto* identity = new QWidget(this);
    identity->setObjectName("orbitInstanceIdentity");
    auto* identityLayout = new QVBoxLayout(identity);
    identityLayout->setContentsMargins(12, 0, 8, 10);
    identityLayout->setSpacing(7);
    auto* brand = new QLabel(QStringLiteral("◎   O R B I T"), identity);
    brand->setObjectName("orbitInstanceBrand");
    auto* name = new QLabel(identity);
    name->setTextFormat(Qt::PlainText);
    name->setText(name->fontMetrics().elidedText(instanceName, Qt::ElideRight, 185));
    name->setToolTip(instanceName);
    name->setObjectName("orbitInstanceName");
    identityLayout->addWidget(brand);
    identityLayout->addWidget(name);
    m_layout->addWidget(identity, 0, 0);
    m_layout->addWidget(m_pageList, 1, 0, 2, 1);
    setStyleSheet(R"(
        #orbitInstancePages { background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #111b21,stop:0.55 #0d151b,stop:1 #141c20); color: #e7ece8; }
        #orbitInstancePages QLabel { background: transparent; color: #c9d1ce; }
        #orbitInstancePages #orbitInstanceBrand { color: #bad6fa; font-size: 17px; font-weight: 600; }
        #orbitInstancePages #orbitInstanceName { color: #98aaa5; font-size: 12px; }
        #orbitInstancePages #orbitInstanceTitle { color: #f3f0e7; font-size: 23px; font-weight: 500; }
        #orbitInstanceNavigation { background: transparent; border: 0; outline: 0; }
        #orbitInstancePages QPushButton, #orbitInstancePages QToolButton { background: #1b272e; color: #dce4df; border: 1px solid #334149; border-radius: 7px; padding: 8px 13px; }
        #orbitInstancePages QPushButton:hover, #orbitInstancePages QToolButton:hover { background: #29373b; border-color: #8f8168; }
        #orbitInstancePages QPushButton:pressed, #orbitInstancePages QToolButton:pressed { background: #35423f; }
        #orbitInstancePages QPushButton:focus, #orbitInstancePages QToolButton:focus { border: 1px solid #c8e0ff; }
        #orbitInstancePages QPushButton:disabled, #orbitInstancePages QToolButton:disabled { color: #657570; background: #131e25; border-color: #263339; }
        #orbitInstancePages #orbitInstanceLaunch { background: #aacdf4; color: #132137; border-color: #aacdf4; font-weight: 600; }
        #orbitInstancePages #orbitInstanceLaunch:hover { background: #ead4ac; }
        #orbitInstancePages #orbitInstanceLaunch:disabled { background: #37423e; color: #82918a; border-color: #37423e; }
        #orbitInstancePages QLineEdit, #orbitInstancePages QSpinBox, #orbitInstancePages QComboBox { background: #101a20; color: #e8ede8; border: 1px solid #35434a; border-radius: 6px; padding: 7px 9px; selection-background-color: #665738; selection-color: #ffffff; }
        #orbitInstancePages QLineEdit:focus { border-color: #abcdf4; }
        #orbitInstancePages QPlainTextEdit, #orbitInstancePages QTextEdit, #orbitInstancePages QTreeView, #orbitInstancePages QTableView { background: #0b1319; alternate-background-color: #111e25; color: #dbe3de; border: 1px solid #2b3a41; border-radius: 8px; selection-background-color: #39483e; selection-color: #f4e2bf; }
        #orbitInstancePages QHeaderView::section { background: #18252c; color: #b7c6bf; border: 0; border-bottom: 1px solid #344249; padding: 8px; }
        #orbitInstancePages #orbitConsoleCaption { color: #9fb1a8; font-size: 10px; font-weight: 600; letter-spacing: 2px; }
        #orbitInstancePages QCheckBox { color: #b7c7c0; spacing: 7px; padding: 5px 0; }
        #orbitInstancePages QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid #60736b; border-radius: 4px; background: #152128; }
        #orbitInstancePages QCheckBox::indicator:checked { background: #b4d1f4; border-color: #d2e6ff; }
        #orbitInstancePages QCheckBox::indicator:disabled { background: #29342f; border-color: #43534c; }
        #orbitInstancePages QGroupBox { border: 1px solid #314048; border-radius: 8px; margin-top: 14px; padding: 14px 8px 8px; color: #d1dcd6; }
        #orbitInstancePages QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }
    )");
}

void PageContainer::useSidebarStyle(bool sidebar)
{
    m_pageList->setProperty("_kde_side_panel_view", sidebar);
}

void PageContainer::showPage(int row)
{
    if (m_currentPage) {
        m_currentPage->closed();
    }
    if (row != -1) {
        m_currentPage = m_model->pages().at(row);
    } else {
        m_currentPage = nullptr;
    }
    if (m_currentPage) {
        m_pageStack->setCurrentIndex(m_currentPage->stackIndex);
        m_header->setText(m_currentPage->displayName());
        m_currentPage->opened();
    } else {
        m_pageStack->setCurrentIndex(0);
        m_header->setText(QString());
    }
}

void PageContainer::help()
{
    if (m_currentPage) {
        QString pageId = m_currentPage->helpPage();
        if (pageId.isEmpty()) {
            return;
        }
        DesktopServices::openUrl(QUrl(BuildConfig.HELP_URL.arg(pageId)));
    }
}

void PageContainer::currentChanged(const QModelIndex& current)
{
    int selectedIndex = current.isValid() ? m_proxyModel->mapToSource(current).row() : -1;

    auto* selected = m_model->pages().at(selectedIndex);
    auto* previous = m_currentPage;

    emit selectedPageChanged(previous, selected);

    showPage(selectedIndex);
}

bool PageContainer::prepareToClose()
{
    if (!saveAll()) {
        return false;
    }
    if (m_currentPage) {
        m_currentPage->closed();
    }
    return true;
}

bool PageContainer::saveAll()
{
    for (auto* page : m_model->pages()) {
        if (!page->apply()) {
            return false;
        }
    }
    return true;
}

void PageContainer::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
    QWidget::changeEvent(event);
}
