# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)

set(CMAKE_SYSROOT /usr/x86_64-w64-mingw32/sys-root)

# Optionally use a different installation folder than CMAKE_SYSROOT
#set(CMAKE_STAGING_PREFIX /home/devel/usr/x86_64-w64-mingw32/sys-root)

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
SET(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
