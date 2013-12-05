CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(appcore C)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(VERSION_MAJOR 1)
SET(VERSION "${VERSION_MAJOR}.1")

IF ( ENABLE_WAYLAND )
	SET(BUILD_FOR_WAYLAND 1)
ELSE ()
        SET(BUILD_FOR_WAYLAND 0)
ENDIF ()

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)

ADD_DEFINITIONS("-DPREFIX=\"${PREFIX}\"")

SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -fvisibility=hidden")
SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -g -Wall")
SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -Wl,--as-needed")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

SET(CMAKE_SKIP_BUILD_RPATH TRUE)

#################################################################
# Build appcore-common Library
# ------------------------------
SET(APPCORE_COMMON "appcore-common")

IF ( ENABLE_WAYLAND )
       SET(SRCS_common src/appcore.c src/appcore-i18n.c src/appcore-measure.c
       #               src/appcore-noti.c src/appcore-pmcontrol.c
                       src/appcore-rotation.c
       #               src/appcore-util.c
                       src/appcore-Wayland.c)
ELSE ()
       SET(SRCS_common src/appcore.c src/appcore-i18n.c src/appcore-measure.c
       #               src/appcore-noti.c src/appcore-pmcontrol.c
                       src/appcore-rotation.c
       #               src/appcore-util.c
                       src/appcore-X.c)
ENDIF ()

SET(HEADERS_common appcore-common.h)

INCLUDE(FindPkgConfig)
IF ( ENABLE_WAYLAND )
       pkg_check_modules(pkg_common REQUIRED vconf sensor aul dlog ecore)
ELSE ()
       pkg_check_modules(pkg_common REQUIRED vconf sensor aul dlog x11 ecore-x)
ENDIF ()

FOREACH(flag ${pkg_common_CFLAGS})
	SET(EXTRA_CFLAGS_common "${EXTRA_CFLAGS_common} ${flag}")
ENDFOREACH(flag)

SET(APPCORE_CONFIG "appcore-config")
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/${APPCORE_CONFIG}.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/${APPCORE_CONFIG}.h @ONLY)

ADD_LIBRARY(${APPCORE_COMMON} SHARED ${SRCS_common})
SET_TARGET_PROPERTIES(${APPCORE_COMMON} PROPERTIES SOVERSION ${VERSION_MAJOR})
SET_TARGET_PROPERTIES(${APPCORE_COMMON} PROPERTIES VERSION ${VERSION})
SET_TARGET_PROPERTIES(${APPCORE_COMMON} PROPERTIES COMPILE_FLAGS ${EXTRA_CFLAGS_common})
TARGET_LINK_LIBRARIES(${APPCORE_COMMON} ${pkg_common_LDFLAGS} "-ldl")

CONFIGURE_FILE(${APPCORE_COMMON}.pc.in ${APPCORE_COMMON}.pc @ONLY)

INSTALL(TARGETS ${APPCORE_COMMON} DESTINATION ${LIB_INSTALL_DIR} COMPONENT RuntimeLibraries)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${APPCORE_COMMON}.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
FOREACH(hfile ${HEADERS_common})
	INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/${hfile} DESTINATION include/appcore)
ENDFOREACH(hfile)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/SLP_Appcore_PG.h DESTINATION include)

#################################################################
# Build appcore-efl Library
# ------------------------------
SET(APPCORE_EFL "appcore-efl")
SET(SRCS_efl src/appcore-efl.c)
SET(HEADERS_efl appcore-efl.h)

INCLUDE(FindPkgConfig)

IF ( ENABLE_WAYLAND )
        pkg_check_modules(pkg_efl REQUIRED elementary dlog ecore gobject-2.0 glib-2.0 aul)
ELSE ()
        pkg_check_modules(pkg_efl REQUIRED elementary dlog ecore ecore-x gobject-2.0 glib-2.0 aul)
ENDIF ()

FOREACH(flag ${pkg_efl_CFLAGS})
	SET(EXTRA_CFLAGS_efl "${EXTRA_CFLAGS_efl} ${flag}")
ENDFOREACH(flag)

ADD_LIBRARY(${APPCORE_EFL} SHARED ${SRCS_efl})
SET_TARGET_PROPERTIES(${APPCORE_EFL} PROPERTIES SOVERSION ${VERSION_MAJOR})
SET_TARGET_PROPERTIES(${APPCORE_EFL} PROPERTIES VERSION ${VERSION})
SET_TARGET_PROPERTIES(${APPCORE_EFL} PROPERTIES COMPILE_FLAGS ${EXTRA_CFLAGS_efl})
TARGET_LINK_LIBRARIES(${APPCORE_EFL} ${pkg_efl_LDFLAGS} ${APPCORE_COMMON})

CONFIGURE_FILE(${APPCORE_EFL}.pc.in ${APPCORE_EFL}.pc @ONLY)

INSTALL(TARGETS ${APPCORE_EFL} DESTINATION ${LIB_INSTALL_DIR} COMPONENT RuntimeLibraries)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${APPCORE_EFL}.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
FOREACH(hfile ${HEADERS_efl})
	INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/${hfile} DESTINATION include/appcore)
ENDFOREACH(hfile)

