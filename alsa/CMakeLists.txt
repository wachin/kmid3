message (STATUS "building the ALSA backend for KMid")

include_directories( 
    ../library
    ${ALSA_INCLUDEDIR}
    ${DRUMSTICK_INCLUDEDIR}
    ${kmid_BINARY_DIR}/library
)

set( plugin_SRCS
    alsabackend.cpp
    alsamidiobject.cpp
    alsamidioutput.cpp
    externalsoftsynth.cpp
    song.cpp
    player.cpp
)

kde4_add_ui_files( plugin_SRCS prefs_progs.ui )

kde4_add_plugin( kmid_alsa ${plugin_SRCS} ) 

target_link_libraries( kmid_alsa 
    ${KDE4_KDEUI_LIBS} 
    ${KDE4_KIO_LIBS} 
    ${DRUMSTICK_LIBRARIES}
    kmidbackend
)

install( TARGETS kmid_alsa DESTINATION ${PLUGIN_INSTALL_DIR})
install( FILES kmid_alsa.desktop DESTINATION ${SERVICES_INSTALL_DIR})
