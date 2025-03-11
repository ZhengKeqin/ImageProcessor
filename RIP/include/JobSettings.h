#ifndef JOBSETTINGS_H
#define JOBSETTINGS_H

#include <QString>
#include <QFile>
#include <QXmlStreamReader>

class JobSettings
{
public:
    JobSettings();

    bool loadSettings(const QString& guid);

    // Getters for all fields
    int header() const { return m_header; }
    int dithering() const { return m_dithering; }
    float xResolution() const { return m_xResolution; }
    float yResolution() const { return m_yResolution; }
    QString importRGBProfile() const { return m_importRGBProfile; }
    QString importCMYKProfile() const { return m_importCMYKProfile; }
    QString outputProfile() const { return m_outputProfile; }
    QString outputFolder() const { return m_outputFolder; }
    float widthPercentage() const { return m_widthPercentage; }
    float heightPercentage() const { return m_heightPercentage; }
    float width() const { return m_width; }
    float height() const { return m_height; }

    // Getters for boolean fields
    bool c() const { return m_c; }
    bool m() const { return m_m; }
    bool y() const { return m_y; }
    bool k() const { return m_k; }
    bool lc() const { return m_lc; }
    bool lm() const { return m_lm; }
    bool lk() const { return m_lk; }
    bool llk() const { return m_llk; }
    bool s1() const { return m_s1; }
    bool s2() const { return m_s2; }
    bool s3() const { return m_s3; }
    bool s4() const { return m_s4; }
    bool s5() const { return m_s5; }
    bool s6() const { return m_s6; }

private:
    void parseXml(QXmlStreamReader& xml);

    // Fields
    int m_header;
    int m_dithering;
    float m_xResolution;
    float m_yResolution;
    QString m_importRGBProfile;
    QString m_importCMYKProfile;
    QString m_outputProfile;
    QString m_outputFolder;
    float m_widthPercentage;
    float m_heightPercentage;
    float m_width;
    float m_height;

    // Boolean fields
    bool m_c;
    bool m_m;
    bool m_y;
    bool m_k;
    bool m_lc;
    bool m_lm;
    bool m_lk;
    bool m_llk;
    bool m_s1;
    bool m_s2;
    bool m_s3;
    bool m_s4;
    bool m_s5;
    bool m_s6;
};

void writeWidthAndHeightToXml(const QString& filePath, double width, double height);
QString generateUniquePrtFilePath(const QString& outputFolder, const QString& imageName);
void logEvent(const QString& fileName, const QString& eventName, const QString& startTime, const QString& finishTime);

#endif // JOBSETTINGS_H
