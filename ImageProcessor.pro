#RESOURCES += C:/QTProject/Image/images.qrc

#INCLUDEPATH += C:/QTProject/lcms2/include
# Add lcms2 include path
#INCLUDEPATH += C:/QTProject/Downloaded/lcms2-2.17/include  # Windows (adjust path as needed)
#LIBS += -LC:/QTProject/lcms2/build -llcms2
# Add lcms2 library path
#LIBS += C:/QTProject/lcms2/lib           # Windows (adjust path as needed)

# Link lcms2 library
#LIBS += -llcms2

#message("Include path: $$INCLUDEPATH")

# Add the path to the lcms2 include directory
#INCLUDEPATH += C:/QTProject/lcms2/include

# Add the path to the lcms2 library directory
#LIBS += -LC:/QTProject/lcms2/lib -llcms2

# If you're using a static library, specify the full path to the .lib file
# LIBS += C:/QTProject/lcms2/lib/lcms2.lib

#LIBS += -LC:\QTProject\lcms2\build\Desktop_Qt_6_8_2_MinGW_64_bit-Debug\debug -llcms2

# Include headers
#INCLUDEPATH += C:\QTProject\ImageProcessor\RIP\include\lcms2.h

# Link against lcms2 library
#LIBS += -LC:QTProject/ImageProcessor/RIP/lib -llcms2

# Include headers
INCLUDEPATH += C:/QTProject/ImageProcessor/RIP/include
DEPENDPATH += C:/QTProject/ImageProcessor/RIP/include

QT += concurrent

# Link against lcms2
LIBS += -LC:/QTProject/ImageProcessor/RIP/lib -llcms2
# LIBS += -LC:/QTProject/ImageProcessor/RIP/lib -ltiff

# Deploy the DLL to the build directory
#win32 {
#   QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$PWD/lib/lcms2.dll) $$quote($$OUT_PWD) $$escape_expand(\n\t)
#}
