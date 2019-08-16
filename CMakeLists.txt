
cmake_minimum_required(VERSION 3.14)
project(OWStreamRecordEx)
set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_PREFIX_PATH ${Qt}/lib/cmake)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Tesseract REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)

add_executable(OWStreamRecordEx src/main.cpp src/mainwindow.cpp src/mainwindow.h src/mainwindow.ui src/ScreenShooter.cpp src/ScreenShooter.h src/SHMEMReader.cpp src/SHMEMReader.h src/VisionThread.cpp src/VisionThread.h res/owsrx.rc res/icon.ico)

target_link_libraries(OWStreamRecordEx Qt5::Core)
target_link_libraries(OWStreamRecordEx Qt5::Gui)
target_link_libraries(OWStreamRecordEx Qt5::Widgets)
target_link_libraries(OWStreamRecordEx libtesseract)

