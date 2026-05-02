include_directories(
    ../library
    ../widgets
    ${CMAKE_CURRENT_BINARY_DIR}/../library
    ${CMAKE_CURRENT_BINARY_DIR}
)

configure_file(config.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

###############
# main target #
###############

set(kmid_SRCS
    channels.cpp
    instrumentset.cpp
    kmid2.cpp
    main.cpp
    pianola.cpp
    playlistdialog.cpp
    ../widgets/keylabel.cpp
    ../widgets/pianokey.cpp
    ../widgets/pianokeybd.cpp
    ../widgets/pianoscene.cpp
    ../widgets/rhythmview.cpp
    ../widgets/timelabel.cpp
    ../widgets/vumeter.cpp
)

qt5_add_resources(kmid_SRCS ../widgets/pianokeybd.qrc)
qt5_add_dbus_adaptor(kmid_SRCS org.kde.KMid.xml kmid2.h KMid2)

add_executable(kmid ${kmid_SRCS})

target_link_libraries(kmid
    Qt5::Svg
    KF5::XmlGui
    KF5::KIOCore
    KF5::KIOFileWidgets
    KF5::KIOWidgets
    KF5::I18n
    KF5::IconThemes
    KF5::TextWidgets
    KF5::ConfigWidgets
    KF5::WidgetsAddons
    Qt5::Widgets
    Qt5::DBus
    Qt5::PrintSupport
    kmidbackend
)

install(TARGETS kmid ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES kmid.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES kmidui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/kmid)
install(FILES org.kde.KMid.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})

#########
# kpart #
#########

set(kmid_part_SRCS
    kmid_partview.cpp
    kmid_part.cpp
)

qt5_add_dbus_adaptor(kmid_part_SRCS org.kde.KMidPart.xml kmid_part.h KMidPart)

add_library(kmid_part MODULE ${kmid_part_SRCS})

target_link_libraries(kmid_part
    KF5::XmlGui
    KF5::Parts
    KF5::I18n
    KF5::IconThemes
    KF5::ConfigWidgets
    KF5::WidgetsAddons
    Qt5::Widgets
    Qt5::DBus
    kmidbackend
)

install(TARGETS kmid_part DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES kmid_part.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES kmid_part.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/kmid_part)
install(FILES org.kde.KMidPart.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})
