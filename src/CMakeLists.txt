
include_directories (
    ../library 
    ../widgets
    ${kmid_BINARY_DIR}/library
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

kde4_add_ui_files( kmid_SRCS prefs_midi.ui prefs_lyrics.ui )
qt4_add_resources( kmid_SRCS ../widgets/pianokeybd.qrc )
kde4_add_app_icon( kmid_SRCS "../icons/hi*-app-kmid.png" )
qt4_add_dbus_adaptor(kmid_SRCS org.kde.KMid.xml kmid2.h KMid2)

kde4_add_executable( kmid ${kmid_SRCS} )
target_link_libraries( kmid 
    ${KDE4_KDEUI_LIBS} 
    ${KDE4_KIO_LIBS} 
    ${KDE4_KFILE_LIBS} 
    kmidbackend
)

install( TARGETS kmid ${INSTALL_TARGETS_DEFAULT_ARGS} )
install( FILES kmid.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES kmidui.rc DESTINATION ${DATA_INSTALL_DIR}/kmid )
install( FILES org.kde.KMid.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )

#########
# kpart #
#########

set(kmid_part_SRCS
   kmid_partview.cpp
   kmid_part.cpp
)

qt4_add_dbus_adaptor(kmid_part_SRCS org.kde.KMidPart.xml kmid_part.h KMidPart)
kde4_add_plugin(kmid_part ${kmid_part_SRCS})
target_link_libraries( kmid_part 
    ${KDE4_KDEUI_LIBS} 
    ${KDE4_KPARTS_LIBS} 
    kmediaplayer
    kmidbackend
)

install( TARGETS kmid_part DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES kmid_part.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES kmid_part.rc DESTINATION ${DATA_INSTALL_DIR}/kmid_part )
install( FILES org.kde.KMidPart.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
