message (STATUS "building a dummy backend for KMid")

include_directories( 
    ../library
    ${kmid_BINARY_DIR}/library
)

set( plugin_SRCS
    dummybackend.cpp
    dummymidiobject.cpp
    dummymidioutput.cpp
)

kde4_add_plugin( kmid_dummy ${plugin_SRCS} )

target_link_libraries( kmid_dummy
    ${KDE4_KDEUI_LIBS} 
    ${KDE4_KIO_LIBS}
    kmidbackend 
)

install( TARGETS kmid_dummy DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES kmid_dummy.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
