cmake_minimum_required(VERSION 3.14)
project(OWStreamRecordEx)
set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_PREFIX_PATH ${Qt}/lib/cmake)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(nlohmann_json REQUIRED)
find_package(Tesseract REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)

link_libraries(Qt5::Core Qt5::Gui Qt5::Widgets libtesseract nlohmann_json nlohmann_json::nlohmann_json)

add_executable(OWStreamRecordEx src/main.cpp src/mainwindow.cpp src/mainwindow.h src/mainwindow.ui res/owsrx.rc res/icon.ico src/VisionWorker.cpp src/VisionWorker.h src/MFindWindow.cpp src/MFindWindow.h src/Config.cpp src/Config.h)
