CMAKE_MINIMUM_REQUIRED( VERSION 2.8 ) 
PROJECT( hnode_hwsrv2 ) 

SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/" )

FIND_PACKAGE(GLIB REQUIRED)
FIND_PACKAGE(GOBJECT REQUIRED)
FIND_PACKAGE(AVAHI REQUIRED)
FIND_PACKAGE(AVAHIGLIB REQUIRED)

INCLUDE_DIRECTORIES( ${GLIB_INCLUDE_DIRS} )
INCLUDE_DIRECTORIES( ${GOBJECT_INCLUDE_DIRS} )
INCLUDE_DIRECTORIES( ${AVAHI_INCLUDE_DIRS} )
INCLUDE_DIRECTORIES( ${AVAHIGLIB_INCLUDE_DIRS} )
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
#TARGET_LINK_LIBRARIES( hmnoded ${GLIB_LIBRARIES} )
#TARGET_LINK_LIBRARIES( hmnoded ${AVAHI_LIBRARIES} )
TARGET_LINK_LIBRARIES( hmnoded hnode_server )

ADD_EXECUTABLE( hnode-testnode ${HNODE_TESTNODE_SRC} )
#TARGET_LINK_LIBRARIES( hmnoded ${GLIB_LIBRARIES} )
#TARGET_LINK_LIBRARIES( hmnoded ${AVAHI_LIBRARIES} )
TARGET_LINK_LIBRARIES( hnode-testnode hnode_node )

ADD_EXECUTABLE( hnode-hmcfg ${HNODE_HMCFG_SRC} )
#TARGET_LINK_LIBRARIES( hmnoded ${GLIB_LIBRARIES} )
#TARGET_LINK_LIBRARIES( hmnoded ${AVAHI_LIBRARIES} )
TARGET_LINK_LIBRARIES( hnode-hmcfg hnode_browser )

# pkg-config files to go with the libraries.
#pkgconfigdir = $(libdir)/pkgconfig
#pkgconfig_DATA = hnode-node-1.0.pc hnode-server-1.0.pc hnode-browser-1.0.pc
#
#$(pkgconfig_DATA): config.status

# Set-up header files for installation

#hnodeincludedir=$(includedir)/hnode-1.0

