// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "translations/OrbitI18n.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QUuid>
#include <algorithm>
#include "FusionTheme.h"

struct AuroraStyle {
    QString name = "Orbit personalizado";
    QColor background{ "#10141d" }, surface{ "#1a2130" }, foreground{ "#edf2fa" }, accent{ "#aacdf4" }, border{ "#323d51" };
    QString fontFamily = "Segoe UI";
    int fontSize = 10, radius = 8, spacing = 8;

    QJsonObject tokens() const
    {
        return { { "name", name },
                 { "background", background.name() },
                 { "surface", surface.name() },
                 { "foreground", foreground.name() },
                 { "accent", accent.name() },
                 { "border", border.name() },
                 { "fontFamily", fontFamily },
                 { "fontSize", fontSize },
                 { "radius", radius },
                 { "spacing", spacing } };
    }
    QJsonObject package() const { return { { "format", "aurora-theme" }, { "version", 1 }, { "tokens", tokens() } }; }
    static bool parse(const QJsonObject& root, AuroraStyle& out, QString& error)
    {
        if (root.value("format").toString() != "aurora-theme" || root.value("version").toInt() != 1 || !root.value("tokens").isObject()) {
            error = OrbitI18n::tr("Formato de tema Aurora desconhecido.");
            return false;
        }
        const auto t = root.value("tokens").toObject();
        AuroraStyle s;
        s.name = t.value("name").toString().trimmed();
        s.fontFamily = t.value("fontFamily").toString().trimmed();
        s.fontSize = t.value("fontSize").toInt(-1);
        s.radius = t.value("radius").toInt(-1);
        s.spacing = t.value("spacing").toInt(-1);
        s.background = QColor(t.value("background").toString());
        s.surface = QColor(t.value("surface").toString());
        s.foreground = QColor(t.value("foreground").toString());
        s.accent = QColor(t.value("accent").toString());
        s.border = QColor(t.value("border").toString());
        if (s.name.isEmpty() || s.name.size() > 80 || s.fontFamily.isEmpty() || s.fontFamily.size() > 128 || s.fontSize < 9 ||
            s.fontSize > 16 || s.radius < 0 || s.radius > 20 || s.spacing < 4 || s.spacing > 16 || !s.background.isValid() ||
            !s.surface.isValid() || !s.foreground.isValid() || !s.accent.isValid() || !s.border.isValid()) {
            error = OrbitI18n::tr("Tema inválido: confira nome, cores, fonte e limites dos controles.");
            return false;
        }
        out = s;
        return true;
    }
    QColor accentText() const
    {
        return (accent.redF() * .299 + accent.greenF() * .587 + accent.blueF() * .114) > .57 ? QColor("#10141d") : QColor("#ffffff");
    }
    QPalette palette() const
    {
        QPalette p;
        p.setColor(QPalette::Window, background);
        p.setColor(QPalette::WindowText, foreground);
        p.setColor(QPalette::Base, background);
        p.setColor(QPalette::AlternateBase, surface);
        p.setColor(QPalette::Text, foreground);
        p.setColor(QPalette::Button, surface);
        p.setColor(QPalette::ButtonText, foreground);
        p.setColor(QPalette::ToolTipBase, surface);
        p.setColor(QPalette::ToolTipText, foreground);
        p.setColor(QPalette::Highlight, accent);
        p.setColor(QPalette::HighlightedText, accentText());
        p.setColor(QPalette::Link, accent);
        p.setColor(QPalette::BrightText, QColor("#f4758c"));
        p.setColor(QPalette::PlaceholderText, foreground.darker(150));
        for (auto role : { QPalette::Text, QPalette::WindowText, QPalette::ButtonText })
            p.setColor(QPalette::Disabled, role,
                       QColor::fromRgbF((foreground.redF() + background.redF()) / 2, (foreground.greenF() + background.greenF()) / 2,
                                        (foreground.blueF() + background.blueF()) / 2));
        return p;
    }
    QString qss() const
    {
        QString font = fontFamily;
        if(OrbitI18n::activeLanguage()=="zh" && font=="Segoe UI")font=OrbitI18n::chineseFontFamily();
        font.replace('\\', "\\\\").replace('"', "\\\"").replace('\n', ' ').replace('\r', ' ');
        return QString(R"QSS(
QWidget { font-family: "%1"; font-size: %2pt; }
QMainWindow, QDialog { background: %3; }
QToolBar { border: none; spacing: 6px; padding: 8px; background: %4; }
QPushButton, QToolButton { border: 1px solid %5; border-radius: %6px; padding: %7px 12px; background: %4; color: %8; }
QPushButton:hover, QToolButton:hover { border-color: %9; }
QPushButton:pressed, QToolButton:pressed, QToolButton:checked { background: %9; color: %10; }
QPushButton:focus, QToolButton:focus { border: 2px solid %9; }
QPushButton:disabled, QToolButton:disabled { color: palette(disabled, button-text); }
QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox { border: 1px solid %5; border-radius: %6px; padding: %7px; background: %3; color: %8; selection-background-color: %9; selection-color: %10; }
QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border-color: %9; }
QAbstractItemView { border: 1px solid %5; border-radius: %6px; padding: 6px; background: %3; alternate-background-color: %4; selection-background-color: %9; selection-color: %10; }
QAbstractItemView::item { padding: 5px; }
QTabWidget::pane { border: 1px solid %5; border-radius: %6px; }
QTabBar::tab { background: %4; padding: 9px 15px; border-bottom: 2px solid transparent; }
QTabBar::tab:selected { border-bottom-color: %9; }
QGroupBox { border: 1px solid %5; border-radius: %6px; margin-top: 14px; padding: 12px; }
QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 5px; }
QMenu { background: %4; color: %8; border: 1px solid %5; padding: 5px; }
QMenu::item { padding: 7px 18px; } QMenu::item:selected { background: %9; color: %10; }
QHeaderView::section { background: %4; border: none; padding: 8px; }
QProgressBar { border: 1px solid %5; border-radius: %6px; text-align: center; min-height: 14px; }
QProgressBar::chunk { background: %9; border-radius: %6px; }
QToolTip { background: %4; color: %8; border: 1px solid %5; padding: 6px; }
)QSS")
            .arg(font)
            .arg(fontSize)
            .arg(background.name(), surface.name(), border.name())
            .arg(radius)
            .arg(spacing)
            .arg(foreground.name(), accent.name(), accentText().name());
    }
    QJsonObject manifest() const
    {
        const auto p = palette();
        QJsonObject colors;
        for (const auto& pair :
             { qMakePair("Window", QPalette::Window), qMakePair("WindowText", QPalette::WindowText), qMakePair("Base", QPalette::Base),
               qMakePair("AlternateBase", QPalette::AlternateBase), qMakePair("Text", QPalette::Text),
               qMakePair("Button", QPalette::Button), qMakePair("ButtonText", QPalette::ButtonText),
               qMakePair("ToolTipBase", QPalette::ToolTipBase), qMakePair("ToolTipText", QPalette::ToolTipText),
               qMakePair("BrightText", QPalette::BrightText), qMakePair("Link", QPalette::Link),
               qMakePair("Highlight", QPalette::Highlight), qMakePair("HighlightedText", QPalette::HighlightedText) })
            colors.insert(QString::fromLatin1(pair.first), p.color(pair.second).name());
        colors.insert("fadeColor", background.name());
        colors.insert("fadeAmount", .5);
        return { { "name", name }, { "widgets", "Fusion" }, { "qssFilePath", "themeStyle.css" }, { "colors", colors } };
    }
    static bool write(const QString& path, const QByteArray& data, QString& error)
    {
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
            error = file.errorString();
            return false;
        }
        return true;
    }
    bool install(const QDir& root, QString& id, QString& error) const
    {
        if (!QDir().mkpath(root.absolutePath())) {
            error = OrbitI18n::tr("Não foi possível criar a pasta de temas.");
            return false;
        }
        QTemporaryDir temp(root.absoluteFilePath(".aurora-XXXXXX"));
        if (!temp.isValid()) {
            error = temp.errorString();
            return false;
        }
        if (!write(temp.filePath("theme.json"), QJsonDocument(manifest()).toJson(), error) ||
            !write(temp.filePath("themeStyle.css"), qss().toUtf8(), error) ||
            !write(temp.filePath("aurora.json"), QJsonDocument(package()).toJson(), error))
            return false;
        const QString newId = "aurora-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (!QDir().rename(temp.path(), root.absoluteFilePath(newId))) {
            error = OrbitI18n::tr("Não foi possível finalizar a instalação do tema.");
            return false;
        }
        temp.setAutoRemove(false);
        id = newId;
        return true;
    }
    static AuroraStyle preset(int index)
    {
        AuroraStyle s;
        if (index == 1) {
            s.name = OrbitI18n::tr("Orbit Light");
            s.background = QColor("#f4f6fa");
            s.surface = QColor("#ffffff");
            s.foreground = QColor("#18243a");
            s.accent = QColor("#6350cc");
            s.border = QColor("#d4dce8");
        } else if (index == 2) {
            s.name = OrbitI18n::tr("Orbit Forest");
            s.background = QColor("#101c19");
            s.surface = QColor("#192c26");
            s.foreground = QColor("#e6f4ed");
            s.accent = QColor("#70d8aa");
            s.border = QColor("#345348");
        } else
            s.name = OrbitI18n::tr("Orbit Dark");
        return s;
    }
};

class AuroraTheme final : public FusionTheme {
    AuroraStyle m_style;
    QString m_id;

   public:
    explicit AuroraTheme(int index) : m_style(AuroraStyle::preset(index)), m_id(QString("aurora-%1").arg(index)) {}
    QString id() override { return m_id; }
    QString name() override { return m_style.name; }
    QString tooltip() override { return "Tema nativo Orbit; personalize em Aparência."; }
    bool hasStyleSheet() override { return true; }
    QString appStyleSheet() override { return m_style.qss(); }
    QPalette colorScheme() override { return m_style.palette(); }
    QColor fadeColor() override { return m_style.background; }
    double fadeAmount() override { return .5; }
};
