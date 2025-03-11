#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>



SettingsDialog::SettingsDialog(const QString& guid, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    m_guid = guid;
    ui->setupUi(this);
    // Set the job name at the top of the dialog
    ui->label_12->setText(guid);

    // Load settings from Settings.xml (for other settings like Header, Resolution, Dithering)
    loadSettingsFromXML();

    // Load ICC profiles from folders
    loadICCProfiles();

    // Connect signals and slots
    connect(ui->outputFolderButton, &QPushButton::clicked, this, &SettingsDialog::onOutputFolderButtonClicked);
    connect(ui->widthPercentageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDialog::onWidthPercentageChanged);
    connect(ui->heightPercentageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDialog::onHeightPercentageChanged);
    connect(ui->pushButton, &QPushButton::clicked, this, &SettingsDialog::on_pushDefaultButton_pressed);

}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::loadSettingsFromXML()
{
    QFile file("Settings.xml");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to open Settings.xml!");
        return;
    }

    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "HeaderList") {
                QStringList headers;
                while (xml.readNextStartElement()) {
                    headers.append(xml.readElementText());
                }
                settingsMap["HeaderList"] = headers;
            } else if (xml.name() == "ResolutionList") {
                QStringList resolutions;
                while (xml.readNextStartElement()) {
                    resolutions.append(xml.readElementText());
                }
                settingsMap["ResolutionList"] = resolutions;
            } else if (xml.name() == "DitheringList") {
                QStringList dithering;
                while (xml.readNextStartElement()) {
                    dithering.append(xml.readElementText());
                }
                settingsMap["DitheringList"] = dithering;
            }
        }
    }

    file.close();

    // Populate dropdowns for Header, Resolution, and Dithering
    ui->headerComboBox->addItems(settingsMap["HeaderList"]);
    ui->resolutionComboBox->addItems(settingsMap["ResolutionList"]);
    ui->ditheringComboBox->addItems(settingsMap["DitheringList"]);
}

void SettingsDialog::loadICCProfiles()
{
    // Load ICC profiles from the specified folders
    QDir rgbDir("C:/QTProject/ICCProfile/RGB");
    QDir cmykDir("C:/QTProject/ICCProfile/CMYK");
    QDir outputDir("C:/QTProject/ICCProfile/Output Profiles");

    // Check if folders exist
    if (!rgbDir.exists() || !cmykDir.exists() || !outputDir.exists()) {
        QMessageBox::warning(this, "Error", "ICC profile folders not found!");
        return;
    }

    // Load RGB profiles
    QStringList rgbProfiles;
    for (const QString& file : rgbDir.entryList(QDir::Files)) {
        rgbProfiles.append(file);
    }
    ui->importRGBComboBox->addItems(rgbProfiles);

    // Load CMYK profiles
    QStringList cmykProfiles;
    for (const QString& file : cmykDir.entryList(QDir::Files)) {
        cmykProfiles.append(file);
    }
    ui->importCMYKComboBox->addItems(cmykProfiles);

    // Load Output profiles
    QStringList outputProfiles;
    for (const QString& file : outputDir.entryList(QDir::Files)) {
        outputProfiles.append(file);
    }
    ui->outputProfileComboBox->addItems(outputProfiles);
}

void SettingsDialog::loadSettings(const QString& guid)
{
    QString fileName = guid + ".xml";
    QFile file(fileName);
    if (!file.exists()) {
        fileName = "DefaultJobSettings.xml";
        file.setFileName(fileName);
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return; // If the file doesn't exist, do nothing
    }
    int nindex = 0;
    QString qstr;
    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "Header") {
                qstr = xml.readElementText();
                nindex = qstr.toInt();
                ui->headerComboBox->setCurrentIndex(nindex);
            } else if (xml.name() == "Resolution") {
                ui->resolutionComboBox->setCurrentText(xml.readElementText());
            } else if (xml.name() == "Dithering") {
                qstr = xml.readElementText();
                nindex = qstr.toInt();
                ui->ditheringComboBox->setCurrentIndex(nindex);
            } else if (xml.name() == "ImportRGBProfile") {
                ui->importRGBComboBox->setCurrentText(xml.readElementText());
            } else if (xml.name() == "ImportCMYKProfile") {
                ui->importCMYKComboBox->setCurrentText(xml.readElementText());
            } else if (xml.name() == "OutputProfile") {
                ui->outputProfileComboBox->setCurrentText(xml.readElementText());
            } else if (xml.name() == "OutputFolder") {
                outputFolder = xml.readElementText();
                ui->outputFolderEdit->setText(outputFolder);
            } else if (xml.name() == "Colors") {
                QStringList colors = xml.readElementText().split(",");
                ui->cCheckBox->setChecked(colors.contains("C"));
                ui->mCheckBox->setChecked(colors.contains("M"));
                ui->yCheckBox->setChecked(colors.contains("Y"));
                ui->kCheckBox->setChecked(colors.contains("K"));
                ui->lcCheckBox->setChecked(colors.contains("Lc"));
                ui->lmCheckBox->setChecked(colors.contains("Lm"));
                ui->lyCheckBox->setChecked(colors.contains("Ly"));
                ui->lkCheckBox->setChecked(colors.contains("Lk"));
                ui->llkCheckBox->setChecked(colors.contains("LLk"));
                ui->s1CheckBox->setChecked(colors.contains("S1"));
                ui->s2CheckBox->setChecked(colors.contains("S2"));
                ui->s3CheckBox->setChecked(colors.contains("S3"));
                ui->s4CheckBox->setChecked(colors.contains("S4"));
                ui->s5CheckBox->setChecked(colors.contains("S5"));
                ui->s6CheckBox->setChecked(colors.contains("S6"));
            } else if (xml.name() == "Width") {
                originalWidth = xml.readElementText().toDouble();
                ui->widthEdit->setText(QString::number(originalWidth));
            } else if (xml.name() == "Height") {
                originalHeight = xml.readElementText().toDouble();
                ui->heightEdit->setText(QString::number(originalHeight));
            } else if (xml.name() == "WidthPercentage") {
                ui->widthPercentageSpinBox->setValue(xml.readElementText().toInt());
            } else if (xml.name() == "HeightPercentage") {
                ui->heightPercentageSpinBox->setValue(xml.readElementText().toInt());
            }
        }
    }

    file.close();
}
void SettingsDialog::saveSettings(const QString& guid)
{
    QString fileName = guid + ".xml";
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to save settings!");
        return;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("settings");
    // Save dropdown selections
    QString str = QString::number(ui->headerComboBox->currentIndex());
    xml.writeTextElement("Header", str);
    xml.writeTextElement("Resolution", ui->resolutionComboBox->currentText());
    str = QString::number(ui->ditheringComboBox->currentIndex());
    xml.writeTextElement("Dithering", str);
    xml.writeTextElement("ImportRGBProfile", ui->importRGBComboBox->currentText());
    xml.writeTextElement("ImportCMYKProfile", ui->importCMYKComboBox->currentText());
    xml.writeTextElement("OutputProfile", ui->outputProfileComboBox->currentText());

    // Save output folder
    xml.writeTextElement("OutputFolder", outputFolder);

    // Save color checkboxes
    QStringList colors;
    if (ui->cCheckBox->isChecked()) colors.append("C");
    if (ui->mCheckBox->isChecked()) colors.append("M");
    if (ui->yCheckBox->isChecked()) colors.append("Y");
    if (ui->kCheckBox->isChecked()) colors.append("K");
    if (ui->lcCheckBox->isChecked()) colors.append("Lc");
    if (ui->lmCheckBox->isChecked()) colors.append("Lm");
    if (ui->lyCheckBox->isChecked()) colors.append("Ly");
    if (ui->lkCheckBox->isChecked()) colors.append("Lk");
    if (ui->llkCheckBox->isChecked()) colors.append("LLk");
    if (ui->s1CheckBox->isChecked()) colors.append("S1");
    if (ui->s2CheckBox->isChecked()) colors.append("S2");
    if (ui->s3CheckBox->isChecked()) colors.append("S3");
    if (ui->s4CheckBox->isChecked()) colors.append("S4");
    if (ui->s5CheckBox->isChecked()) colors.append("S5");
    if (ui->s6CheckBox->isChecked()) colors.append("S6");
    xml.writeTextElement("Colors", colors.join(","));

    // Save image size
    xml.writeTextElement("Width", ui->widthEdit->text());
    xml.writeTextElement("Height", ui->heightEdit->text());
    xml.writeTextElement("WidthPercentage", QString::number(ui->widthPercentageSpinBox->value()));
    xml.writeTextElement("HeightPercentage", QString::number(ui->heightPercentageSpinBox->value()));

    xml.writeEndElement();
    xml.writeEndDocument();

    file.close();
}

void SettingsDialog::onOutputFolderButtonClicked()
{
    QString folder = QFileDialog::getExistingDirectory(this, "Select Output Folder", outputFolder);
    if (!folder.isEmpty()) {
        outputFolder = folder;
        ui->outputFolderEdit->setText(outputFolder);
    }
}

void SettingsDialog::onWidthPercentageChanged(int value)
{
    double newWidth = originalWidth * (value / 100.0);
    ui->widthEdit->setText(QString::number(newWidth));
    updateSizeFields();
}

void SettingsDialog::onHeightPercentageChanged(int value)
{
    double newHeight = originalHeight * (value / 100.0);
    ui->heightEdit->setText(QString::number(newHeight));
    updateSizeFields();
}

void SettingsDialog::updateSizeFields()
{
    // Update width and height proportionally
    double width = ui->widthEdit->text().toDouble();
    double height = ui->heightEdit->text().toDouble();
    double aspectRatio = originalWidth / originalHeight;

    if (sender() == ui->widthPercentageSpinBox) {
        height = width / aspectRatio;
        ui->heightEdit->setText(QString::number(height));
    } else if (sender() == ui->heightPercentageSpinBox) {
        width = height * aspectRatio;
        ui->widthEdit->setText(QString::number(width));
    }
}

void SettingsDialog::on_pushDefaultButton_pressed()
{
    // Save settings (assuming this works correctly)
    SettingsDialog::saveSettings(m_guid);

    // Check if the GUID.xml file already exists
    QString guidFilePath = m_guid + ".xml";
    QFile guidFile(guidFilePath);

    // Open the GUID.xml file for reading
    if (!guidFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open GUID file for reading:" << guidFilePath;
        return;
    }

    // Read the content of the GUID.xml file
    QTextStream in(&guidFile);
    QString content = in.readAll();
    guidFile.close();

    // Open the DefaultJobSettings.xml file for writing
    QFile defaultFile("DefaultJobSettings.xml");
    if (!defaultFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open DefaultJobSettings.xml for writing";
        return;
    }

    // Write the content to the DefaultJobSettings.xml file
    QTextStream out(&defaultFile);
    out << content; // Write the content to the new file
    defaultFile.close();

    qDebug() << "Default settings file updated successfully.";
}

#include <QFile>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QDebug>

void writeWidthAndHeightToXml(const QString& filePath, double width, double height)
{
    // Open the file for reading
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for reading:" << filePath;
        return;
    }

    // Read the existing XML content
    QByteArray xmlData = file.readAll();
    file.close();

    // Reopen the file for writing (this clears the file)
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return;
    }

    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true); // Enable pretty formatting

    QXmlStreamReader xmlReader(xmlData);

    // Write the XML content back, updating the Width and Height fields
    while (!xmlReader.atEnd()) {
        xmlReader.readNext();

        if (xmlReader.isStartDocument()) {
            xmlWriter.writeStartDocument();
        } else if (xmlReader.isEndDocument()) {
            xmlWriter.writeEndDocument();
        } else if (xmlReader.isStartElement()) {
            xmlWriter.writeStartElement(xmlReader.name().toString());

            // Write attributes (if any)
            for (const auto& attr : xmlReader.attributes()) {
                xmlWriter.writeAttribute(attr.name().toString(), attr.value().toString());
            }

            // Handle specific elements
            if (xmlReader.name() == "Width") {
                xmlWriter.writeCharacters(QString::number(width));
                xmlReader.readElementText(); // Skip the text node
            } else if (xmlReader.name() == "Height") {
                xmlWriter.writeCharacters(QString::number(height));
                xmlReader.readElementText(); // Skip the text node
            }
        } else if (xmlReader.isEndElement()) {
            xmlWriter.writeEndElement();
        } else if (xmlReader.isCharacters() && !xmlReader.isWhitespace()) {
            // Write text content for other elements
            xmlWriter.writeCharacters(xmlReader.text().toString());
        }
    }

    if (xmlReader.hasError()) {
        qWarning() << "XML error:" << xmlReader.errorString();
    }

    file.close();
}

#include <QFileInfo>
#include <QDir>

QString generateUniquePrtFilePath(const QString& outputFolder, const QString& imageName)
{
    QDir outputDir(outputFolder);
    if (!outputDir.exists()) {
        qWarning() << "Output folder does not exist:" << outputFolder;
        return QString();
    }

    // Extract the base name of the image (without extension)
    QFileInfo imageInfo(imageName);
    QString baseName = imageInfo.baseName(); // e.g., "image" from "image.jpg"

    // Start with the base name and .prt extension
    QString prtFileName = baseName + ".prt";
    QString prtFilePath = outputDir.filePath(prtFileName);

    // Check if the file already exists
    int counter = 1;
    while (QFile::exists(prtFilePath)) {
        // Append a number to the base name
        prtFileName = QString("%1%2.prt").arg(baseName).arg(counter, 2, 10, QChar('0')); // e.g., "image01.prt"
        prtFilePath = outputDir.filePath(prtFileName);
        counter++;
    }

    return prtFilePath;
}
