set(kmidbackend_VERSION "1.0.0")
set(kmidbackend_SOVERSION "1")

set(library_HEADERS
    backendloader.h
    backend.h
    kmidmacros.h
    midiobject.h
    midioutput.h
    midimapper.h
)

set(library_SOURCES
    backendloader.cpp
    backend.cpp
    midiobject.cpp
    midioutput.cpp
    midimapper.cpp
)

kconfig_add_kcfg_files(library_SOURCES settings.kcfgc)

add_library(kmidbackend SHARED ${library_SOURCES} ${library_HEADERS})
target_compile_definitions(kmidbackend PRIVATE MAKE_KMIDBACKEND_LIB)

target_link_libraries(kmidbackend
    KF5::CoreAddons
    KF5::ConfigCore
    KF5::ConfigGui
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
)

set_target_properties(kmidbackend PROPERTIES
    VERSION ${kmidbackend_VERSION}
    SOVERSION ${kmidbackend_SOVERSION}
)

install(TARGETS kmidbackend ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${library_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/kmid)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/settings.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/kmid)
install(FILES kmid_backend.desktop
    DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})
install(FILES kmid.kcfg
    DESTINATION ${KDE_INSTALL_KCFGDIR})
