#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QPixmap>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QSettings>
#include <QProcess>
#include <QImageReader>
#include <QDebug>
#include "settingsdialog.h"
#include <QUuid>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDir>
#include "RIPConvert.h"
#include "JobSettings.h"
#include "ProcessStruct.h"

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
    connect(ui->actionAdd, &QAction::triggered, this, &MainWindow::onAddJob);
    connect(ui->actionDelete, &QAction::triggered, this, &MainWindow::onDeleteJob);
    connect(ui->jobTreeWidget, &QTreeWidget::itemClicked, this, &MainWindow::onJobSelected);
    connect(ui->actionJobSettings, &QAction::triggered, this, &MainWindow::onJobSettingsTriggered);

    // Load the job list when the application starts
    loadJobList();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onAddJob()
{
    QList<QByteArray> formats = QImageReader::supportedImageFormats();
    QString filter = "Image Files (";
    for (const QByteArray& format : formats) {
        filter += "*." + QString(format) + " ";
    }
    filter.chop(1);
    filter += ");;All Files (*.*)";

    QString jobName = QFileDialog::getOpenFileName(
        this,
        "Add Job",
        lastUsedDirectory,
        filter
        );

    if (!jobName.isEmpty()) {
        QFileInfo fileInfo(jobName);
        lastUsedDirectory = fileInfo.path();

        QString guid = generateGUID();
        m_guid = guid;

        QTreeWidgetItem* item = new QTreeWidgetItem(ui->jobTreeWidget);
        item->setText(0, jobName);
        item->setData(0, Qt::UserRole, guid);
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
    QFile::remove(guid + ".xml");
    delete selectedItem;
    saveJobList();
}

void MainWindow::onJobSelected(QTreeWidgetItem *item, int column)
{
    QString startTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    if (item) {
        selectedJobFullPath = item->text(0);
        ui->sendJobButton->setEnabled(true);
        ui->jobNameLabel->setText(selectedJobFullPath);
        m_guid = item->data(0, Qt::UserRole).toString();
    } else {
        selectedJobFullPath.clear();
        ui->sendJobButton->setEnabled(false);
    }

    m_curfilePath = item->text(0);
    QImage image(m_curfilePath);
    m_curImage = image;

    if (image.isNull()) {
        ui->previewLabel->setText("Failed to load image!");
    } else {
        ui->previewLabel->setPixmap(QPixmap::fromImage(image.scaled(ui->previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    }

    qsizetype si = image.sizeInBytes();
    QFileInfo fileInfo(m_curfilePath);
    ui->jobNameLabel->setText(m_curfilePath);

    QString format = "Welcome to AI world!";
    ui->TBDLabel->setText(format);

    const double dpiX = image.dotsPerMeterX() * 0.0254;
    const double dpiY = image.dotsPerMeterY() * 0.0254;
    if (!image.isNull()) {
        QString resolution = QString("%1 x %2 DPI").arg(dpiX).arg(dpiY);
        ui->resolutionLabel->setText(resolution);
    } else {
        ui->resolutionLabel->setText("N/A");
    }

    int widthPixels = image.width();
    int heightPixels = image.height();
    QFile Imgf(m_curfilePath);
    float imgByte = (float)Imgf.size() / 1024;

    double widthMeters = widthPixels / dpiX * 2.54;
    double heightMeters = heightPixels / dpiY * 2.54;

    QString imageInfo = QString("Image size: %1x%2 pixels%3K %4x%5cm").arg(widthPixels).arg(heightPixels).arg(imgByte, 0, 'f', 1).arg(widthMeters, 0, 'f', 2).arg(heightMeters, 0, 'f', 2);

    if (!image.isNull()) {
        ui->dateLabel->setText(imageInfo);
    } else {
        ui->dateLabel->setText("N/A");
    }

    QString finishTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    logEvent(selectedJobFullPath, "Preview time:", startTime, finishTime);
}

void MainWindow::loadJobList()
{
    QFile file("joblist.xml");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == "job") {
            QString id = xml.attributes().value("id").toString();
            QString path = xml.attributes().value("path").toString();

            QTreeWidgetItem* item = new QTreeWidgetItem(ui->jobTreeWidget);
            item->setText(0, path);
            item->setData(0, Qt::UserRole, id);
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
    saveJobList();
    QSettings settings("MyCompany", "ImageProcessor");
    settings.setValue("LastUsedDirectory", lastUsedDirectory);
    event->accept();
}

void MainWindow::setupJobDetails()
{
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
    dialog.loadSettings(guid);
    if (dialog.exec() == QDialog::Accepted) {
        dialog.saveSettings(guid);
    }
}

void MainWindow::onSendJob()
{
    sendJobProcess(m_curImage, m_guid);
}

bool MainWindow::sendJobProcess(QImage& image, QString& guid)
{
    QString guidFilePath = guid + ".xml";
    QFile file(guidFilePath);
    if (!QFile::exists(guidFilePath)) {
        QFile defaultFile("defaultJobSettings.xml");
        QFile guidFile(guidFilePath);

        if (defaultFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&defaultFile);
            QString content = in.readAll();
            defaultFile.close();

            if (guidFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&guidFile);
                out << content;
                guidFile.close();
            }
        }
    }

    if (selectedJobFullPath.isEmpty()) {
        qWarning() << "No job selected!";
        return false;
    }

    JobSettings jobSettings;
    if (!jobSettings.loadSettings(m_guid)) {
        qWarning() << "Failed to load settings for job:" << selectedJobFullPath;
        return false;
    }

    // Populate the TagJobInfoRecord structure using FillJobInfoStruct
    TagJobInfoRecord jobInfo;
    FillJobInfoStruct(jobSettings, image, jobInfo);

    ui->TBDLabel->setText("Processing Image....");
    QCoreApplication::processEvents();

    QString startTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QByteArray labDataBuf = convertRGBtoLAB(image, jobInfo);
    QString finishTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString combinedString = "Dither = " + QString::number(jobInfo.dithering) +
                             ", nLevel = " + QString::number(jobInfo.nLevel) +
                             ", Resolution = " + QString::number(jobInfo.xResolution, 'f', 1) + "x" + QString::number(jobInfo.yResolution, 'f', 1) +
                             " convertRGBtoLAB";
    logEvent(selectedJobFullPath, combinedString, startTime, finishTime);

    startTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QByteArray cmykDataBuf = convertLABtoCMYK(labDataBuf, jobInfo);
    finishTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    logEvent(selectedJobFullPath, "ConvertLABtoCMYK", startTime, finishTime);

    startTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QByteArray resizedCmykDataBuf = resizeCMYKData(cmykDataBuf, jobInfo);
    finishTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    logEvent(selectedJobFullPath, "resizeCMYKData", startTime, finishTime);

    startTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QByteArray outDataBuf;
    if (jobInfo.dithering <= 2)
        outDataBuf = floydSteinbergDitherFloat(resizedCmykDataBuf, jobInfo);
    else
        outDataBuf = orderedDither(resizedCmykDataBuf, jobInfo);
    finishTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    logEvent(selectedJobFullPath, "Dithering", startTime, finishTime);

    ui->TBDLabel->setText("Saving native file....!");

    QString outputFolder = jobInfo.outputFolder;
    QFileInfo fileInfo(m_curfilePath);
    QString imageName = fileInfo.baseName();
    QString prtFilePath = generateUniquePrtFilePath(outputFolder, imageName);

    startTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QByteArray headerBuf;
    FillHeaderStruct(jobInfo, &headerBuf); // Fill the header structure
    int TPrnSize = CreatePrnFile(prtFilePath, outDataBuf, jobInfo);
    finishTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    logEvent(selectedJobFullPath, "CreatePrnFile", startTime, finishTime);

    ui->TBDLabel->setText("Job finished!");

    return true;
}
