#include "JobSettings.h"
#include <QDebug>

#include <QFileInfo>
#include <QDir>


#include <QFile>
#include <QXmlStreamWriter>
#include <QDateTime>
#include <QElapsedTimer>

JobSettings::JobSettings()
    : m_header(0), m_dithering(0), m_xResolution(0.0f), m_yResolution(0.0f),
      m_widthPercentage(100.0f), m_heightPercentage(100.0f), m_width(0.0f), m_height(0.0f),
      m_c(false), m_m(false), m_y(false), m_k(false),
      m_lc(false), m_lm(false), m_lk(false), m_llk(false),
      m_s1(false), m_s2(false), m_s3(false), m_s4(false), m_s5(false), m_s6(false)
{
    // Initialize other fields as needed
}

bool JobSettings::loadSettings(const QString& guid)
{
    QString fileName = guid + ".xml";

    // If the job-specific settings file does not exist, use DefaultJobSettings.xml
    QFile file(fileName);
    if (!file.exists()) {
        fileName = "DefaultJobSettings.xml";
        file.setFileName(fileName);
    }

    // Open the file for reading
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open settings file:" << fileName;
        return false;
    }

    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            parseXml(xml);
        }
    }

    file.close();
    return true;
}

void JobSettings::parseXml(QXmlStreamReader& xml)
{
    if (xml.name() == "Header") {
        m_header = xml.readElementText().toInt();
    } else if (xml.name() == "Dithering") {
        m_dithering = xml.readElementText().toInt();
    } else if (xml.name() == "Resolution") {
        QString resolution = xml.readElementText();
        QStringList parts = resolution.split("x");
        if (parts.size() == 2) {
            m_xResolution = parts[0].toFloat();
            m_yResolution = parts[1].toFloat();
        }
    } else if (xml.name() == "ImportRGBProfile") {
        m_importRGBProfile = xml.readElementText();
    } else if (xml.name() == "ImportCMYKProfile") {
        m_importCMYKProfile = xml.readElementText();
    } else if (xml.name() == "OutputProfile") {
        m_outputProfile = xml.readElementText();
    } else if (xml.name() == "OutputFolder") {
        m_outputFolder = xml.readElementText();
    } else if (xml.name() == "WidthPercentage") {
        m_widthPercentage = xml.readElementText().toFloat();
    } else if (xml.name() == "HeightPercentage") {
        m_heightPercentage = xml.readElementText().toFloat();
    } else if (xml.name() == "Width") {
        m_width = xml.readElementText().toFloat();
    } else if (xml.name() == "Height") {
        m_height = xml.readElementText().toFloat();
    } else if (xml.name() == "Colors") {
        QStringList colors = xml.readElementText().split(",");
        m_c = colors.contains("C");
        m_m = colors.contains("M");
        m_y = colors.contains("Y");
        m_k = colors.contains("K");
        m_lc = colors.contains("Lc");
        m_lm = colors.contains("Lm");
        m_lk = colors.contains("Lk");
        m_llk = colors.contains("LLk");
        m_s1 = colors.contains("S1");
        m_s2 = colors.contains("S2");
        m_s3 = colors.contains("S3");
        m_s4 = colors.contains("S4");
        m_s5 = colors.contains("S5");
        m_s6 = colors.contains("S6");
    }
}
#include <QFile>
#include <QXmlStreamWriter>
#include <QDateTime>
#include <QDebug>
#include <QThread>

void logEvent(const QString& fileName, const QString& eventName, const QString& startTime, const QString& finishTime){
//void logEvent(const QString& fileName, const QString& eventName) {
    QFile file("log.xml");

    // Check if the file exists
    bool fileExists = file.exists();

    // Open the file in append mode or create it if it doesn't exist
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        qWarning() << "Failed to open log file.";
        return;
    }

    // Create an XML stream writer
    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true); // Enable pretty formatting

    if (!fileExists) {
        // If the file doesn't exist, write the XML declaration and root element
        xmlWriter.writeStartDocument();
        xmlWriter.writeStartElement("Events");
    } else {
        // If the file exists, move to the end of the file and find the closing </Events> tag
        file.seek(file.size() - QString("</Events>\n").size());
    }
    // Calculate the time interval
    QDateTime start = QDateTime::fromString(startTime, "yyyy-MM-dd HH:mm:ss");
    QDateTime finish = QDateTime::fromString(finishTime, "yyyy-MM-dd HH:mm:ss");
    int intervalSeconds = start.secsTo(finish); // Time difference in seconds

    // Write the new event
    xmlWriter.writeStartElement("Event");
    xmlWriter.writeTextElement("FileName", fileName);
    xmlWriter.writeTextElement("EventName", eventName);
    xmlWriter.writeTextElement("StartTime", startTime);
    xmlWriter.writeTextElement("FinishTime", finishTime);
    xmlWriter.writeTextElement("IntervalSeconds", QString::number(intervalSeconds)); // Add interval
    xmlWriter.writeEndElement(); // Close Event

    if (!fileExists) {
        // Close the root element if the file was just created
        xmlWriter.writeEndElement(); // Close Events
        xmlWriter.writeEndDocument();
    } else {
        // Write the closing </Events> tag if the file already existed
        xmlWriter.writeEndElement(); // Close Events
    }

    file.close();
    qDebug() << "Event logged successfully.";
}

