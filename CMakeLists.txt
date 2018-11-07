cmake_minimum_required(VERSION 3.1)
cmake_policy(SET CMP0054 NEW)

project(bitdribble)

set(PACKAGE_BUGREPORT "bugs@bitdribble.com")
set(PACKAGE_NAME "bitdribble")

# The version number.
set(BITD_VERSION_MAJOR 1)
set(BITD_VERSION_MINOR 0)
set(BITD_VERSION_PATCH 0)
set(BITD_VERSION "${BITD_VERSION_MAJOR}.${BITD_VERSION_MINOR}.${BITD_VERSION_PATCH}")

# Install in standard GNU directories
include(GNUInstallDirs)

# Set build output subdirs
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set path to local modules. Local modules are necessary when
# cmake-installed modules don't include all modules we need. In our case,
# the libyaml module is not distributed (yet) with cmake.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

#
# Detect build machine OS distribution
#
if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
  find_program(LSB_RELEASE_EXEC lsb_release)
  execute_process(COMMAND ${LSB_RELEASE_EXEC} -is
    OUTPUT_VARIABLE LSB_RELEASE_ID
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${LSB_RELEASE_EXEC} -rs
    OUTPUT_VARIABLE LSB_RELEASE_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${LSB_RELEASE_EXEC} -cs
    OUTPUT_VARIABLE LSB_RELEASE_CODENAME
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  if (NOT LSB_RELEASE_ID)
    # Check if it's Alpile Linux
    file(READ /etc/alpine-release LSB_RELEASE_VERSION)
    if (LSB_RELEASE_VERSION)
      set(LSB_RELEASE_ID Alpine)
      set(LSB_RELEASE_CODENAME alpine)
    endif()
  endif()
  if (NOT LSB_RELEASE_ID)
    # Check if it's OpenWRT Linux
    file(READ /etc/openwrt_release LSB_RELEASE_VERSION)
    if (LSB_RELEASE_VERSION)
      set(LSB_RELEASE_ID OpenWRT)
      set(LSB_RELEASE_CODENAME openwrt)
    endif()
  endif()
endif()

#
# Supported platforms and how to detect them:
# - Linux:                 if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
#   - Centos:              if ("${LSB_RELEASE_ID}" STREQUAL "CentOS")
#   - Ubuntu:              if ("${LSB_RELEASE_ID}" STREQUAL "Ubuntu")
#   - Alpine:              if ("${LSB_RELEASE_ID}" STREQUAL "Alpine")
#   - Raspberry Pi:        if ("${LSB_RELEASE_ID}" STREQUAL "Raspbian")
#   - OpenWRT:             if ("${LSB_RELEASE_ID}" STREQUAL "Raspbian")
# - Mac OSX:               if (APPLE)
# - Windows                if (WIN32)
#   - Mingw
#   - Cygwin               if (CMAKE_SYSTEM_NAME MATCHES "CYGWIN")
#

if (APPLE)
 # On OSX, set this variable to look up openssl in the right place
 if (NOT "${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")
   error("${CMAKE_SYSTEM_NAME} not supported")
 endif()
endif()

# We depend on libexpat
if (NOT WIN32 AND NOT OPENWRT)
  include(FindEXPAT)
  find_package(EXPAT REQUIRED)
  include_directories(${EXPAT_INCLUDE_DIRS})
else()
  set(EXPAT_LIBRARIES expat)
endif()

if (NOT WIN32 AND NOT OPENWRT)
  # We depend on libyaml - and provide our own module definition for it
  include(FindLIBYAML)
  find_package(LIBYAML REQUIRED)
  include_directories(${LIBYAML_INCLUDE_DIR})
else()
  set(LIBYAML_LIBRARIES yaml)  
endif()

if (NOT APPLE AND NOT WIN32 AND NOT OPENWRT)
  # Some modules depend on openssl
  include(FindOpenSSL)
  find_package(OpenSSL REQUIRED)
  include_directories(${OPENSSL_INCLUDE_DIR})

  # Some modules depend on libcurl
  include(FindCURL)
  find_package(CURL REQUIRED)
  include_directories(${CURL_INCLUDE_DIRS})
endif()

if (APPLE)
   # Use the Brew-installed openssl and curl instead of the sytem libraries
   # by the same name
   include_directories(/usr/local/opt/openssl/include)
   include_directories(/usr/local/opt/curl/include)
   set(OPENSSL_LIBRARIES /usr/local/opt/openssl/lib/libssl.dylib /usr/local/opt/openssl/lib/libcrypto.dylib)
   set(CURL_LIBRARIES /usr/local/opt/curl/lib/libcurl.dylib)
endif()

if (WIN32 OR OPENWRT)
   # Use the MinGW-w64 openssl and curl
   set(OPENSSL_LIBRARIES ssl crypto)
   set(CURL_LIBRARIES curl)
endif()

#
# Set up the system libraries and do system checks
#
include("${CMAKE_SOURCE_DIR}/cmake/Includes/SystemChecks.cmake")

# Config header file
configure_file (
  config.h.in
  "${PROJECT_BINARY_DIR}/include/bitd/config.h"
  )

# Determine if we're installing under lib or lib64
get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
if ("${LIB64}" STREQUAL "TRUE")
  set(BITD_LIBSUFFIX 64)
else()
  set(BITD_LIBSUFFIX "")
endif()

# Use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH  FALSE)

# When building, don't use the install RPATH already
# (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib${BITD_LIBSUFFIX}")

# Add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# The RPATH to be used when installing, but only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib${BITD_LIBSUFFIX}" isSystemDir)
if ("${isSystemDir}" STREQUAL "-1")
   set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib${BITD_LIBSUFFIX}")
endif()

# Enable @rpath for OSX
set(CMAKE_MACOSX_RPATH 1)

#
# Platform specific compiler options
#
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall)
endif()

if (MINGW)
   add_compile_options(-Wno-format)
endif()

# Enable unit tests
enable_testing()

# The bitdribble library is built under this folder
add_subdirectory(src)

# Headers install
install(DIRECTORY include
        DESTINATION .
	COMPONENT Development)

install(DIRECTORY ${CMAKE_BINARY_DIR}/include
        DESTINATION .
	COMPONENT Development)

# The installer subdir
add_subdirectory(install)

