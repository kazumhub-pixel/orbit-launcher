// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QCoreApplication>
#include <QFontDatabase>
#include <QDir>
#include <QStandardPaths>
#include <QLocale>
#include <QString>
#include <QTranslator>

namespace OrbitI18n {
inline QString& activeLanguage(){static QString language="en_US";return language;}
inline QString tr(const char* source) { return QCoreApplication::translate("Orbit",source); }
inline QString canonical(const QString& language) {
    const auto locale=QLocale(language);
    switch(locale.language()) {
    case QLocale::Portuguese:return "pt_BR";
    case QLocale::Russian:return "ru";
    case QLocale::Chinese:return "zh";
    case QLocale::Spanish:return "es";
    case QLocale::Arabic:return "ar";
    default:return "en_US";
    }
}
inline QString chineseFontFamily() {
    static const QString family=[] {
#ifdef Q_OS_WIN
        // The offscreen Qt font backend may omit Windows TTC families during discovery.
        auto path=QStandardPaths::locate(QStandardPaths::FontsLocation,"msyh.ttc");
        if(path.isEmpty())path=QDir(qEnvironmentVariable("WINDIR")).filePath("Fonts/msyh.ttc");
        const int id=QFontDatabase::addApplicationFont(path);
        const auto names=QFontDatabase::applicationFontFamilies(id);
        if(!names.isEmpty())return names.first();
#endif
        for(const auto& name:{QStringLiteral("Microsoft YaHei UI"),QStringLiteral("Noto Sans CJK SC"),QStringLiteral("Noto Sans SC")})
            if(QFontDatabase::families().contains(name))return name;
        return QStringLiteral("Sans Serif");
    }();
    return family;
}
inline bool load(QTranslator& translator,const QString& language) {
    const auto code=canonical(language);
    const bool loaded=translator.load(":/orbit/i18n/orbit_"+code+".qm");
    if(loaded)activeLanguage()=code;
    return loaded;
}
}
