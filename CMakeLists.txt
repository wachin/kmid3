cmake_minimum_required(VERSION 3.16)
project(kmid VERSION 2.4.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt5
find_package(Qt5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    DBus
    PrintSupport
    Network
    Svg
)

# Find KDE Frameworks 5
find_package(ECM 5.80 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

find_package(KF5 REQUIRED COMPONENTS
    CoreAddons
    I18n
    XmlGui
    KIO
    Parts
    Config
    ConfigWidgets
    WidgetsAddons
    TextWidgets
    IconThemes
    Notifications
)

# Linux / ALSA
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ALSA alsa>=1.0)
    if(ALSA_FOUND)
        set(WITH_ALSA 1)
        # Check for drumstick (Qt5 version, Debian package names without "2" suffix)
        pkg_check_modules(DRUMSTICK_FILE drumstick-file>=1.0)
        pkg_check_modules(DRUMSTICK_ALSA drumstick-alsa>=1.0)
        if(DRUMSTICK_ALSA_FOUND AND DRUMSTICK_FILE_FOUND)
            set(DRUMSTICK_LIBRARIES
                ${DRUMSTICK_FILE_LIBRARIES}
                ${DRUMSTICK_ALSA_LIBRARIES}
                ${ALSA_LIBRARIES}
            )
            set(DRUMSTICK_INCLUDEDIR
                ${DRUMSTICK_FILE_INCLUDE_DIRS}
                ${DRUMSTICK_ALSA_INCLUDE_DIRS}
            )
            message(STATUS "Found system drumstick: ${DRUMSTICK_FILE_VERSION}")
        else()
            message(FATAL_ERROR "drumstick-file and drumstick-alsa are required. Install libdrumstick-dev.")
        endif()
    endif()
endif()

include_directories(${DRUMSTICK_INCLUDEDIR})

add_subdirectory(library)
add_subdirectory(src)

if(ALSA_FOUND)
    add_subdirectory(alsa)
endif()

add_subdirectory(icons)
add_subdirectory(maps)
add_subdirectory(examples)
add_subdirectory(po)

find_package(KF5DocTools 5.80 QUIET)
if(KF5DocTools_FOUND)
    add_subdirectory(doc)
else()
    message(STATUS "KF5DocTools not found — skipping HTML handbooks (Debian: apt install libkf5doctools-dev)")
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
