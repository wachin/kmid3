message(STATUS "building the ALSA backend for KMid")

include_directories(
    ../library
    ${ALSA_INCLUDEDIR}
    ${DRUMSTICK_INCLUDEDIR}
    ${CMAKE_BINARY_DIR}/library
)

set(plugin_SRCS
    alsabackend.cpp
    alsamidiobject.cpp
    alsamidioutput.cpp
    externalsoftsynth.cpp
    song.cpp
    player.cpp
)

add_library(kmid_alsa MODULE ${plugin_SRCS})

target_compile_options(kmid_alsa PRIVATE -fexceptions)

target_link_libraries(kmid_alsa
    KF5::XmlGui
    KF5::KIOCore
    KF5::KIOFileWidgets
    KF5::KIOWidgets
    KF5::ConfigWidgets
    KF5::WidgetsAddons
    KF5::I18n
    KF5::ConfigCore
    ${DRUMSTICK_LIBRARIES}
    Qt5::Widgets
    Qt5::DBus
    kmidbackend
)

install(TARGETS kmid_alsa DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES kmid_alsa.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
