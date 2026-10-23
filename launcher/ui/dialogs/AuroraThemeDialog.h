// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "translations/OrbitI18n.h"
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>
#include "ui/themes/AuroraTheme.h"

class AuroraThemeDialog final : public QDialog {
    AuroraStyle m_style;
    QLineEdit* m_name;
    QFontComboBox* m_font;
    QSpinBox *m_size, *m_radius, *m_spacing;
    QWidget* m_preview;
    QList<QPushButton*> m_colors;
    QString m_themeId;
    QDir m_folder;
    QColor& color(int i)
    {
        switch (i) {
            case 0:
                return m_style.background;
            case 1:
                return m_style.surface;
            case 2:
                return m_style.foreground;
            case 3:
                return m_style.accent;
            default:
                return m_style.border;
        }
    }
    void readControls()
    {
        m_style.name = m_name->text().trimmed();
        m_style.fontFamily = m_font->currentFont().family();
        m_style.fontSize = m_size->value();
        m_style.radius = m_radius->value();
        m_style.spacing = m_spacing->value();
    }
    void updatePreview()
    {
        readControls();
        m_preview->setPalette(m_style.palette());
        m_preview->setAutoFillBackground(true);
        m_preview->setStyleSheet(m_style.qss());
        for (int i = 0; i < m_colors.size(); i++)
            m_colors[i]->setText(color(i).name());
    }
    void loadControls()
    {
        const QSignalBlocker a(m_name), b(m_font), c(m_size), d(m_radius), e(m_spacing);
        m_name->setText(m_style.name);
        m_font->setCurrentFont(QFont(m_style.fontFamily));
        m_size->setValue(m_style.fontSize);
        m_radius->setValue(m_style.radius);
        m_spacing->setValue(m_style.spacing);
        updatePreview();
    }
    bool valid()
    {
        readControls();
        AuroraStyle check;
        QString error;
        if (!AuroraStyle::parse(m_style.package(), check, error)) {
            QMessageBox::warning(this, OrbitI18n::tr("Tema"), error);
            return false;
        }
        return true;
    }
    void importTheme()
    {
        const auto path = QFileDialog::getOpenFileName(this, OrbitI18n::tr("Importar tema Aurora"), {}, OrbitI18n::tr("Temas Aurora (*.json)"));
        if (path.isEmpty())
            return;
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly) || f.size() > 65536) {
            QMessageBox::warning(this, OrbitI18n::tr("Tema"), OrbitI18n::tr("Não foi possível ler o arquivo ou ele excede 64 KB."));
            return;
        }
        QJsonParseError parseError;
        const auto doc = QJsonDocument::fromJson(f.readAll(), &parseError);
        AuroraStyle incoming;
        QString error;
        if (parseError.error != QJsonParseError::NoError || !doc.isObject() || !AuroraStyle::parse(doc.object(), incoming, error)) {
            QMessageBox::warning(this, OrbitI18n::tr("Tema"), error.isEmpty() ? OrbitI18n::tr("JSON inválido.") : error);
            return;
        }
        m_style = incoming;
        loadControls();
    }
    void exportTheme()
    {
        if (!valid())
            return;
        const auto path = QFileDialog::getSaveFileName(this, OrbitI18n::tr("Exportar tema Aurora"), "meu-tema.aurora.json", OrbitI18n::tr("Temas Aurora (*.json)"));
        if (path.isEmpty())
            return;
        QString error;
        if (!AuroraStyle::write(path, QJsonDocument(m_style.package()).toJson(), error))
            QMessageBox::warning(this, OrbitI18n::tr("Tema"), error);
    }
    void saveTheme()
    {
        if (!valid())
            return;
        QString error;
        if (!m_style.install(m_folder, m_themeId, error)) {
            QMessageBox::warning(this, OrbitI18n::tr("Tema"), error);
            return;
        }
        accept();
    }

   public:
    explicit AuroraThemeDialog(const QDir& folder, QWidget* parent = nullptr) : QDialog(parent), m_folder(folder)
    {
        setWindowTitle(OrbitI18n::tr("Aurora • Personalizar aparência"));
        resize(760, 560);
        auto* root = new QVBoxLayout(this);
        auto* intro = new QLabel(OrbitI18n::tr("Seu launcher, do seu jeito. Crie, importe e compartilhe temas sem alterar suas instâncias."), this);
        intro->setWordWrap(true);
        root->addWidget(intro);
        auto* columns = new QHBoxLayout;
        root->addLayout(columns, 1);
        auto* form = new QFormLayout;
        columns->addLayout(form, 1);
        auto* preset = new QComboBox(this);
        preset->addItems({ OrbitI18n::tr("Orbit Dark"), OrbitI18n::tr("Orbit Light"), OrbitI18n::tr("Orbit Forest") });
        form->addRow(OrbitI18n::tr("Ponto de partida"), preset);
        m_name = new QLineEdit(this);
        m_name->setMaxLength(80);
        form->addRow(OrbitI18n::tr("Nome do tema"), m_name);
        for (const auto& label : { OrbitI18n::tr("Fundo"), OrbitI18n::tr("Superfície"), OrbitI18n::tr("Texto"), OrbitI18n::tr("Destaque"), OrbitI18n::tr("Bordas") }) {
            const int i = m_colors.size();
            auto* button = new QPushButton(this);
            m_colors.append(button);
            form->addRow(label, button);
            connect(button, &QPushButton::clicked, this, [this, i] {
                const auto value = QColorDialog::getColor(color(i), this, OrbitI18n::tr("Escolha uma cor"));
                if (value.isValid()) {
                    color(i) = value;
                    updatePreview();
                }
            });
        }
        m_font = new QFontComboBox(this);
        form->addRow(OrbitI18n::tr("Fonte"), m_font);
        m_size = new QSpinBox(this);
        m_size->setRange(9, 16);
        form->addRow(OrbitI18n::tr("Tamanho da fonte"), m_size);
        m_radius = new QSpinBox(this);
        m_radius->setRange(0, 20);
        form->addRow(OrbitI18n::tr("Cantos"), m_radius);
        m_spacing = new QSpinBox(this);
        m_spacing->setRange(4, 16);
        form->addRow(OrbitI18n::tr("Espaçamento"), m_spacing);
        auto* preview = new QGroupBox(OrbitI18n::tr("Prévia"), this);
        m_preview = preview;
        columns->addWidget(preview, 1);
        auto* sample = new QVBoxLayout(preview);
        auto* heading = new QLabel(OrbitI18n::tr("Sua próxima aventura"), preview);
        sample->addWidget(heading);
        auto* search = new QLineEdit(preview);
        search->setPlaceholderText(OrbitI18n::tr("Buscar instâncias…"));
        sample->addWidget(search);
        auto* list = new QListWidget(preview);
        list->addItems({ OrbitI18n::tr("Survival com amigos"), "Fabric • 1.21", OrbitI18n::tr("Criativo • Vanilla") });
        list->setCurrentRow(0);
        sample->addWidget(list, 1);
        sample->addWidget(new QPushButton(OrbitI18n::tr("Jogar"), preview));
        auto* tip = new QLabel(OrbitI18n::tr("As alterações são aplicadas ao salvar. Cancelar mantém o tema atual."), preview);
        tip->setWordWrap(true);
        sample->addWidget(tip);
        auto* tools = new QHBoxLayout;
        root->addLayout(tools);
        auto* importButton = new QPushButton(OrbitI18n::tr("Importar…"), this);
        auto* exportButton = new QPushButton(OrbitI18n::tr("Exportar…"), this);
        tools->addWidget(importButton);
        tools->addWidget(exportButton);
        tools->addStretch();
        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
        root->addWidget(buttons);
        buttons->button(QDialogButtonBox::Save)->setText(OrbitI18n::tr("Salvar e aplicar"));
        buttons->button(QDialogButtonBox::Cancel)->setText(OrbitI18n::tr("Cancelar"));
        connect(buttons, &QDialogButtonBox::accepted, this, [this] { saveTheme(); });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(importButton, &QPushButton::clicked, this, [this] { importTheme(); });
        connect(exportButton, &QPushButton::clicked, this, [this] { exportTheme(); });
        connect(preset, &QComboBox::currentIndexChanged, this, [this](int i) {
            m_style = AuroraStyle::preset(i);
            loadControls();
        });
        connect(m_name, &QLineEdit::textChanged, this, [this] { updatePreview(); });
        connect(m_font, &QFontComboBox::currentFontChanged, this, [this] { updatePreview(); });
        for (auto* control : { m_size, m_radius, m_spacing })
            connect(control, &QSpinBox::valueChanged, this, [this] { updatePreview(); });
        m_style = AuroraStyle::preset(0);
        loadControls();
    }
    QString themeId() const { return m_themeId; }
};
