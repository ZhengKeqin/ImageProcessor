cmake_minimum_required(VERSION 3.16)

project(ImageProcessor VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt6 REQUIRED COMPONENTS Concurrent)

# Add lcms2 include directory
include_directories(${CMAKE_SOURCE_DIR}/RIP/include)

set(TS_FILES ImageProcessor_en_US.ts)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ImageProcessor
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        settingsdialog.h
        settingsdialog.cpp
        settingsdialog.ui
        RIP/src/RgbToLabConverter.cpp
        RIP/include/lcms2.h RIP/include/RgbToLabConverter.h
        ImageProcessor.pro
        RIP/include/include.h
        RIP/include/JobSettings.h
        RIP/src/JobSettings.cpp

    )
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    add_executable(ImageProcessor ${PROJECT_SOURCES})
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

# Link lcms2 library
target_link_libraries(ImageProcessor PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${CMAKE_SOURCE_DIR}/RIP/lib/liblcms2.a)
# target_link_libraries(ImageProcessor PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${CMAKE_SOURCE_DIR}/RIP/lib/libtiff.a)
target_link_libraries(ImageProcessor PRIVATE Qt${QT_VERSION_MAJOR}::Concurrent)

# Copy lcms2.dll to the build directory (if using dynamic linking)
if(WIN32)
    add_custom_command(TARGET ImageProcessor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/RIP/lib/lcms2.dll
        $<TARGET_FILE_DIR:ImageProcessor>
    )
# add_custom_command(TARGET ImageProcessor POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy
#     ${CMAKE_SOURCE_DIR}/RIP/lib/libtiff.dll
#     $<TARGET_FILE_DIR:ImageProcessor>
# )
endif()

# Finalize for Qt 6
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ImageProcessor)
endif()
