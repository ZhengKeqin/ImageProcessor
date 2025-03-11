#include "../../include.h"

#include "../include/ProcessStruct.h"

void FillJobInfoStruct(JobSettings& settings, QImage& image, TagJobInfoRecord& Info)
{
    // Use the loaded settings
    Info.header = settings.header();
    Info.dithering = settings.dithering();
    Info.xResolution = settings.xResolution();
    Info.yResolution = settings.yResolution();
    Info.xImageResolution = 2.54*image.dotsPerMeterX()/100;
    Info.yImageResolution = 2.54*image.dotsPerMeterY()/100;
    Info.importRGBProfile = settings.importRGBProfile();
    Info.importCMYKProfile = settings.importCMYKProfile();
    Info.outputProfile = settings.outputProfile();
    Info.outputFolder = settings.outputFolder();
    Info.widthPercentage = settings.widthPercentage();
    Info.heightPercentage = settings.heightPercentage();
    Info.outputBuffSize = 0;
    Info.nLevel = 2;
    switch(Info.dithering%3){
    case 1:
        Info.nLevel = 3;
    case 2:
        Info.nLevel = 4;
    }
    Info.nPixelPerByte = 8/((Info.nLevel + 1)/2);
    Info.OutputWidth = static_cast<int>(std::round(image.width()*Info.widthPercentage/100/image.dotsPerMeterX()*100/2.54*Info.xResolution)) ;
    Info.OutputHeight = static_cast<int>(std::round(image.height()*Info.heightPercentage/100/image.dotsPerMeterY()*100/2.54*Info.yResolution));
    Info.bytePerLine = static_cast<int>(std::ceil(static_cast<float>(Info.OutputWidth)/Info.nPixelPerByte));
    Info.xTimes = static_cast<float>(Info.OutputWidth)/image.width();
    Info.yTimes =static_cast<float>(Info.OutputHeight)/image.height();
    Info.OutputWidth = settings.width();
    Info.OutputHeight = settings.height();
    // Boolean fields
    Info.cc = settings.c();
    Info.mm = settings.m();
    Info.yy = settings.y();
    Info.kk = settings.k();
    Info.lcc = settings.lc();
    Info.lmm = settings.lm();
    Info.lkk = settings.lk();
    Info.llkk = settings.llk();
    Info.ss1 = settings.s1();
    Info.ss2 = settings.s2();
    Info.ss3 = settings.s3();
    Info.ss4 = settings.s4();
    Info.ss5 = settings.s5();
    Info.ss6 = settings.s6();

return;
}

int FillHeaderStruct(TagJobInfoRecord& settings, QByteArray* Headbuf)
{
	int nSize = 0;
    Headbuf->fill(0);

    if (settings.header == 1)
	{
		// Ensure Headbuf is large enough to hold the JobInfo structure
		 nSize = Headbuf->size();
         if (Headbuf->size() < static_cast<int>(sizeof(tagHeadRecord_1))) {
            Headbuf->resize(sizeof(tagHeadRecord_1));
		}

		// Step 2: Cast Headbuf's data to a JobInfo pointer
        tagHeadRecord_1* header = reinterpret_cast<tagHeadRecord_1*>(Headbuf->data());

		// Step 3: Assign values to the structure
		   // Populate the header structure

        header->nSignature = 0x5555; // Signature
        header->nXDPI = int(settings.xResolution + 0.5) ;         // Example: 300 DPI (adjust as needed)
        header->nYDPI = int(settings.yResolution+ 0.5) ;         // Example: 300 DPI (adjust as needed)
        header->nBytesPerLine = (settings.bytePerLine + 3)/4*4;
        header->nHeight = settings.OutputHeight;
        header->nWidth = settings.OutputWidth;
        header->nPaperWidth = 0;     // Not used
        header->nColors = 4;         // CMYK
        header->nBits = (settings.nLevel+1)/2;           // Not used
        header->nReserved[0] = 0;    // Pass Number
        header->nReserved[1] = 0;    // vsdMode
        header->nReserved[2] = 0;    // Reserved
	}
	return nSize;
}
