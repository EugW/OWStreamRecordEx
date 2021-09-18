cmake_minimum_required(VERSION 3.14)
project(OWStreamRecordEx)
set(CMAKE_CXX_STANDARD 17)


find_package(nlohmann_json REQUIRED)
find_package(Tesseract REQUIRED)
find_package(Qt6Core REQUIRED)
find_package(Qt6Gui REQUIRED)
find_package(Qt6Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB OWSRX_DEP CONFIGURE_DEPENDS "src/*.cpp" "src/*.h" "src/DXGI/*.cpp" "src/DXGI/*.h")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    SET(APPTYPE WIN32)
endif()

add_executable(OWStreamRecordEx ${APPTYPE} ${OWSRX_DEP} src/mainwindow.ui res/owsrx.rc res/icon.ico)

target_link_libraries(OWStreamRecordEx Qt6::Core Qt6::Gui Qt6::Widgets libtesseract nlohmann_json nlohmann_json::nlohmann_json)
