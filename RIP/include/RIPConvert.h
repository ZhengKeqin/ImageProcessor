#ifndef RGBTOLABCONVERTER_H
#define RGBTOLABCONVERTER_H

#include <QImage>
#include <QByteArray>
#include <QString>
#include "ProcessStruct.h"

QByteArray convertRGBtoLAB(const QImage &image,  TagJobInfoRecord &jobInfo);
QByteArray convertLABtoCMYK(const QByteArray &labDataBuf,  TagJobInfoRecord &jobInfo);
QByteArray resizeCMYKData(const QByteArray &cmykDataBuf,  TagJobInfoRecord &jobInfo);
QByteArray floydSteinbergDitherFloat(const QByteArray &cmykDataBuf, TagJobInfoRecord &jobInfo);
//QByteArray floydSteinbergDitherInt(const QByteArray &cmykDataBuf, const TagJobInfoRecord &jobInfo);
//QByteArray floydSteinbergDitherFloatMP(const QByteArray &cmykDataBuf, const TagJobInfoRecord &jobInfo);
QByteArray orderedDither(const QByteArray &cmykDataBuf,  TagJobInfoRecord &jobInfo);
int CreatePrnFile(const QString &prnFilePath, const QByteArray &outDataBuf, const TagJobInfoRecord &jobInfo);

#endif // RGBTOLABCONVERTER_H
