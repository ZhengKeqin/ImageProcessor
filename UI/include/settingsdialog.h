#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QMap>
#include <QStringList>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(const QString& guid,QWidget *parent = nullptr);
    ~SettingsDialog();

    // Load settings from XML
    void loadSettings(const QString& guid);
    // Save settings to XML
    void saveSettings(const QString& guid);

private slots:
    void onOutputFolderButtonClicked();
    void onWidthPercentageChanged(int value);
    void onHeightPercentageChanged(int value);

    void on_pushDefaultButton_pressed();

private:
    Ui::SettingsDialog *ui;
    QMap<QString, QStringList> settingsMap; // Store settings from Settings.xml
    QString outputFolder; // Store the output folder path
    double originalWidth; // Original width in cm
    double originalHeight; // Original height in cm
    QString m_guid;
    void loadSettingsFromXML(); // Load settings from Settings.xml
    void updateSizeFields(); // Update width and height fields proportionally
    void loadICCProfiles();
};

#endif // SETTINGSDIALOG_H
