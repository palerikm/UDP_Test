set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

# Find the QtWidgets library
find_package(Qt5 COMPONENTS Core Widgets Charts Quick REQUIRED)


add_executable( udpqtclient udpqtclient.cpp
          JitterQChartWidget.cpp JitterQChartWidget.h
        ControlWindow.cpp ControlWindow.h
        TestRunWorker.cpp TestRunWorker.h
        ../src/iphelper.c ../include/iphelper.h
         ../src/sockethelper.c ../include/sockethelper.h
        ../src/packettest.c ../include/packettest.h
        ../src/clientcommon.c ../include/clientcommon.h
        ../src/udptestcommon.c ../include/udptestcommon.h
        )
target_link_libraries( udpqtclient PRIVATE sockaddrutil hashmap_c Qt5::Core Qt5::Charts Qt5::Quick)

set_target_properties(udpqtclient PROPERTIES INSTALL_RPATH "${install_dir}/lib")
set_target_properties(udpqtclient PROPERTIES BUILD_WITH_INSTALL_RPATH ON)

include_directories ( ../include ${hashmap_c_SOURCE_DIR})
link_directories ( ${CHECK_LIBRARY_DIRS} )