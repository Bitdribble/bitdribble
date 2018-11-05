if (UNIX)
   # Are we running systemd?
   execute_process(COMMAND pidof systemd >/dev/null && echo yes OUTPUT_VARIABLE IS_SYSTEMD)
   execute_process(COMMAND pidof /sbin/init >/dev/null && echo yes OUTPUT_VARIABLE IS_SYSVINIT)
endif()

#message("IS_SYSTEMD = ${IS_SYSTEMD}")
#message("IS_SYSVINIT = ${IS_SYSVINIT}")

if (IS_SYSTEMD)
 install(FILES systemd/bitd.service DESTINATION /lib/systemd/system)
endif()

if (UNIX)
 install(FILES etc/bitd.yml DESTINATION /etc)
endif()

#
# CPACK setup
#
set(CPACK_PACKAGE_NAME "bitd")
set(CPACK_PACKAGE_VENDOR "bitdribble.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Bitdribble Installer")
set(CPACK_PACKAGE_VERSION ${BITD_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${BITD_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${BITD_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${BITD_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "bitdribble")

# Define components and their display names
set(CPACK_COMPONENTS_ALL applications libraries headers)
set(CPACK_COMPONENT_APPLICATIONS_DISPLAY_NAME "Bitdribble Applications")
set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Bitdribble Libraries")
set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "Bitdribble C/C++ Headers")

# Human readable component descriptions
set(CPACK_COMPONENT_APPLICATIONS_DESCRIPTION
  "A microcontainer agent for distributed data monitoring")
set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION
  "Libraries for Bitdribble development tools")
set(CPACK_COMPONENT_HEADERS_DESCRIPTION
  "C/C++ headers for Bitdribble development tools")

# Define dependencies between components
set (CPACK_COMPONENT_HEADERS_DEPENDS libraries)

# Define groups
set(CPACK_COMPONENT_APPLICATIONS_GROUP "Runtime")
set(CPACK_COMPONENT_LIBRARIES_GROUP "Development")
set(CPACK_COMPONENT_HEADERS_GROUP "Development")

set(CPACK_COMPONENT_GROUP_DEVELOPMENT_DESCRIPTION
   "Tools needed to develop applications with the bitdribble library")

# Define NSIS installation types
set(CPACK_ALL_INSTALL_TYPES Full Developer)
set(CPACK_COMPONENT_LIBRARIES_INSTALL_TYPES Developer Full)
set(CPACK_COMPONENT_HEADERS_INSTALL_TYPES Developer Full)
set(CPACK_COMPONENT_APPLICATIONS_INSTALL_TYPES Full)

# Debian package specific definitions
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Andrei Radulescu-Banu") #required
set(CPACK_DEBIAN_PACKAGE_DEPENDS "expat, libyaml-0-2, openssl")
if ("${LSB_RELEASE_CODENAME}" STREQUAL "jessie")
  # Raspbian 8.0 jessie
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libcurl3")
elseif ("${LSB_RELEASE_CODENAME}" STREQUAL "stretch")
  # Ubuntu 18.04 bionic
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libcurl3")
elseif ("${LSB_RELEASE_CODENAME}" STREQUAL "bionic")
  # Ubuntu 18.04 bionic
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libcurl4")
else()
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libcurl")
endif()
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
  "${CMAKE_SOURCE_DIR}/install/deb/prerm;${CMAKE_SOURCE_DIR}/install/deb/postinst")

# RPM package specific definitions
set(CPACK_RPM_USER_BINARY_SPECFILE "${CMAKE_SOURCE_DIR}/install/rpm/bitd.spec.in")
set(CPACK_RPM_PACKAGE_REQUIRES "expat, libyaml, openssl, libcurl")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "/etc;/lib;/lib/systemd;/lib/systemd/system")

# Must be after the last CPACK macros
include(CPack)
