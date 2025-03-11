#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QPixmap>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QSettings> // Include for QSettings
#include <QProcess> // For running Ghostscript
#include <QImageReader>
#include <QDebug>
#include "settingsdialog.h"
//#include <poppler-qt5.h> // For PDF rendering
#include <QUuid>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDir>
#include "./RIP/include/RgbToLabConverter.h"
#include "./RIP/include/JobSettings.h"
//#include "./RIP/include/tiffio.h"

QString generateGUID() {
    return QUuid::createUuid().toString();
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QSettings settings("MyCompany", "ImageProcessor");
    lastUsedDirectory = settings.value("LastUsedDirectory", QDir::homePath()).toString();
    // Setup UI
    setupJobDetails();
    selectedJobFullPath.clear();
    // Connect signals and slots
    connect(ui->sendJobButton, &QPushButton::clicked, this, &MainWindow::onSendJob);
    connect(ui->actionAdd, &QAction::triggered, this, &MainWindow::onAddJob); // Connect Add action
    connect(ui->actionDelete, &QAction::triggered, this, &MainWindow::onDeleteJob); // Connect Delete action
    connect(ui->jobTreeWidget, &QTreeWidget::itemClicked, this, &MainWindow::onJobSelected);
    connect(ui->actionJobSettings, &QAction::triggered, this, &MainWindow::onJobSettingsTriggered); // Connect onJobSettingsTriggered action

    // Load the job list when the application starts
    loadJobList();
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::onAddJob()
{
    // Get the list of supported image formats
    QList<QByteArray> formats = QImageReader::supportedImageFormats();

    // Build the filter string
    QString filter = "Image Files (";
    for (const QByteArray& format : formats) {
        filter += "*." + QString(format) + " ";
    }
    filter.chop(1); // Remove the trailing space
    filter += ");;All Files (*.*)";

    // Open the file dialog with the dynamically generated filter
    QString jobName = QFileDialog::getOpenFileName(
        this,
        "Add Job",
        lastUsedDirectory,
        filter
        );

    if (!jobName.isEmpty()) {
        QFileInfo fileInfo(jobName);
        lastUsedDirectory = fileInfo.path(); // Store the directory of the selected file

        // Generate a GUID for the job
        QString guid = generateGUID();
        m_guid = guid;
        // Add the job to the tree widget
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->jobTreeWidget);
        item->setText(0, jobName);
        item->setData(0, Qt::UserRole, guid); // Store GUID in the item's data
    }
}

void MainWindow::onDeleteJob()
{
    QTreeWidgetItem* selectedItem = ui->jobTreeWidget->currentItem();
    if (!selectedItem) {
        QMessageBox::warning(this, "Error", "No job selected!");
        return;
    }

    QString guid = selectedItem->data(0, Qt::UserRole).toString();

    // Delete the GUID.xml file
    QFile::remove(guid + ".xml");

    // Remove the job from the tree widget
    delete selectedItem;

    // Save the updated job list
    saveJobList();
}

void MainWindow::onJobSelected(QTreeWidgetItem *item, int column)
{
    QString startTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    if (item) {
        // Assuming the GUID is stored in the first column (column 0)
        selectedJobFullPath = item->text(0); // Store the selected job's GUID
        ui->sendJobButton->setEnabled(true); // Enable the sendJob button
        // Optionally, update the job properties in the UI
        ui->jobNameLabel->setText(selectedJobFullPath); // Display the job name
        m_guid = item->data(0, Qt::UserRole).toString();

    } else {
        selectedJobFullPath.clear(); // Clear the GUID if no item is selected
        ui->sendJobButton->setEnabled(false); // Disable the sendJob button
    }

    m_curfilePath = item->text(0);
    // Handle other image formats (e.g., JPEG, PNG, etc.)
    QImage image(m_curfilePath);
    m_curImage=image;
    if (image.isNull()) {
        ui->previewLabel->setText("Failed to load image!");
    } else {
        ui->previewLabel->setPixmap(QPixmap::fromImage(image.scaled(ui->previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    }
/*
    int w = image.width();
    int h = image.height();
    QSize s= image.size();
    QRect r=image.rect();

    int d= image.depth();
    int c= image.colorCount();
    int b=image.bitPlaneCount();
*/
    qsizetype si=image.sizeInBytes();

    // Update job details
    QFileInfo fileInfo(m_curfilePath);
    ui->jobNameLabel->setText(m_curfilePath);

    // Show image format (e.g., RGB or CMYK)
    QString format = "Welcome to AI world!";
    ui->TBDLabel->setText(format);


    // Show image resolution (DPI)
    const double dpiX = image.dotsPerMeterX() * 0.0254;
    const double dpiY = image.dotsPerMeterY() * 0.0254;
    if (!image.isNull()) {
        QString resolution = QString("%1 x %2 DPI").arg(image.dotsPerMeterX() * 0.0254).arg(image.dotsPerMeterY() * 0.0254);
        ui->resolutionLabel->setText(resolution);
    } else {
        ui->resolutionLabel->setText("N/A");
    }


    // 获取图像的宽度和高度（以像素为单位）
    int widthPixels = image.width();
    int heightPixels = image.height();
    QFile Imgf(m_curfilePath);
    float imgByte= (float)Imgf.size()/1024;

    // 将像素转换为米
    double widthMeters = widthPixels / dpiX*2.54;
    double heightMeters = heightPixels / dpiY*2.54;

    // 格式化 QString
    QString imageInfo = QString("Image size: %1x%2 pixels%3K %4x%5cm").arg(widthPixels).arg(heightPixels).arg(imgByte, 0, 'f', 1).arg(widthMeters, 0, 'f', 2).arg(heightMeters, 0, 'f', 2);

    if (!image.isNull()) {
        ui->dateLabel->setText(imageInfo);
    } else {
        ui->dateLabel->setText("N/A");
    }
    // 输出或显示 imageInfo
    // ...
    //QString guidFile = m_guid + ".xml";
    //writeWidthAndHeightToXml(guidFile, widthMeters, heightMeters);
    QString finishTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    logEvent(selectedJobFullPath, "Preview time:", startTime, finishTime);

}
void MainWindow::loadJobList()
{
    QFile file("joblist.xml");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return; // If the file doesn't exist, do nothing
    }

    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == "job") {
            QString id = xml.attributes().value("id").toString();
            QString path = xml.attributes().value("path").toString();

            // Add the job to the tree widget
            QTreeWidgetItem* item = new QTreeWidgetItem(ui->jobTreeWidget);
            item->setText(0, path);
            item->setData(0, Qt::UserRole, id); // Store GUID in the item's data
        }
    }

    file.close();
}
void MainWindow::saveJobList()
{
    QFile file("joblist.xml");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to save job list!");
        return;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("jobs");

    for (int i = 0; i < ui->jobTreeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = ui->jobTreeWidget->topLevelItem(i);
        QString path = item->text(0);
        QString id = item->data(0, Qt::UserRole).toString();

        xml.writeStartElement("job");
        xml.writeAttribute("id", id);
        xml.writeAttribute("path", path);
        xml.writeEndElement();
    }

    xml.writeEndElement();
    xml.writeEndDocument();

    file.close();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Save the job list before closing the application
    saveJobList();
    QSettings settings("MyCompany", "ImageProcessor");
    settings.setValue("LastUsedDirectory", lastUsedDirectory);
    event->accept();
}

void MainWindow::setupJobDetails()
{
    // Example: Add a job to the tree widget
    //jobItem->setText(0, "Color Swatches_Pantone-A.ps");

    // Example: Set system information
    ui->ramLabel->setText("RAM Total: ");
    ui->diskLabel->setText("C: Free: ");
}

void MainWindow::onJobSettingsTriggered()
{
    QTreeWidgetItem* selectedItem = ui->jobTreeWidget->currentItem();
    if (!selectedItem) {
        QMessageBox::warning(this, "Error", "No job selected!");
        return;
    }

    QString jobName = selectedItem->text(0);
    QString guid = selectedItem->data(0, Qt::UserRole).toString();
    m_guid = guid;
    SettingsDialog dialog(guid, this);
    dialog.loadSettings(guid); // Load settings from GUID.xml
    if (dialog.exec() == QDialog::Accepted) {
        dialog.saveSettings(guid); // Save settings to GUID.xml
    }
}


void  MainWindow::onSendJob()
{
    sendJobProcess(m_curImage, m_guid);
}

bool MainWindow::sendJobProcess(QImage& image, QString& guid)
{
        // Check if the GUID.xml file already exists
    QString guidFilePath = guid + ".xml";
    QFile file(guidFilePath);
    if (!QFile::exists(guidFilePath)) {
        QFile defaultFile("defaultJobSettings.xml");
        QFile guidFile(guidFilePath);

        // Open the default settings file for reading
        if (defaultFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // Read the entire content of the default settings file
            QTextStream in(&defaultFile);
            QString content = in.readAll();
            defaultFile.close();

            // Open the GUID.xml file for writing
            if (guidFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&guidFile);
                out << content; // Write the content to the new file
                guidFile.close();
            } else {
                //qWarning() << "Failed to create" << guidFilePath;
            }
        } else {
        }
    } else {
        qDebug() << guidFilePath << "already exists. No action taken.";
    }
    // Check if a job is selected
    if (selectedJobFullPath.isEmpty()) {
        qWarning() << "No job selected!";
        return false;
    }

    // Load settings for the selected job
    JobSettings jobSettings;
    if (!jobSettings.loadSettings(m_guid)) {
        qWarning() << "Failed to load settings for job:" << selectedJobFullPath;
        return false;
    }

    // Use the loaded settings
    int header = jobSettings.header();
    int dithering = jobSettings.dithering();
    float xResolution = jobSettings.xResolution();
    float yResolution = jobSettings.yResolution();
    //float xResolution = 2.54*image.dotsPerMeterX()/100;
    //float yResolution = 2.54*image.dotsPerMeterY()/100;
    QString importRGBProfile = jobSettings.importRGBProfile();
    QString importCMYKProfile = jobSettings.importCMYKProfile();
    QString outputProfile = jobSettings.outputProfile();
    QString outputFolder = jobSettings.outputFolder();
    float widthPercentage = jobSettings.widthPercentage();
    float heightPercentage = jobSettings.heightPercentage();
    int outputBuffSize = 0;
    int nLevel = 2;
    switch(dithering%3){
    case 1:
        nLevel = 3;
    case 2:
        nLevel = 4;
    }
    int nPerByte = 8/((nLevel + 1)/2);
    int FWidth = static_cast<int>(std::round(image.width()*widthPercentage/100/image.dotsPerMeterX()*100/2.54*xResolution)) ;
    int FHeight = static_cast<int>(std::round(image.height()*heightPercentage/100/image.dotsPerMeterY()*100/2.54*yResolution));
    int bytePerLine = static_cast<int>(std::ceil(static_cast<float>(FWidth)/nPerByte));
    float xTimes = static_cast<float>(FWidth)/image.width();
    float yTimes =static_cast<float>(FHeight)/image.height();

    /* float width = jobSettings.width();
    float height = jobSettings.height();

    // Boolean fields
    bool c = jobSettings.c();
    bool m = jobSettings.m();
    bool y = jobSettings.y();
    bool k = jobSettings.k();
    bool lc = jobSettings.lc();
    bool lm = jobSettings.lm();
    bool lk = jobSettings.lk();
    bool llk = jobSettings.llk();
    bool s1 = jobSettings.s1();
    bool s2 = jobSettings.s2();
    bool s3 = jobSettings.s3();
    bool s4 = jobSettings.s4();
    bool s5 = jobSettings.s5();
    bool s6 = jobSettings.s6();

    //QString inputRGBICC("C:/QTProject/ICCProfile/RGB/AdobeRGB1998.icc");

    //QString outputCMYKICC("C:/QTProject/ICCProfile/CMYK/CoatedFOGRA39.icc");
*/
    ui->TBDLabel->setText("Processing Image....");
    // Force the event loop to process pending events (like UI updates)
    QCoreApplication::processEvents();
    importRGBProfile = "C:/QTProject/ICCProfile/RGB/"+ importRGBProfile;
    outputProfile =  "C:/QTProject/ICCProfile/CMYK/" + outputProfile;
        // Get the current system time for StartTime
    QString startTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    QByteArray labDataBuf= RgbToLabConverter::convert(image, importRGBProfile);

    // Get the current system time for FinishTime
    QString finishTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString combinedString = "Dither = " + QString::number(dithering) +
                             ", nLevel = " + QString::number(nLevel) +
                             ", Resolution = " + QString::number(xResolution, 'f', 1) + "x" + QString::number(yResolution, 'f', 1)
                             +" convertRGBtoLAB";
    logEvent(selectedJobFullPath, combinedString, startTime, finishTime);

    startTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QByteArray cmykDataBuf= RgbToLabConverter::ConvertLABtoCMYK(labDataBuf, outputProfile);
    finishTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    logEvent(selectedJobFullPath, "ConvertLABtoCMYK", startTime, finishTime);
    //labDataBuf.clear();

    startTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    //xTimes = 1.0; yTimes=1.0;
    QByteArray resizedCmykDataBuf = RgbToLabConverter::resizeCMYKData(cmykDataBuf, image.width(), image.height(), xTimes, yTimes);
    finishTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    logEvent(selectedJobFullPath, "resizeCMYKData", startTime, finishTime);

    startTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QByteArray outDataBuf;
    if (dithering <= 2)
        outDataBuf = RgbToLabConverter::floydSteinbergDither(resizedCmykDataBuf, FWidth, FHeight, nLevel, bytePerLine, outputBuffSize);
    else
        outDataBuf = RgbToLabConverter::orderedDither(resizedCmykDataBuf, FWidth, FHeight, nLevel, bytePerLine, outputBuffSize);
    finishTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    logEvent(selectedJobFullPath, "Dithering", startTime, finishTime);
    ui->TBDLabel->setText("Saving native file....!");

    outputFolder = jobSettings.outputFolder();
    QFileInfo fileInfo(m_curfilePath);
    QString imageName = fileInfo.baseName(); //
    // Generate a unique .prt file path
    QString prtFilePath = generateUniquePrtFilePath(outputFolder, imageName);

    startTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    //xResolution = image.dotsPerMeterX()/100*2.54;
    //yResolution = image.dotsPerMeterY()/100*2.54;
    int TPrnSize =  RgbToLabConverter::CreatePrnFile(prtFilePath, outDataBuf, FWidth, FHeight, bytePerLine,xResolution,yResolution, nLevel);
    finishTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    logEvent(selectedJobFullPath, "CreatePrnFile", startTime, finishTime);
    ui->TBDLabel->setText("Job finished!");


    return true;
}
