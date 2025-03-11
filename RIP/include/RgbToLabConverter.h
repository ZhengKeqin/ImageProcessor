
#ifndef RGBTOLABCONVERTER_H
#define RGBTOLABCONVERTER_H

#include <QImage>
#include <QByteArray>
#include <QString>
//#include <vector>
#include <QImage>

class RgbToLabConverter
{
public:
    RgbToLabConverter();
    static QByteArray convert(const QImage &image, const QString &rgbIccProfilePath);
    static QByteArray ConvertLABtoCMYK(const QByteArray &labDataBuf, const QString &cmykIccProfilePath);
    static QByteArray resizeCMYKData(const QByteArray &cmykDataBuf, int originalWidth, int originalHeight, float xTimes, float yTimes);
    static QByteArray floydSteinbergDither(const QByteArray &cmykDataBuf, int width, int height, int level, int &bytePerLine, int &outputBuffSize);
    static QByteArray orderedDither(const QByteArray &cmykDataBuf, int width, int height, int level, int &bytePerLine, int &outputBuffSize);
    static int CreatePrnFile(const QString &prnFilePath, const QByteArray &outDataBuf, int width, int height, int bytePerLine,float xRes, float yRes, int nLevel);
    static QVector<QVector<int>> generateBayerMatrix(int size) ;

private:
    //RgbToLabConverter() = delete;
    ~RgbToLabConverter() = delete;
};

#endif // RGBTOLABCONVERTER_H
