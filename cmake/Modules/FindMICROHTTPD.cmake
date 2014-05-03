include(FindPkgConfig)

pkg_check_modules(MICROHTTPD_PKG libmicrohttpd)

if (MICROHTTPD_PKG_FOUND)
    find_path(MICROHTTPD_INCLUDE_DIR  NAMES microhttpd.h
       PATHS
       ${MICROHTTPD_PKG_INCLUDE_DIRS}
       /usr/include
       /usr/local/include
    )

    find_library(MICROHTTPD_LIBRARIES NAMES microhttpd
       PATHS
       ${MICROHTTPD_PKG_LIBRARY_DIRS}
       /usr/lib
       /usr/local/lib
    )

else (MICROHTTPD_PKG_FOUND)
    # Find Glib even if pkg-config is not working (eg. cross compiling to Windows)
    find_library(MICROHTTPD_LIBRARIES NAMES microhttpd)
    string (REGEX REPLACE "/[^/]*$" "" MICROHTTPD_LIBRARIES_DIR ${MICROHTTPD_LIBRARIES})

    find_path(MICROHTTPD_INCLUDE_DIR NAMES microhttpd.h)

endif (MICROHTTPD_PKG_FOUND)

if (MICROHTTPD_INCLUDE_DIR AND MICROHTTPD_LIBRARIES)
    set(MICROHTTPD_INCLUDE_DIRS ${MICROHTTPD_INCLUDE_DIR})
endif (MICROHTTPD_INCLUDE_DIR AND MICROHTTPD_LIBRARIES)

if(MICROHTTPD_INCLUDE_DIRS AND MICROHTTPD_LIBRARIES)
   set(MICROHTTPD_FOUND TRUE CACHE INTERNAL "libmicrohttpd found")
   message(STATUS "Found libmicrohttpd: ${MICROHTTPD_INCLUDE_DIR}, ${MICROHTTPD_LIBRARIES}")
else(MICROHTTPD_INCLUDE_DIRS AND MICROHTTPD_LIBRARIES)
   set(MICROHTTPD_FOUND FALSE CACHE INTERNAL "libmicrohttpd not found")
   message(STATUS "libmicrohttpd not found.")
endif(MICROHTTPD_INCLUDE_DIRS AND MICROHTTPD_LIBRARIES)

mark_as_advanced(MICROHTTPD_INCLUDE_DIR MICROHTTPD_INCLUDE_DIRS MICROHTTPD_LIBRARIES)


