set(kmidbackend_VERSION "1.0.0")
set(kmidbackend_SOVERSION "1")

set ( library_HEADERS
    backendloader.h
    backend.h
    kmidmacros.h
    midiobject.h
    midioutput.h
    midimapper.h
)

set ( library_SOURCES
    backendloader.cpp
    backend.cpp
    midiobject.cpp
    midioutput.cpp
    midimapper.cpp
)

kde4_add_kcfg_files( library_SOURCES settings.kcfgc )

kde4_add_library ( kmidbackend SHARED ${library_SOURCES} ${library_HEADERS} )

target_link_libraries( kmidbackend ${KDE4_KDECORE_LIBRARY} ${KDE4_KDEUI_LIBS} )

set_target_properties( kmidbackend PROPERTIES
    VERSION ${kmidbackend_VERSION}
    SOVERSION ${kmidbackend_SOVERSION}
)

install( TARGETS kmidbackend ${INSTALL_TARGETS_DEFAULT_ARGS} )
install( FILES ${library_HEADERS} 
    DESTINATION ${INCLUDE_INSTALL_DIR}/kmid )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/settings.h 
    DESTINATION ${INCLUDE_INSTALL_DIR}/kmid )
install( FILES kmid_backend.desktop 
    DESTINATION ${SERVICETYPES_INSTALL_DIR} )
install( FILES kmid.kcfg 
    DESTINATION ${KCFG_INSTALL_DIR} )

find_package(Doxygen)
if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.cmake 
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    add_custom_target( apidox
        COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile 
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
endif(DOXYGEN_FOUND)
