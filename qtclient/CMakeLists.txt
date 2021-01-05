set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

# Find the QtWidgets library
find_package(Qt5 COMPONENTS Core Widgets Charts REQUIRED)


add_executable( udpqtclient main.cpp chart.cpp )

target_link_libraries( udpqtclient Qt5::Core Qt5::Charts)