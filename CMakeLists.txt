###############################################################################
#  Copyright (c) 2016-2023 Silov Andrey
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.24)
project(Skia VERSION 0.1.0 LANGUAGES CXX C)

set(SKIA_Ver m141)

find_package(OpenGL REQUIRED)

############################################################################
# Prebuilt binaries
############################################################################
if (APPLE)
  message(FATAL_ERROR "Warning: Not supported")

elseif (MSVC)

  if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    set(SKIA_BINARIES ${CMAKE_CURRENT_BINARY_DIR}/windows/msvc/x86_64/src/prebuilt_binaries)

    if (CMAKE_BUILD_TYPE STREQUAL "Release")
      set(SKIA_BINARIES_URL skia_msvc.zip)
      set(SKIA_BINARIES_HASH db2820a86f18d6eafbe61aa622e477c2fd2f91266ded9903aed26b8be7ed84f6)
    else()
      set(SKIA_BINARIES_URL skia_msvc_dbg.zip)
      set(SKIA_BINARIES_HASH a8971d891f8e5fc4daa5432d3c9dc3b66730a168e4ece94c28a16bd0b8c101f2)
    endif()

  elseif (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
      message(WARNING "Warning: There are no precompiled binaries built using MTd for Clang. Use MT.")
    endif()

    message(WARNING "Warning: Not supported")
    # set(SKIA_BINARIES ${CMAKE_CURRENT_BINARY_DIR}/windows/clang/x86_64/src/prebuilt_binaries)
    # set(SKIA_BINARIES_URL https://github.com/cycfi/artist/raw/prebuilt_binaries_0.90/windows/skia/clang/x86_64/x86_64.zip)
    # set(SKIA_BINARIES_MD5 b2bc06c19cd61f93505b7e37eaf489db)
    # set(SKIA_BINARIES_PREFIX windows/clang/x86_64)
  endif()

  set(SKIA_BINARIES_PREFIX windows/msvc/x86_64)

  set(SKIA_LIBRARIES
      ${SKIA_BINARIES}/skia.lib
      ${SKIA_BINARIES}/harfbuzz.lib
      ${SKIA_BINARIES}/fontconfig.lib
      ${SKIA_BINARIES}/freetype.lib
      ${SKIA_BINARIES}/bz2.lib
      ${SKIA_BINARIES}/brotlicommon.lib
      ${SKIA_BINARIES}/brotlienc.lib
      ${SKIA_BINARIES}/brotlidec.lib
      ${SKIA_BINARIES}/libjpeg.lib
      ${SKIA_BINARIES}/libpng.lib
      ${SKIA_BINARIES}/libwebp_sse41.lib
      ${SKIA_BINARIES}/libwebp.lib
      ${SKIA_BINARIES}/zlib.lib
      ${SKIA_BINARIES}/skcms.lib
      ${SKIA_BINARIES}/expat.lib
  )

  #set(SKIA_DEPENDENCIES ${SKIA_LIBRARIES})

  set(SKIA_TOOLS_SOURCE
        ${CMAKE_CURRENT_LIST_DIR}/tools/window/win/GLWindowContext_win.cpp
        ${CMAKE_CURRENT_LIST_DIR}/tools/window/win/RasterWindowContext_win.cpp
        ${CMAKE_CURRENT_LIST_DIR}/tools/gpu/gl/win/SkWGL_win.cpp
      ) 

  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
  set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ob2 /Oi /Ot /GL /MT")

elseif (UNIX)

  if (SKIA_X11)
    set(PLATFORM x11)
    set(DEPEND OpenGL::GLX)

    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
      set(CPL gcc)
      set(SKIA_BINARIES_HASH c8b3c6def4e870ac17d8805a9056999c311eb5f72b6df399f551628b56fa1af6)
    elseif (CMAKE_C_COMPILER_ID STREQUAL "Clang")
      set(CPL clang)
      set(SKIA_BINARIES_HASH 785de1222ebe63b94d397e81693ea3b65cd09919e4546d142e29be79713714ce)
    endif()

  else()
    set(PLATFORM egl)
    set(DEPEND OpenGL::EGL)
    set(SKIA_TOOLS_SOURCE 
        ${CMAKE_CURRENT_LIST_DIR}/tools/window/GLWindowContext.cpp
        ${CMAKE_CURRENT_LIST_DIR}/tools/window/WindowContext.cpp
      )

    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
      set(CPL gcc)
      set(SKIA_BINARIES_HASH acc9525fa0d1f7561da9298132921a62f9b37dfb65267a2e15ecc84e08e5f424)
    elseif (CMAKE_C_COMPILER_ID STREQUAL "Clang")
      set(CPL clang)
      set(SKIA_BINARIES_HASH e18f6841e5acc246f4248da442a487b96c5ab8a768924eee4cad995b062e05f8)
    endif()

  endif()

  find_package(PkgConfig REQUIRED)
  find_package(harfbuzz REQUIRED)

  pkg_check_modules(skia_depends REQUIRED IMPORTED_TARGET
    fontconfig freetype2 zlib libpng libjpeg libwebp libwebpdemux
  )

  set(SKIA_BINARIES_PREFIX linux/${CPL}/x86_64)
  set(SKIA_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/${SKIA_BINARIES_PREFIX}/src/prebuilt_binaries/libskia.a)
  set(SKIA_BINARIES_URL skia_${PLATFORM}_${CPL}.zip)

  set(SKIA_DEPENDENCIES
      ${DEPEND}
      PkgConfig::skia_depends
      harfbuzz::harfbuzz
  )

endif()

include(ExternalProject)

ExternalProject_Add(prebuilt_binaries
      PREFIX "${SKIA_BINARIES_PREFIX}"
      URL https://github.com/silovaa/skia_bin/releases/download/${SKIA_Ver}/${SKIA_BINARIES_URL}
      URL_HASH SHA256=${SKIA_BINARIES_HASH}
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
      BUILD_BYPRODUCTS ${SKIA_LIBRARIES}
  )

add_library(Skia::skia STATIC IMPORTED GLOBAL)
add_dependencies(Skia::skia prebuilt_binaries)

set_target_properties(Skia::skia PROPERTIES
  IMPORTED_LOCATION ${SKIA_LIBRARIES}
)
target_link_libraries(Skia::skia INTERFACE OpenGL::OpenGL ${SKIA_DEPENDENCIES})
target_include_directories(Skia::skia INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/include/
  ${CMAKE_CURRENT_LIST_DIR}/include/core
)

if (WIN_TOOLS_REQURE)
  add_library(win_tools STATIC ${SKIA_TOOLS_SOURCE})
  add_library(Skia::win_tools ALIAS win_tools)
  target_link_libraries(win_tools PRIVATE Skia::skia)
  target_include_directories(win_tools PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src 
                                              ${CMAKE_CURRENT_LIST_DIR}/tools
  )
  target_compile_definitions(
      win_tools
      PUBLIC
        SK_RELEASE
         #SK_DEBUG
        SK_GL
   )
endif()

