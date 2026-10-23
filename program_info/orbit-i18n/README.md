# Orbit translations

The launcher uses Qt QTranslator throughout. No online translation service or
second localization engine is used. The original application Language setting
is also used by Themes > Interface language. Selecting a language persists it;
restart Orbit to apply it consistently to every existing window.

Supported bundled Orbit catalogs: en_US, pt_BR, ru, zh (Simplified Chinese), es,
ar. Arabic uses Qt right-to-left layout, including custom painted content tiles.
Windows Chinese text uses the installed Microsoft YaHei TTC when the default
theme font is Segoe UI. Custom theme font choices remain user-controlled.

`catalog.tsv` contains PT-BR source, English, Russian, Simplified Chinese,
Spanish and Arabic in that order. Literal `\n` becomes a newline. `%1` and file
filters must be preserved. `scripts/orbit-translations.py` validates rows,
placeholders, filters and duplicate sources before producing native Qt TS files.
The instance count uses Qt numerus forms (including Russian and Arabic).

To rebuild Orbit catalogs, run the Python script from any working directory,
then use Qt lrelease on each orbit_*.ts file. The resulting orbit_*.qm files are
embedded by program_info/AuroraLauncher.qrc.in. Keep generated TS/QM files in
the source package so end-user builds do not need Python or Qt Linguist tools.

`prism-source` contains GPL source catalogs and upstream license at the commit
recorded in ORIGIN.json. Compile these with lrelease to mmc_<language>.qm.
The base retains its existing translation download/update behavior. Missing or
unfinished upstream strings fall back to English. Language completeness in
advanced settings is distinct from the complete Orbit-specific catalogs.

`qt-source` contains corresponding qtbase TS files from Qt v6.12.0.
`qt-licenses` preserves the package's license notices. Compile zh_CN as
qtbase_zh.qm for the launcher language key. The other names match their keys.

Tests: OrbitI18n loads every embedded catalog, checks placeholders and plurals,
exercises selection callbacks and renders shell/profile/theme previews in all
six languages. ORBIT_I18N_GALLERY sets an optional output folder. Automated
checks do not replace review by native speakers.
