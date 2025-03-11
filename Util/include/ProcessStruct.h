#ifndef PROCESSSTRUCT_H
#define PROCESSSTRUCT_H

#include <QByteArray>
#include <QString>
#include "JobSettings.h"

// Define the info structure to process the image
#pragma pack(push, 1) // Ensure the structure is packed without padding
struct TagJobInfoRecord {
    QByteArray inputDataBuf;    // Input data buffer
    QByteArray outputDataBuf;   // Output data buffer
    int width;                  // Image width
    int height;                 // Image height
    int level;                  // Dithering level
    int bytePerLine;            // Bytes per line
    int outputBuffSize;         // Output buffer size
    int header;                 // Header information
    int dithering;              // Dithering method
    float xImageResolution;     // Image X resolution
    float yImageResolution;     // Image Y resolution
    float xResolution;          // Output X resolution
    float yResolution;          // Output Y resolution
    QString importRGBProfile;   // Input RGB ICC profile path
    QString importCMYKProfile;  // Input CMYK ICC profile path
    QString outputProfile;      // Output ICC profile path
    QString outputFolder;       // Output folder path
    float widthPercentage;      // Width scaling percentage
    float heightPercentage;     // Height scaling percentage
    int nLevel;                 // Number of levels for dithering
    int nPixelPerByte;          // Number of pixels per byte
    int OutputWidth;            // Output width
    int OutputHeight;            // Output width
    float xTimes;               // X scaling factor
    float yTimes;               // Y scaling factor
    bool cc;                    // Cyan channel enabled
    bool mm;                    // Magenta channel enabled
    bool yy;                    // Yellow channel enabled
    bool kk;                    // Black channel enabled
    bool lcc;                   // Light cyan channel enabled
    bool lmm;                   // Light magenta channel enabled
    bool lkk;                   // Light black channel enabled
    bool llkk;                  // Light light black channel enabled
    bool ss1;                   // Spot color 1 enabled
    bool ss2;                   // Spot color 2 enabled
    bool ss3;                   // Spot color 3 enabled
    bool ss4;                   // Spot color 4 enabled
    bool ss5;                   // Spot color 5 enabled
    bool ss6;                   // Spot color 6 enabled
};
#pragma pack(pop) // Restore default packing

// Define the header structure
#pragma pack(push, 1) // Ensure the structure is packed without padding
struct tagHeadRecord_1 {
    int nSignature;      // Signature (e.g., 0x1234)
    int nXDPI;           // Image XDPI
    int nYDPI;           // Image YDPI
    int nBytesPerLine;   // Bytes per line
    int nHeight;         // Image height
    int nWidth;          // Image width
    int nPaperWidth;     // Paper width (unused)
    int nColors;         // Number of colors (1=monochrome, 4=CMYK)
    int nBits;           // Bits per pixel (unused)
    int nReserved[3];    // Reserved fields
};
#pragma pack(pop) // Restore default packing

void FillJobInfoStruct(JobSettings& settings, QImage& image, TagJobInfoRecord& Info);
int FillHeaderStruct(TagJobInfoRecord& settings, QByteArray* Headbuf);

#endif // PROCESSSTRUCT_H
