// Copyright 2026 Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QDesktopServices>
#include <QDir>
#include <QHeaderView>
#include <QSettings>
#include <QTableWidgetItem>
#include <QUrl>

#include "citra_qt/plugins/plugin_dialog.h"
#include "core/plugins/plugin_manager.h"
#include "ui_plugin_dialog.h"

PluginDialog::PluginDialog(QWidget* parent)
    : QDialog(parent), ui(std::make_unique<Ui::PluginDialog>()) {
    ui->setupUi(this);

    ui->tablePlugins->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->tablePlugins->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->tablePlugins->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->tablePlugins->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);

    connect(ui->buttonOpenFolder, &QPushButton::clicked, this, &PluginDialog::OnOpenPluginsFolder);
    connect(ui->buttonReload, &QPushButton::clicked, this, &PluginDialog::OnReloadPlugins);
    connect(ui->tablePlugins, &QTableWidget::itemChanged, this, &PluginDialog::OnItemChanged);

    LoadSettings();
    PopulatePluginList();
}

PluginDialog::~PluginDialog() = default;

void PluginDialog::LoadSettings() {
    QSettings settings;
    settings.beginGroup(QStringLiteral("plugins"));
    auto& manager = Plugins::PluginManager::GetInstance();
    const auto plugins = manager.GetPlugins();
    for (const auto& plugin : plugins) {
        QString key = QString::fromStdString(plugin.filename);
        if (settings.contains(key)) {
            bool enabled = settings.value(key, false).toBool();
            manager.SetPluginEnabled(plugin.filename, enabled);
        }
    }
    settings.endGroup();
}

void PluginDialog::SaveSettings() {
    QSettings settings;
    settings.beginGroup(QStringLiteral("plugins"));
    const auto plugins = Plugins::PluginManager::GetInstance().GetPlugins();
    for (const auto& plugin : plugins) {
        settings.setValue(QString::fromStdString(plugin.filename), plugin.is_enabled);
    }
    settings.endGroup();
}

void PluginDialog::PopulatePluginList() {
    populating = true;
    ui->tablePlugins->setRowCount(0);

    const auto plugins = Plugins::PluginManager::GetInstance().GetPlugins();
    ui->tablePlugins->setRowCount(static_cast<int>(plugins.size()));

    for (int row = 0; row < static_cast<int>(plugins.size()); ++row) {
        const auto& plugin = plugins[row];

        // Column 0: Status Checkbox
        auto* status_item = new QTableWidgetItem(plugin.is_enabled ? tr("Enabled") : tr("Disabled"));
        status_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
        status_item->setCheckState(plugin.is_enabled ? Qt::Checked : Qt::Unchecked);
        status_item->setData(Qt::UserRole, QString::fromStdString(plugin.filename));
        ui->tablePlugins->setItem(row, 0, status_item);

        // Column 1: Name
        auto* name_item = new QTableWidgetItem(QString::fromStdString(plugin.name));
        name_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        ui->tablePlugins->setItem(row, 1, name_item);

        // Column 2: Version
        auto* version_item = new QTableWidgetItem(QString::fromStdString(plugin.version));
        version_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        ui->tablePlugins->setItem(row, 2, version_item);

        // Column 3: Description
        auto* desc_item = new QTableWidgetItem(QString::fromStdString(plugin.description));
        desc_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        ui->tablePlugins->setItem(row, 3, desc_item);
    }

    populating = false;
}

void PluginDialog::OnItemChanged(QTableWidgetItem* item) {
    if (populating || !item || item->column() != 0) {
        return;
    }

    QString filename = item->data(Qt::UserRole).toString();
    bool enabled = (item->checkState() == Qt::Checked);
    item->setText(enabled ? tr("Enabled") : tr("Disabled"));

    Plugins::PluginManager::GetInstance().SetPluginEnabled(filename.toStdString(), enabled);
    SaveSettings();
}

void PluginDialog::OnOpenPluginsFolder() {
    std::string dir_str = Plugins::PluginManager::GetInstance().GetPluginsDirectory();
    QDir dir(QString::fromStdString(dir_str));
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
}

void PluginDialog::OnReloadPlugins() {
    Plugins::PluginManager::GetInstance().ReloadPlugins();
    LoadSettings();
    PopulatePluginList();
}
