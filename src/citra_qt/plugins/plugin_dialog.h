// Copyright 2026 Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QDialog>

namespace Ui {
class PluginDialog;
}

class QTableWidgetItem;

class PluginDialog : public QDialog {
    Q_OBJECT

public:
    explicit PluginDialog(QWidget* parent = nullptr);
    ~PluginDialog() override;

private slots:
    void OnItemChanged(QTableWidgetItem* item);
    void OnOpenPluginsFolder();
    void OnReloadPlugins();

private:
    void PopulatePluginList();
    void LoadSettings();
    void SaveSettings();

    std::unique_ptr<Ui::PluginDialog> ui;
    bool populating{false};
};
