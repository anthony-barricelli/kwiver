# VXL External Project
#
# Required symbols are:
#   KWIVER_BUILD_PREFIX - where packages are built
#   KWIVER_BUILD_INSTALL_PREFIX - directory install target
#   KWIVER_PACKAGES_DIR - location of git submodule packages
#   KWIVER_ARGS_COMMON -
#
# Produced symbols are:
#   KWIVER_ARGS_sprokit -
#

if(KWIVER_ENABLE_GUI)
  set(VXL_ARGS_GUI
    -DBUILD_VGUI:BOOL=ON
    -DVGUI_USE_QT:BOOL=ON
    ${KWIVER_ARGS_QT}
    -DVGUI_USE_GLUT:BOOL=OFF
    -DVGUI_USE_GTK:BOOL=OFF
    -DVGUI_USE_GTK2:BOOL=OFF
    )
endif()

set(VXL_ARGS_CONTRIB
  -DBUILD_BRL:BOOL=OFF
  -DBUILD_MUL_TOOLS:BOOL=OFF
  -DBUILD_PRIP:BOOL=OFF
  )

set(VXL_ARGS_VIDL
  -DFFMPEG_CONFIG:FILEPATH=IGNORE
  -DFFMPEG_INCLUDE1_DIR:PATH=IGNORE
  -DFFMPEG_INCLUDE2_DIR:PATH=${KWIVER_BUILD_INSTALL_PREFIX}/include
  -DFFMPEG_avcodec_LIBRARY:PATH=${KWIVER_BUILD_INSTALL_PREFIX}/lib/${LIB_PREFIX}avcodec${LIB_SUFFIX}
  -DFFMPEG_avformat_LIBRARY:PATH=${KWIVER_BUILD_INSTALL_PREFIX}/lib/${LIB_PREFIX}avformat${LIB_SUFFIX}
  -DFFMPEG_avutil_LIBRARY:PATH=${KWIVER_BUILD_INSTALL_PREFIX}/lib/${LIB_PREFIX}avutil${LIB_SUFFIX}
  -DFFMPEG_swscale_LIBRARY:PATH=${KWIVER_BUILD_INSTALL_PREFIX}/lib/${LIB_PREFIX}swscale${LIB_SUFFIX}
  )

if(UNIX)
  set(VXL_ARGS_V3P
    -DVXL_USING_NATIVE_ZLIB:BOOL=TRUE
    -DVXL_USING_NATIVE_BZLIB2:BOOL=TRUE
    )
  set(VXL_EXTRA_CMAKE_CXX_FLAGS
    -DVXL_EXTRA_CMAKE_CXX_FLAGS:STRING=-D__STDC_CONSTANT_MACROS
    )
elseif(WIN32)
  set(VXL_ARGS_V3P
    -DVXL_FORCE_V3P_JPEG:BOOL=ON
    -DVXL_FORCE_V3P_TIFF:BOOL=ON
    )
endif()

ExternalProject_Add(VXL
  PREFIX ${KWIVER_BUILD_PREFIX}
  SOURCE_DIR ${KWIVER_PACKAGES_DIR}/VXL
  CMAKE_GENERATOR ${gen}
  CMAKE_ARGS
    ${KWIVER_ARGS_COMMON}
    ${VXL_ARGS_GUI}
    ${VXL_ARGS_CONTRIB}
    # ${VXL_ARGS_VIDL}
    ${VXL_ARGS_V3P}
    ${VXL_EXTRA_CMAKE_CXX_FLAGS}
    -DBUILD_CORE_PROBABILITY:BOOL=OFF
    -DBUILD_EXAMPLES:BOOL=OFF
    -DBUILD_TESTING:BOOL=OFF
    -DBUILD_DOCUMENTATION:BOOL=OFF
    -DBUILD_CORE_GEOMETRY:BOOL=OFF
    -DBUILD_CORE_IMAGING:BOOL=ON
    -DBUILD_CORE_SERIALISATION:BOOL=ON
    -DBUILD_GEL:BOOL=OFF
    -DBUILD_MUL:BOOL=OFF
    -DBUILD_MUL_TOOLS:BOOL=OFF
    -DBUILD_TBL:BOOL=OFF
    # Forcing off the use of OpenGL
    -DOPENGL_xmesa_INCLUDE_DIR:PATH=
  INSTALL_DIR ${KWIVER_BUILD_INSTALL_PREFIX}
  )

ExternalProject_Add_Step(VXL forcebuild
  COMMAND ${CMAKE_COMMAND}
    -E remove ${KWIVER_BUILD_PREFIX}/src/VXL-stamp/VXL-build
  COMMENT "Removing build stamp file for build update (forcebuild)."
  DEPENDEES configure
  DEPENDERS build
  ALWAYS 1
  )

include_directories( ${KWIVER_BUILD_INSTALL_PREFIX}/include/vxl
                     ${KWIVER_BUILD_INSTALL_PREFIX}/include/vxl/vcl
                     ${KWIVER_BUILD_INSTALL_PREFIX}/include/vxl/core )

set(KWIVER_ARGS_VXL
  #-DVXL_DIR=${KWIVER_BUILD_PREFIX}/src/VXL-build
  -DVXL_DIR:PATH=${KWIVER_BUILD_INSTALL_PREFIX}/share/vxl/cmake
  )
