set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.10" CACHE STRING "Minimum OS X deployment version")
if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

set(APP_NAME udpqtclient)

set(prefix "${APP_NAME}.app/Contents")
set(INSTALL_RUNTIME_DIR "${prefix}/MacOS")
set(INSTALL_CMAKE_DIR "${prefix}/Resources")


# Find the QtWidgets library
find_package(Qt5 COMPONENTS Core Widgets Charts Quick REQUIRED)


add_executable( ${APP_NAME} MACOSX_BUNDLE udpqtclient.cpp
          JitterQChartWidget.cpp JitterQChartWidget.h
        ControlWindow.cpp ControlWindow.h
        TestRunWorker.cpp TestRunWorker.h
        ../src/iphelper.c ../include/iphelper.h
         ../src/sockethelper.c ../include/sockethelper.h
        ../src/clientcommon.c ../include/clientcommon.h
        )
target_link_libraries( ${APP_NAME} PRIVATE sockaddrutil hashmap_c udpjitter Qt5::Widgets Qt5::Charts Qt5::Core)



include_directories( ../include ${hashmap_c_SOURCE_DIR} ${sockaddrutil_SOURCE_DIR}/include)



# based on code from CMake's QtDialog/CMakeLists.txt
macro(install_qt5_plugin _qt_plugin_name _qt_plugins_var _prefix)
    get_target_property(_qt_plugin_path "${_qt_plugin_name}" LOCATION)
    if(EXISTS "${_qt_plugin_path}")
        get_filename_component(_qt_plugin_file "${_qt_plugin_path}" NAME)
        get_filename_component(_qt_plugin_type "${_qt_plugin_path}" PATH)
        get_filename_component(_qt_plugin_type "${_qt_plugin_type}" NAME)
        set(_qt_plugin_dest "${_prefix}/PlugIns/${_qt_plugin_type}")
        install(FILES "${_qt_plugin_path}"
                DESTINATION "${_qt_plugin_dest}")
        set(${_qt_plugins_var}
                "${${_qt_plugins_var}};\$ENV{DEST_DIR}\${CMAKE_INSTALL_PREFIX}/${_qt_plugin_dest}/${_qt_plugin_file}")
    else()
        message(FATAL_ERROR "QT plugin ${_qt_plugin_name} not found")
    endif()
endmacro()

install_qt5_plugin("Qt5::QCocoaIntegrationPlugin" QT_PLUGINS ${prefix})


#set(CPACK_MONOLITHIC_INSTALL 1)
set(CMAKE_MACOSX_BUNDLE on)
set(CMAKE_MACOSX_RPATH ON)


set(CPACK_PACKAGE_NAME ${APP_NAME})
set(CPACK_PACKAGE_VENDOR "CMake.org")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simple UDP Testing tool")
set(CPACK_BUNDLE_NAME ${APP_NAME})


file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
        "[Paths]\nPlugins = ${_qt_plugin_dir}\n")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
        DESTINATION "${INSTALL_CMAKE_DIR}")


# Destination paths below are relative to ${CMAKE_INSTALL_PREFIX}
install(TARGETS ${APP_NAME}
        BUNDLE DESTINATION . COMPONENT Runtime
        RUNTIME DESTINATION bin COMPONENT Runtime
        )



# Note Mac specific extension .app
#set(APPS "\${CMAKE_INSTALL_PREFIX}/${APP_NAME}.app")
#Note Mac specific extension .app
set(APPS "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${APP_NAME}.app")

# Directories to look for dependencies
set(DIRS ${CMAKE_BINARY_DIR} )
# Path used for searching by FIND_XXX(), with appropriate suffixes added
if(CMAKE_PREFIX_PATH)
    foreach(dir ${CMAKE_PREFIX_PATH})
        list(APPEND DIRS "${dir}/bin" "${dir}/lib")
    endforeach()
endif()

# Append Qt's lib folder which is two levels above Qt5Widgets_DIR
list(APPEND DIRS "${Qt5Widgets_DIR}/../..")

include(InstallRequiredSystemLibraries)

message(STATUS "APPS: ${APPS}")
message(STATUS "QT_PLUGINS: ${QT_PLUGINS}")
message(STATUS "DIRS: ${DIRS}")



install(CODE "include(BundleUtilities)
    fixup_bundle(\"${APPS}\" \"${QT_PLUGINS}\" \"${DIRS}\")")


set(CPACK_GENERATOR "Bundle")
include(CPack)