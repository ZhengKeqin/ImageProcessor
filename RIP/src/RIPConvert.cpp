#include "include.h"
#include "ProcessStruct.h"


QByteArray convertRGBtoLAB(const QImage &image, const TagJobInfoRecord &jobInfo) {
    if (image.isNull()) {
        qWarning() << "Invalid input image";
        return QByteArray();
    }

    // Convert to 32-bit ARGB format for consistent processing
    QImage convertedImage = image.convertToFormat(QImage::Format_ARGB32);

    // Ensure contiguous memory storage
    if (convertedImage.bytesPerLine() != convertedImage.width() * 4) {
        convertedImage = QImage(convertedImage.constBits(),
                                convertedImage.width(),
                                convertedImage.height(),
                                convertedImage.width() * 4,
                                convertedImage.format()).copy();
        if (convertedImage.isNull()) {
            qWarning() << "Failed to create contiguous image copy";
            return QByteArray();
        }
    }

    cmsHPROFILE rgbProfile = cmsOpenProfileFromFile(jobInfo.importRGBProfile.toLocal8Bit().constData(), "r");
    if (jobInfo.importRGBProfile.isEmpty() || !rgbProfile) {
        const int width = convertedImage.width();
        const int height = convertedImage.height();
        const int totalPixels = width * height;

        QByteArray labBuffer(totalPixels * 3 * sizeof(float), Qt::Uninitialized); // 3 channels (L, a, b)
        float* labData = reinterpret_cast<float*>(labBuffer.data());

        const quint32* pixels = reinterpret_cast<const quint32*>(convertedImage.constBits());

        for (int i = 0; i < totalPixels; ++i) {
            const quint32 pixel = pixels[i];

            // Extract RGB components
            float r3 = qRed(pixel) / 255.0f;
            float g3 = qGreen(pixel) / 255.0f;
            float b3 = qBlue(pixel) / 255.0f;

            // Convert sRGB to linear RGB
            auto srgbToLinear = [](float value) {
                return (value <= 0.04045f) ? value / 12.92f : std::pow((value + 0.055f) / 1.055f, 2.4f);
            };

            r3 = srgbToLinear(r3);
            g3 = srgbToLinear(g3);
            b3 = srgbToLinear(b3);

            // Convert linear RGB to XYZ (sRGB matrix)
            float x = r3 * 0.4124564f + g3 * 0.3575761f + b3 * 0.1804375f;
            float y = r3 * 0.2126729f + g3 * 0.7151522f + b3 * 0.0721750f;
            float z = r3 * 0.0193339f + g3 * 0.1191920f + b3 * 0.9503041f;

            // Convert XYZ to Lab
            auto xyzToLab = [](float value) {
                return (value > 0.008856f) ? std::pow(value, 1.0f / 3.0f) : (7.787f * value) + (16.0f / 116.0f);
            };

            float xn = 0.95047f; // D65 white point
            float yn = 1.00000f;
            float zn = 1.08883f;

            float fx = xyzToLab(x / xn);
            float fy = xyzToLab(y / yn);
            float fz = xyzToLab(z / zn);

            labData[i * 3] = std::max(0.0f, 116.0f * fy - 16.0f); // L*
            labData[i * 3 + 1] = 500.0f * (fx - fy);              // a*
            labData[i * 3 + 2] = 200.0f * (fy - fz);              // b*
        }

        return labBuffer;
    }

    cmsHPROFILE labProfile = cmsCreateLab4Profile(nullptr);
    if (!labProfile) {
        cmsCloseProfile(rgbProfile);
        qWarning() << "Failed to create LAB profile";
        return QByteArray();
    }

    // Create color transformation
    cmsHTRANSFORM transform = cmsCreateTransform(rgbProfile,
                                                 TYPE_RGB_8,
                                                 labProfile,
                                                 TYPE_Lab_FLT,
                                                 INTENT_PERCEPTUAL,
                                                 0);
    cmsCloseProfile(rgbProfile);
    cmsCloseProfile(labProfile);

    if (!transform) {
        qWarning() << "Failed to create color transform";
        return QByteArray();
    }

    // Prepare input and output buffers
    const int width = convertedImage.width();
    const int height = convertedImage.height();
    const int totalPixels = width * height;

    QByteArray rgbData(totalPixels * 3, Qt::Uninitialized); // 3 channels (R, G, B)
    QByteArray labBuffer(totalPixels * 3 * sizeof(float), Qt::Uninitialized); // 3 channels (L, a, b)

    // Extract RGB components from ARGB32 pixels
    const quint32* pixels = reinterpret_cast<const quint32*>(convertedImage.constBits());
    uchar* rgbBuffer = reinterpret_cast<uchar*>(rgbData.data());

    for (int i = 0; i < totalPixels; ++i) {
        const quint32 pixel = pixels[i];
        rgbBuffer[i * 3] = qRed(pixel);    // Red component
        rgbBuffer[i * 3 + 1] = qGreen(pixel); // Green component
        rgbBuffer[i * 3 + 2] = qBlue(pixel);  // Blue component
    }

    // Transform to LAB
    cmsDoTransform(transform, rgbBuffer, labBuffer.data(), totalPixels);

    // Clean up
    cmsDeleteTransform(transform);

    return labBuffer;
}

QByteArray convertLABtoCMYK(const QByteArray &labDataBuf, const TagJobInfoRecord &jobInfo) {
    if (labDataBuf.isEmpty()) {
        qWarning() << "Invalid LAB data buffer";
        return QByteArray();
    }

    // Load the CMYK profile
    cmsHPROFILE cmykProfile = cmsOpenProfileFromFile(jobInfo.importCMYKProfile.toLocal8Bit().constData(), "r");

    // If no ICC profile is provided, use theoretical Lab-to-CMYK conversion
    if (jobInfo.importCMYKProfile.isEmpty() || !cmykProfile) {
        qWarning() << "Failed to open CMYK ICC profile";
        const int pixelCount = labDataBuf.size() / (3 * sizeof(float)); // 3 channels (L, a, b)
        QByteArray cmykDataBuf(pixelCount * 4, Qt::Uninitialized);     // 4 channels (C, M, Y, K)
        uchar* cmykData = reinterpret_cast<uchar*>(cmykDataBuf.data());
        const float* labData = reinterpret_cast<const float*>(labDataBuf.constData());

        for (int i = 0; i < pixelCount; ++i) {
            // Extract Lab values
            float L = labData[i * 3];
            float a = labData[i * 3 + 1];
            float b = labData[i * 3 + 2];

            // Convert Lab to XYZ
            float fy = (L + 16.0f) / 116.0f;
            float fx = a / 500.0f + fy;
            float fz = fy - b / 200.0f;

            auto labToXyz = [](float value) {
                float cube = value * value * value;
                return (cube > 0.008856f) ? cube : (value - 16.0f / 116.0f) / 7.787f;
            };

            float xn = 0.95047f; // D65 white point
            float yn = 1.00000f;
            float zn = 1.08883f;

            float x = xn * labToXyz(fx);
            float y = yn * labToXyz(fy);
            float z = zn * labToXyz(fz);

            // Convert XYZ to RGB (sRGB matrix)
            float r3 = x * 3.2404542f + y * -1.5371385f + z * -0.4985314f;
            float g3 = x * -0.9692660f + y * 1.8760108f + z * 0.0415560f;
            float b3 = x * 0.0556434f + y * -0.2040259f + z * 1.0572252f;

            // Clamp RGB values to [0, 1]
            r3 = std::clamp(r3, 0.0f, 1.0f);
            g3 = std::clamp(g3, 0.0f, 1.0f);
            b3 = std::clamp(b3, 0.0f, 1.0f);

            // Convert RGB to CMYK (simplified model)
            float ck = 1.0f - std::max({r3, g3, b3});
            float cc = (1.0f - r3 - ck) / (1.0f - ck);
            float cm = (1.0f - g3 - ck) / (1.0f - ck);
            float cy = (1.0f - b3 - ck) / (1.0f - ck);

            // Clamp CMYK values to [0, 1] and scale to [0, 255]
            cmykData[i * 4] = static_cast<uchar>(std::clamp(cc, 0.0f, 1.0f) * 255.0f);
            cmykData[i * 4 + 1] = static_cast<uchar>(std::clamp(cm, 0.0f, 1.0f) * 255.0f);
            cmykData[i * 4 + 2] = static_cast<uchar>(std::clamp(cy, 0.0f, 1.0f) * 255.0f);
            cmykData[i * 4 + 3] = static_cast<uchar>(std::clamp(ck, 0.0f, 1.0f) * 255.0f);
        }

        return cmykDataBuf;
    }

    // Create a standard LAB profile
    cmsHPROFILE labProfile = cmsCreateLab4Profile(nullptr);
    if (!labProfile) {
        cmsCloseProfile(cmykProfile);
        qWarning() << "Failed to create LAB profile";
        return QByteArray();
    }

    // Create a transform from LAB to CMYK
    cmsHTRANSFORM transform = cmsCreateTransform(
        labProfile,              // Input profile (LAB)
        TYPE_Lab_FLT,           // Input format (LAB as float)
        cmykProfile,            // Output profile (CMYK)
        TYPE_CMYK_8,            // Output format (CMYK as 8-bit)
        INTENT_PERCEPTUAL,      // Rendering intent
        0                       // Flags (0 for default)
        );

    if (!transform) {
        qWarning() << "Failed to create color transform";
        cmsCloseProfile(labProfile);
        cmsCloseProfile(cmykProfile);
        return QByteArray();
    }

    // Prepare input and output buffers
    const int pixelCount = labDataBuf.size() / (3 * sizeof(float)); // 3 channels (L, a, b)
    QByteArray cmykDataBuf(pixelCount * 4, Qt::Uninitialized);     // 4 channels (C, M, Y, K)

    // Perform the conversion
    cmsDoTransform(
        transform,                              // Transform handle
        labDataBuf.constData(),                 // Input LAB data
        cmykDataBuf.data(),                     // Output CMYK data
        pixelCount                              // Number of pixels
        );

    // Clean up
    cmsDeleteTransform(transform);
    cmsCloseProfile(labProfile);
    cmsCloseProfile(cmykProfile);

    return cmykDataBuf;
}

QByteArray resizeCMYKData(const QByteArray &cmykDataBuf, const TagJobInfoRecord &jobInfo) {
    if (cmykDataBuf.isEmpty() || jobInfo.width <= 0 || jobInfo.height <= 0 || jobInfo.xTimes <= 0 || jobInfo.yTimes <= 0) {
        qWarning() << "Invalid input parameters";
        return QByteArray();
    }

    // Calculate new dimensions
    int newWidth = static_cast<int>(std::round(jobInfo.width * jobInfo.xTimes));
    int newHeight = static_cast<int>(std::round(jobInfo.height * jobInfo.yTimes));

    if (newWidth <= 0 || newHeight <= 0) {
        qWarning() << "Invalid output dimensions";
        return QByteArray();
    }

    // Create output buffer
    QByteArray resizedCmykDataBuf(newWidth * newHeight * 4, Qt::Uninitialized);

    // Get pointers to input and output data
    const uchar* inputData = reinterpret_cast<const uchar*>(cmykDataBuf.constData());
    uchar* outputData = reinterpret_cast<uchar*>(resizedCmykDataBuf.data());

    // Function to process a single row
    auto processRow = [&](int y) {
        for (int x = 0; x < newWidth; ++x) {
            // Calculate corresponding coordinates in the original image
            float srcX = x / jobInfo.xTimes;
            float srcY = y / jobInfo.yTimes;

            // Get the four neighboring pixels
            int x1 = static_cast<int>(srcX);
            int y1 = static_cast<int>(srcY);
            int x2 = std::min(x1 + 1, jobInfo.width - 1);
            int y2 = std::min(y1 + 1, jobInfo.height - 1);

            // Calculate weights for interpolation
            float xWeight = srcX - x1;
            float yWeight = srcY - y1;

            // Process each channel (C, M, Y, K)
            for (int c = 0; c < 4; ++c) {
                // Perform bilinear interpolation
                float v11 = inputData[(y1 * jobInfo.width + x1) * 4 + c];
                float v21 = inputData[(y1 * jobInfo.width + x2) * 4 + c];
                float v12 = inputData[(y2 * jobInfo.width + x1) * 4 + c];
                float v22 = inputData[(y2 * jobInfo.width + x2) * 4 + c];

                float v1 = v11 * (1 - xWeight) + v21 * xWeight;
                float v2 = v12 * (1 - xWeight) + v22 * xWeight;
                float value = v1 * (1 - yWeight) + v2 * yWeight;

                // Clamp the value to the valid range [0, 255]
                outputData[(y * newWidth + x) * 4 + c] = static_cast<uchar>(std::clamp(value, 0.0f, 255.0f));
            }
        }
    };

    // Use a thread pool for parallel row processing
    QThreadPool pool;
    pool.setMaxThreadCount(QThread::idealThreadCount()); // Use all available cores

    QVector<int> rows(newHeight);
    for (int y = 0; y < newHeight; ++y) {
        rows[y] = y;
    }

    QtConcurrent::blockingMap(rows, processRow);

    return resizedCmykDataBuf;
}

QByteArray floydSteinbergDitherFloat(const QByteArray &cmykDataBuf, TagJobInfoRecord &jobInfo) {
    if (cmykDataBuf.isEmpty() || jobInfo.width <= 0 || jobInfo.height <= 0 || jobInfo.level < 2 || jobInfo.level > 4) {
        qWarning() << "Invalid input parameters";
        return QByteArray();
    }

    // Calculate bytes per line and buffer size
    int pixelsPerByte = (jobInfo.level == 2) ? 8 : 4;
    jobInfo.bytePerLine = (jobInfo.width + pixelsPerByte - 1) / pixelsPerByte;
    jobInfo.outputBuffSize = jobInfo.bytePerLine * jobInfo.height * 4;

    // Create output buffer
    QByteArray ditheringDataBuf(jobInfo.outputBuffSize, 0);
    uchar* outputData = reinterpret_cast<uchar*>(ditheringDataBuf.data());

    // Create a non-const copy of the input data
    QByteArray inputDataCopy = cmykDataBuf;
    uchar* inputData = reinterpret_cast<uchar*>(inputDataCopy.data());

    // Precompute quantization levels
    const int maxQuantizedValue = jobInfo.level - 1;
    const float quantizationStep = 255.0f / maxQuantizedValue;

    // Function to process a single channel
    auto processChannel = [&](int c) {
        QVector<float> errorBuffer(jobInfo.width * jobInfo.height, 0.0f);

        for (int y = 0; y < jobInfo.height; ++y) {
            for (int x = 0; x < jobInfo.width; ++x) {
                int inputIndex = (y * jobInfo.width + x) * 4 + c;
                float oldPixel = static_cast<float>(inputData[inputIndex]) + errorBuffer[y * jobInfo.width + x];

                // Quantize the pixel
                float newPixel = std::round(oldPixel / quantizationStep) * quantizationStep;
                newPixel = std::clamp(newPixel, 0.0f, 255.0f);

                // Calculate quantization error
                float error = oldPixel - newPixel;

                // Distribute the error to neighboring pixels
                if (x + 1 < jobInfo.width) {
                    errorBuffer[y * jobInfo.width + (x + 1)] += error * 7.0f / 16.0f;
                }
                if (y + 1 < jobInfo.height) {
                    if (x - 1 >= 0) {
                        errorBuffer[(y + 1) * jobInfo.width + (x - 1)] += error * 3.0f / 16.0f;
                    }
                    errorBuffer[(y + 1) * jobInfo.width + x] += error * 5.0f / 16.0f;
                    if (x + 1 < jobInfo.width) {
                        errorBuffer[(y + 1) * jobInfo.width + (x + 1)] += error * 1.0f / 16.0f;
                    }
                }

                // Pack the pixel into the output buffer
                int outputIndex = (y * jobInfo.bytePerLine * 4) + (c * jobInfo.bytePerLine) + (x / pixelsPerByte);
                int shift = (x % pixelsPerByte) * (8 / pixelsPerByte);
                uchar quantizedValue = static_cast<uchar>(std::round((newPixel / 255.0f * maxQuantizedValue)));
                outputData[outputIndex] &= ~(0xFF << shift);
                outputData[outputIndex] |= (quantizedValue << shift);
            }
        }
    };

    // Use a thread pool for parallel channel processing
    QVector<int> channels = {0, 1, 2, 3}; // Channels: C, M, Y, K
    QtConcurrent::blockingMap(channels, processChannel);

    return ditheringDataBuf;
}

    QByteArray convertRGBtoLAB(const QImage &image,  TagJobInfoRecord &jobInfo) {
    if (image.isNull()) {
        qWarning() << "Invalid input image";
        return QByteArray();
    }

    // Convert to 32-bit ARGB format for consistent processing
    QImage convertedImage = image.convertToFormat(QImage::Format_ARGB32);

    // Ensure contiguous memory storage
    if (convertedImage.bytesPerLine() != convertedImage.width() * 4) {
        convertedImage = QImage(convertedImage.constBits(),
                                convertedImage.width(),
                                convertedImage.height(),
                                convertedImage.width() * 4,
                                convertedImage.format()).copy();
        if (convertedImage.isNull()) {
            qWarning() << "Failed to create contiguous image copy";
            return QByteArray();
        }
    }

    cmsHPROFILE rgbProfile = cmsOpenProfileFromFile(jobInfo.importRGBProfile.toLocal8Bit().constData(), "r");
    if (jobInfo.importRGBProfile.isEmpty() || !rgbProfile) {
        const int width = convertedImage.width();
        const int height = convertedImage.height();
        const int totalPixels = width * height;

        QByteArray labBuffer(totalPixels * 3 * sizeof(float), Qt::Uninitialized); // 3 channels (L, a, b)
        float* labData = reinterpret_cast<float*>(labBuffer.data());

        const quint32* pixels = reinterpret_cast<const quint32*>(convertedImage.constBits());

        for (int i = 0; i < totalPixels; ++i) {
            const quint32 pixel = pixels[i];

            // Extract RGB components
            float r3 = qRed(pixel) / 255.0f;
            float g3 = qGreen(pixel) / 255.0f;
            float b3 = qBlue(pixel) / 255.0f;

            // Convert sRGB to linear RGB
            auto srgbToLinear = [](float value) {
                return (value <= 0.04045f) ? value / 12.92f : std::pow((value + 0.055f) / 1.055f, 2.4f);
            };

            r3 = srgbToLinear(r3);
            g3 = srgbToLinear(g3);
            b3 = srgbToLinear(b3);

            // Convert linear RGB to XYZ (sRGB matrix)
            float x = r3 * 0.4124564f + g3 * 0.3575761f + b3 * 0.1804375f;
            float y = r3 * 0.2126729f + g3 * 0.7151522f + b3 * 0.0721750f;
            float z = r3 * 0.0193339f + g3 * 0.1191920f + b3 * 0.9503041f;

            // Convert XYZ to Lab
            auto xyzToLab = [](float value) {
                return (value > 0.008856f) ? std::pow(value, 1.0f / 3.0f) : (7.787f * value) + (16.0f / 116.0f);
            };

            float xn = 0.95047f; // D65 white point
            float yn = 1.00000f;
            float zn = 1.08883f;

            float fx = xyzToLab(x / xn);
            float fy = xyzToLab(y / yn);
            float fz = xyzToLab(z / zn);

            labData[i * 3] = std::max(0.0f, 116.0f * fy - 16.0f); // L*
            labData[i * 3 + 1] = 500.0f * (fx - fy);              // a*
            labData[i * 3 + 2] = 200.0f * (fy - fz);              // b*
        }

        return labBuffer;
    }

    cmsHPROFILE labProfile = cmsCreateLab4Profile(nullptr);
    if (!labProfile) {
        cmsCloseProfile(rgbProfile);
        qWarning() << "Failed to create LAB profile";
        return QByteArray();
    }

    // Create color transformation
    cmsHTRANSFORM transform = cmsCreateTransform(rgbProfile,
                                                 TYPE_RGB_8,
                                                 labProfile,
                                                 TYPE_Lab_FLT,
                                                 INTENT_PERCEPTUAL,
                                                 0);
    cmsCloseProfile(rgbProfile);
    cmsCloseProfile(labProfile);

    if (!transform) {
        qWarning() << "Failed to create color transform";
        return QByteArray();
    }

    // Prepare input and output buffers
    const int width = convertedImage.width();
    const int height = convertedImage.height();
    const int totalPixels = width * height;

    QByteArray rgbData(totalPixels * 3, Qt::Uninitialized); // 3 channels (R, G, B)
    QByteArray labBuffer(totalPixels * 3 * sizeof(float), Qt::Uninitialized); // 3 channels (L, a, b)

    // Extract RGB components from ARGB32 pixels
    const quint32* pixels = reinterpret_cast<const quint32*>(convertedImage.constBits());
    uchar* rgbBuffer = reinterpret_cast<uchar*>(rgbData.data());

    for (int i = 0; i < totalPixels; ++i) {
        const quint32 pixel = pixels[i];
        rgbBuffer[i * 3] = qRed(pixel);    // Red component
        rgbBuffer[i * 3 + 1] = qGreen(pixel); // Green component
        rgbBuffer[i * 3 + 2] = qBlue(pixel);  // Blue component
    }

    // Transform to LAB
    cmsDoTransform(transform, rgbBuffer, labBuffer.data(), totalPixels);

    // Clean up
    cmsDeleteTransform(transform);

    return labBuffer;
}

QByteArray convertLABtoCMYK(const QByteArray &labDataBuf, TagJobInfoRecord &jobInfo) {
    if (labDataBuf.isEmpty()) {
        qWarning() << "Invalid LAB data buffer";
        return QByteArray();
    }

    // Load the CMYK profile
    cmsHPROFILE cmykProfile = cmsOpenProfileFromFile(jobInfo.importCMYKProfile.toLocal8Bit().constData(), "r");

    // If no ICC profile is provided, use theoretical Lab-to-CMYK conversion
    if (jobInfo.importCMYKProfile.isEmpty() || !cmykProfile) {
        qWarning() << "Failed to open CMYK ICC profile";
        const int pixelCount = labDataBuf.size() / (3 * sizeof(float)); // 3 channels (L, a, b)
        QByteArray cmykDataBuf(pixelCount * 4, Qt::Uninitialized);     // 4 channels (C, M, Y, K)
        uchar* cmykData = reinterpret_cast<uchar*>(cmykDataBuf.data());
        const float* labData = reinterpret_cast<const float*>(labDataBuf.constData());

        for (int i = 0; i < pixelCount; ++i) {
            // Extract Lab values
            float L = labData[i * 3];
            float a = labData[i * 3 + 1];
            float b = labData[i * 3 + 2];

            // Convert Lab to XYZ
            float fy = (L + 16.0f) / 116.0f;
            float fx = a / 500.0f + fy;
            float fz = fy - b / 200.0f;

            auto labToXyz = [](float value) {
                float cube = value * value * value;
                return (cube > 0.008856f) ? cube : (value - 16.0f / 116.0f) / 7.787f;
            };

            float xn = 0.95047f; // D65 white point
            float yn = 1.00000f;
            float zn = 1.08883f;

            float x = xn * labToXyz(fx);
            float y = yn * labToXyz(fy);
            float z = zn * labToXyz(fz);

            // Convert XYZ to RGB (sRGB matrix)
            float r3 = x * 3.2404542f + y * -1.5371385f + z * -0.4985314f;
            float g3 = x * -0.9692660f + y * 1.8760108f + z * 0.0415560f;
            float b3 = x * 0.0556434f + y * -0.2040259f + z * 1.0572252f;

            // Clamp RGB values to [0, 1]
            r3 = std::clamp(r3, 0.0f, 1.0f);
            g3 = std::clamp(g3, 0.0f, 1.0f);
            b3 = std::clamp(b3, 0.0f, 1.0f);

            // Convert RGB to CMYK (simplified model)
            float ck = 1.0f - std::max({r3, g3, b3});
            float cc = (1.0f - r3 - ck) / (1.0f - ck);
            float cm = (1.0f - g3 - ck) / (1.0f - ck);
            float cy = (1.0f - b3 - ck) / (1.0f - ck);

            // Clamp CMYK values to [0, 1] and scale to [0, 255]
            cmykData[i * 4] = static_cast<uchar>(std::clamp(cc, 0.0f, 1.0f) * 255.0f);
            cmykData[i * 4 + 1] = static_cast<uchar>(std::clamp(cm, 0.0f, 1.0f) * 255.0f);
            cmykData[i * 4 + 2] = static_cast<uchar>(std::clamp(cy, 0.0f, 1.0f) * 255.0f);
            cmykData[i * 4 + 3] = static_cast<uchar>(std::clamp(ck, 0.0f, 1.0f) * 255.0f);
        }

        return cmykDataBuf;
    }

    // Create a standard LAB profile
    cmsHPROFILE labProfile = cmsCreateLab4Profile(nullptr);
    if (!labProfile) {
        cmsCloseProfile(cmykProfile);
        qWarning() << "Failed to create LAB profile";
        return QByteArray();
    }

    // Create a transform from LAB to CMYK
    cmsHTRANSFORM transform = cmsCreateTransform(
        labProfile,              // Input profile (LAB)
        TYPE_Lab_FLT,           // Input format (LAB as float)
        cmykProfile,            // Output profile (CMYK)
        TYPE_CMYK_8,            // Output format (CMYK as 8-bit)
        INTENT_PERCEPTUAL,      // Rendering intent
        0                       // Flags (0 for default)
        );

    if (!transform) {
        qWarning() << "Failed to create color transform";
        cmsCloseProfile(labProfile);
        cmsCloseProfile(cmykProfile);
        return QByteArray();
    }

    // Prepare input and output buffers
    const int pixelCount = labDataBuf.size() / (3 * sizeof(float)); // 3 channels (L, a, b)
    QByteArray cmykDataBuf(pixelCount * 4, Qt::Uninitialized);     // 4 channels (C, M, Y, K)

    // Perform the conversion
    cmsDoTransform(
        transform,                              // Transform handle
        labDataBuf.constData(),                 // Input LAB data
        cmykDataBuf.data(),                     // Output CMYK data
        pixelCount                              // Number of pixels
        );

    // Clean up
    cmsDeleteTransform(transform);
    cmsCloseProfile(labProfile);
    cmsCloseProfile(cmykProfile);

    return cmykDataBuf;
}

QByteArray resizeCMYKData(const QByteArray &cmykDataBuf,  TagJobInfoRecord &jobInfo) {
    if (cmykDataBuf.isEmpty() || jobInfo.width <= 0 || jobInfo.height <= 0 || jobInfo.xTimes <= 0 || jobInfo.yTimes <= 0) {
        qWarning() << "Invalid input parameters";
        return QByteArray();
    }

    // Calculate new dimensions
    int newWidth = static_cast<int>(std::round(jobInfo.width * jobInfo.xTimes));
    int newHeight = static_cast<int>(std::round(jobInfo.height * jobInfo.yTimes));

    if (newWidth <= 0 || newHeight <= 0) {
        qWarning() << "Invalid output dimensions";
        return QByteArray();
    }

    // Create output buffer
    QByteArray resizedCmykDataBuf(newWidth * newHeight * 4, Qt::Uninitialized);

    // Get pointers to input and output data
    const uchar* inputData = reinterpret_cast<const uchar*>(cmykDataBuf.constData());
    uchar* outputData = reinterpret_cast<uchar*>(resizedCmykDataBuf.data());

    // Function to process a single row
    auto processRow = [&](int y) {
        for (int x = 0; x < newWidth; ++x) {
            // Calculate corresponding coordinates in the original image
            float srcX = x / jobInfo.xTimes;
            float srcY = y / jobInfo.yTimes;

            // Get the four neighboring pixels
            int x1 = static_cast<int>(srcX);
            int y1 = static_cast<int>(srcY);
            int x2 = std::min(x1 + 1, jobInfo.width - 1);
            int y2 = std::min(y1 + 1, jobInfo.height - 1);

            // Calculate weights for interpolation
            float xWeight = srcX - x1;
            float yWeight = srcY - y1;

            // Process each channel (C, M, Y, K)
            for (int c = 0; c < 4; ++c) {
                // Perform bilinear interpolation
                float v11 = inputData[(y1 * jobInfo.width + x1) * 4 + c];
                float v21 = inputData[(y1 * jobInfo.width + x2) * 4 + c];
                float v12 = inputData[(y2 * jobInfo.width + x1) * 4 + c];
                float v22 = inputData[(y2 * jobInfo.width + x2) * 4 + c];

                float v1 = v11 * (1 - xWeight) + v21 * xWeight;
                float v2 = v12 * (1 - xWeight) + v22 * xWeight;
                float value = v1 * (1 - yWeight) + v2 * yWeight;

                // Clamp the value to the valid range [0, 255]
                outputData[(y * newWidth + x) * 4 + c] = static_cast<uchar>(std::clamp(value, 0.0f, 255.0f));
            }
        }
    };

    // Use a thread pool for parallel row processing
    QThreadPool pool;
    pool.setMaxThreadCount(QThread::idealThreadCount()); // Use all available cores

    QVector<int> rows(newHeight);
    for (int y = 0; y < newHeight; ++y) {
        rows[y] = y;
    }

    QtConcurrent::blockingMap(rows, processRow);

    return resizedCmykDataBuf;
}


QByteArray orderedDither(const QByteArray &cmykDataBuf, TagJobInfoRecord &jobInfo) {
    if (cmykDataBuf.isEmpty() || jobInfo.width <= 0 || jobInfo.height <= 0 || jobInfo.level < 2 || jobInfo.level > 4) {
        qWarning() << "Invalid input parameters";
        return QByteArray();
    }

    // Calculate bytes per line for each color
    int pixelsPerByte = (jobInfo.level == 2) ? 8 : 4;
    jobInfo.bytePerLine = (jobInfo.width + pixelsPerByte - 1) / pixelsPerByte;
    jobInfo.outputBuffSize = jobInfo.bytePerLine * jobInfo.height * 4;

    // Create output buffer
    QByteArray ditheringDataBuf(jobInfo.outputBuffSize, 0);
    uchar* outputData = reinterpret_cast<uchar*>(ditheringDataBuf.data());

    // Use an 8x8 Bayer threshold map for better quality
    const int bayerMap[8][8] = {
        {  0, 32,  8, 40,  2, 34, 10, 42 },
        { 48, 16, 56, 24, 50, 18, 58, 26 },
        { 12, 44,  4, 36, 14, 46,  6, 38 },
        { 60, 28, 52, 20, 62, 30, 54, 22 },
        {  3, 35, 11, 43,  1, 33,  9, 41 },
        { 51, 19, 59, 27, 49, 17, 57, 25 },
        { 15, 47,  7, 39, 13, 45,  5, 37 },
        { 63, 31, 55, 23, 61, 29, 53, 21 }
    };

    // Normalize the threshold map to the range [0, 255]
    const float bayerScale = 255.0f / 64.0f;
    const int maxQuantizedValue = jobInfo.level - 1;
    const float quantizationStep = 255.0f / maxQuantizedValue;

    // Iterate over each pixel
    for (int y = 0; y < jobInfo.height; ++y) {
        for (int x = 0; x < jobInfo.width; ++x) {
            // Process each channel (C, M, Y, K)
            for (int c = 0; c < 4; ++c) {
                int inputIndex = (y * jobInfo.width + x) * 4 + c;
                float oldPixel = static_cast<float>(outputData[inputIndex]);

                // Apply the Bayer threshold
                int threshold = bayerMap[y % 8][x % 8];
                float normalizedThreshold = threshold * bayerScale;

                // Quantize the pixel based on the level
                float newPixel = std::round(oldPixel / quantizationStep) * quantizationStep;
                newPixel = std::clamp(newPixel, 0.0f, 255.0f);

                // Pack the pixel into the output buffer
                int outputIndex = (y * jobInfo.bytePerLine * 4) + (c * jobInfo.bytePerLine) + (x / pixelsPerByte);
                int shift = (x % pixelsPerByte) * (8 / pixelsPerByte);
                uchar quantizedValue = static_cast<uchar>(std::round((newPixel / 255.0f * maxQuantizedValue)));

                outputData[outputIndex] &= ~(0xFF << shift);
                outputData[outputIndex] |= (quantizedValue << shift);
            }
        }
    }

    return ditheringDataBuf;
}

int CreatePrnFile(const QString &prnFilePath, const QByteArray &outDataBuf, const TagJobInfoRecord &jobInfo) {
    // Open the file for writing (truncate if it exists)
    QFile prnFile(prnFilePath);
    if (!prnFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Failed to open .prn file for writing:" << prnFilePath;
        return -1;
    }

    // Create a data stream for writing to the file
    QDataStream out(&prnFile);
    out.setByteOrder(QDataStream::LittleEndian); // Ensure little-endian byte order

    // Populate the header structure
    tagHeadRecord_1 header;
    header.nSignature = 0x5555; // Signature
    header.nXDPI = int(jobInfo.xImageResolution + 0.5);
    header.nYDPI = int(jobInfo.yImageResolution + 0.5);
    header.nBytesPerLine = (jobInfo.bytePerLine + 3) / 4 * 4;
    header.nHeight = jobInfo.height;
    header.nWidth = jobInfo.width;
    header.nPaperWidth = 0;     // Not used
    header.nColors = 4;         // CMYK
    header.nBits = (jobInfo.nLevel + 1) / 2;
    header.nReserved[0] = 0;    // Pass Number
    header.nReserved[1] = 0;    // vsdMode
    header.nReserved[2] = 0;    // Reserved

    // Write the header to the file
    out.writeRawData(reinterpret_cast<const char*>(&header), sizeof(header));

    // Calculate padding size (if needed)
    int paddingSize = (4 - (jobInfo.bytePerLine % 4)) % 4; // Ensure bytePerLine is a multiple of 4

    // Write the data in YMCK order
    const uchar* inputData = reinterpret_cast<const uchar*>(outDataBuf.constData());
    for (int y = 0; y < jobInfo.height; ++y) {
        // Write K (Black) data
        out.writeRawData(reinterpret_cast<const char*>(inputData + (y * jobInfo.bytePerLine * 4) + 3 * jobInfo.bytePerLine), jobInfo.bytePerLine);
        if (paddingSize > 0) {
            out.writeRawData("\0\0\0", paddingSize); // Add padding
        }

        // Write C (Cyan) data
        out.writeRawData(reinterpret_cast<const char*>(inputData + (y * jobInfo.bytePerLine * 4) + 0 * jobInfo.bytePerLine), jobInfo.bytePerLine);
        if (paddingSize > 0) {
            out.writeRawData("\0\0\0", paddingSize); // Add padding
        }

        // Write M (Magenta) data
        out.writeRawData(reinterpret_cast<const char*>(inputData + (y * jobInfo.bytePerLine * 4) + 1 * jobInfo.bytePerLine), jobInfo.bytePerLine);
        if (paddingSize > 0) {
            out.writeRawData("\0\0\0", paddingSize); // Add padding
        }

        // Write Y (Yellow) data
        out.writeRawData(reinterpret_cast<const char*>(inputData + (y * jobInfo.bytePerLine * 4) + 2 * jobInfo.bytePerLine), jobInfo.bytePerLine);
        if (paddingSize > 0) {
            out.writeRawData("\0\0\0", paddingSize); // Add padding
        }
    }

    // Close the file
    prnFile.close();

    return 0; // Success
}

