CMAKE_MINIMUM_REQUIRED( VERSION 2.8 ) 
PROJECT( hnode ) 

SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/" )

FIND_PACKAGE(GLIB REQUIRED)
FIND_PACKAGE(GOBJECT REQUIRED)
FIND_PACKAGE(AVAHI REQUIRED)
FIND_PACKAGE(AVAHIGLIB REQUIRED)
FIND_PACKAGE(LIBDAEMON REQUIRED)

INCLUDE_DIRECTORIES( ${GLIB_INCLUDE_DIRS} )
INCLUDE_DIRECTORIES( ${GOBJECT_INCLUDE_DIRS} )
INCLUDE_DIRECTORIES( ${AVAHI_INCLUDE_DIRS} )
INCLUDE_DIRECTORIES( ${AVAHIGLIB_INCLUDE_DIRS} )
INCLUDE_DIRECTORIES( ${LIBDAEMON_INCLUDE_DIRS} )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/src )

SET( HNODE_LIBNODE_SRC
     ${CMAKE_SOURCE_DIR}/src/hnode-nodeobj.c 
     ${CMAKE_SOURCE_DIR}/src/hnode-pktsrc.c 
     ${CMAKE_SOURCE_DIR}/src/hnode-packet.c 
     ${CMAKE_SOURCE_DIR}/src/hnode-pktaddr.c
)

SET( HNODE_LIBSERVER_SRC
     ${CMAKE_SOURCE_DIR}/src/hnode-pktsrc.c 
     ${CMAKE_SOURCE_DIR}/src/hnode-packet.c 
     ${CMAKE_SOURCE_DIR}/src/hnode-pktaddr.c 
     ${CMAKE_SOURCE_DIR}/src/hnode-uid.c 
     ${CMAKE_SOURCE_DIR}/src/hnode-provider.c 
     ${CMAKE_SOURCE_DIR}/src/hnode-provider-epoint.c 
     ${CMAKE_SOURCE_DIR}/src/hnode-srvobj.c 
     ${CMAKE_SOURCE_DIR}/src/hnode-srvobj-client.c
     ${CMAKE_SOURCE_DIR}/src/hnode-signal-source.c
     ${CMAKE_SOURCE_DIR}/src/hnode-logging.c
)

SET( HNODE_LIBBROWSER_SRC
     ${CMAKE_SOURCE_DIR}/src/hnode-browser.c 
     ${CMAKE_SOURCE_DIR}/src/hnode-browser-mpoint.c 
     ${CMAKE_SOURCE_DIR}/src/hnode-browser-hnode.c 
     ${CMAKE_SOURCE_DIR}/src/hnode-pktsrc.c 
     ${CMAKE_SOURCE_DIR}/src/hnode-packet.c 
     ${CMAKE_SOURCE_DIR}/src/hnode-pktaddr.c 
     ${CMAKE_SOURCE_DIR}/src/hnode-uid.c 
     ${CMAKE_SOURCE_DIR}/src/hnode-provider.c 
     ${CMAKE_SOURCE_DIR}/src/hnode-provider-epoint.c
)

SET( HNODE_PUBLIC_HEADERS
	 ${CMAKE_SOURCE_DIR}/src/hnode-nodeobj.h	
     ${CMAKE_SOURCE_DIR}/src/hnode-pktsrc.h 
     ${CMAKE_SOURCE_DIR}/src/hnode-srvobj.h 
	 ${CMAKE_SOURCE_DIR}/src/hnode-provider.h 
     ${CMAKE_SOURCE_DIR}/src/hnode-browser.h 
     ${CMAKE_SOURCE_DIR}/src/hnode-cfginf.h 
     ${CMAKE_SOURCE_DIR}/src/hnode-uid.h
)

ADD_LIBRARY( hnode_node SHARED ${HNODE_LIBNODE_SRC} )
TARGET_LINK_LIBRARIES( hnode_node ${GLIB_LIBRARIES} )
TARGET_LINK_LIBRARIES( hnode_node ${GOBJECT_LIBRARIES} )
TARGET_LINK_LIBRARIES( hnode_node ${AVAHI_LIBRARIES} )
TARGET_LINK_LIBRARIES( hnode_node ${AVAHIGLIB_LIBRARIES} )

ADD_LIBRARY( hnode_server SHARED ${HNODE_LIBSERVER_SRC} )
TARGET_LINK_LIBRARIES( hnode_server ${GLIB_LIBRARIES} )
TARGET_LINK_LIBRARIES( hnode_server ${GOBJECT_LIBRARIES} )
TARGET_LINK_LIBRARIES( hnode_server ${AVAHI_LIBRARIES} )
TARGET_LINK_LIBRARIES( hnode_server ${AVAHIGLIB_LIBRARIES} )
TARGET_LINK_LIBRARIES( hnode_server ${LIBDAEMON_LIBRARIES} )

ADD_LIBRARY( hnode_browser SHARED ${HNODE_LIBBROWSER_SRC} )
TARGET_LINK_LIBRARIES( hnode_browser ${GLIB_LIBRARIES} )
TARGET_LINK_LIBRARIES( hnode_browser ${GOBJECT_LIBRARIES} )
TARGET_LINK_LIBRARIES( hnode_browser ${AVAHI_LIBRARIES} )
TARGET_LINK_LIBRARIES( hnode_browser ${AVAHIGLIB_LIBRARIES} )

SET( HNODE_HMNODED_SRC
     ${CMAKE_SOURCE_DIR}/src/hmsrv.c
)

SET( HNODE_TESTNODE_SRC
     ${CMAKE_SOURCE_DIR}/src/hnode.c
)

SET( HNODE_HMCFG_SRC
     ${CMAKE_SOURCE_DIR}/src/hmcfg.c
)

ADD_EXECUTABLE( hmnoded ${HNODE_HMNODED_SRC} )
TARGET_LINK_LIBRARIES( hmnoded hnode_server )

ADD_EXECUTABLE( hnode-testnode ${HNODE_TESTNODE_SRC} )
TARGET_LINK_LIBRARIES( hnode-testnode hnode_node )

ADD_EXECUTABLE( hnode-hmcfg ${HNODE_HMCFG_SRC} )
TARGET_LINK_LIBRARIES( hnode-hmcfg hnode_browser )

INSTALL( TARGETS hnode_node hnode_server hnode_browser DESTINATION ${CMAKE_INSTALL_PREFIX}/lib COMPONENT lib )
INSTALL( TARGETS hmnoded DESTINATION ${CMAKE_INSTALL_PREFIX}/bin COMPONENT daemon )
INSTALL( TARGETS hnode-testnode hnode-hmcfg DESTINATION ${CMAKE_INSTALL_PREFIX}/bin COMPONENT util )

INSTALL( FILES ${HNODE_PUBLIC_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include COMPONENT devel )

# pkg-config files to go with the libraries.
#pkgconfigdir = $(libdir)/pkgconfig
#pkgconfig_DATA = hnode-node-1.0.pc hnode-server-1.0.pc hnode-browser-1.0.pc
#
#$(pkgconfig_DATA): config.status

# Set-up header files for installation

#hnodeincludedir=$(includedir)/hnode-1.0

SET( CPACK_GENERATOR "DEB" )

#SET( CPACK_PACKAGE_FILE_NAME "lib${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}" )
SET( CPACK_DEBIAN_PACKAGE_MAINTAINER "Curtis Nottberg" ) #required
 
#SET( CPACK_SET_DESTDIR "on" )
SET( CPACK_PACKAGING_INSTALL_PREFIX "/usr" )

SET( VERSION 1.1-2 )
SET( ${VERSION} CPACK_DEBIAN_PACKAGE_VERSION )
SET( CPACK_DEBIAN_PACKAGE_PRIORITY "optional" )
SET( CPACK_DEBIAN_PACKAGE_SECTION "Libraries" )
SET( CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR} )
#SET( CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (> = 2.1.3), libgcc1 (> = 1: 4.1.1)" )

SET( CPACK_PACKAGE_DESCRIPTION "Short description" )
SET( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Long description" )
SET( CPACK_PACKAGE_CONTACT "Developer " )
SET( CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}_${VERSION}_${CPACK_DEBIAN_ARCHITECTURE}" )

SET( CPACK_DEB_COMPONENT_INSTALL 1 )
SET( CPACK_COMPONENTS_ALL lib daemon util devel )

INCLUDE(CPack)

