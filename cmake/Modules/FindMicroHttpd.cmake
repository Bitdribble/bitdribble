#.rst:
# FindMicroHttpd
# --------------
# Finds the MicroHttpd library
#
# This will will define the following variables::
#
# MICROHTTPD_FOUND - system has MicroHttpd
# MICROHTTPD_INCLUDE_DIRS - the MicroHttpd include directory
# MICROHTTPD_LIBRARIES - the MicroHttpd libraries
# MICROHTTPD_DEFINITIONS - the MicroHttpd definitions
#
# and the following imported targets::
#
#   MicroHttpd::MicroHttpd   - The MicroHttpd library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_MICROHTTPD libmicrohttpd>=0.4 QUIET)
endif()

find_path(MICROHTTPD_INCLUDE_DIR NAMES microhttpd.h
                                 PATHS ${PC_MICROHTTPD_INCLUDEDIR})
find_library(MICROHTTPD_LIBRARY NAMES microhttpd libmicrohttpd
                                PATHS ${PC_MICROHTTPD_LIBDIR})

set(MICROHTTPD_VERSION ${PC_MICROHTTPD_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MicroHttpd
                                  REQUIRED_VARS MICROHTTPD_LIBRARY MICROHTTPD_INCLUDE_DIR
                                  VERSION_VAR MICROHTTPD_VERSION)

if(MICROHTTPD_FOUND)
  set(MICROHTTPD_LIBRARIES ${MICROHTTPD_LIBRARY})
  set(MICROHTTPD_INCLUDE_DIRS ${MICROHTTPD_INCLUDE_DIR})
  set(MICROHTTPD_DEFINITIONS -DHAVE_LIBMICROHTTPD=1)
endif()

mark_as_advanced(MICROHTTPD_LIBRARY MICROHTTPD_INCLUDE_DIR)
