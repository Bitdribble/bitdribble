#
# System checks
#
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckStructHasMember)
include(CheckSymbolExists)
include(CheckLibraryExists)

# Find the thread library of the system. 
find_package(Threads REQUIRED)

#
# Check for common headers
#
check_include_files(stdio.h BITD_HAVE_STDIO_H)
if (NOT BITD_HAVE_STDIO_H)
  message(FATAL_ERROR "stdio.h not found.")
endif()

check_include_files(stddef.h BITD_HAVE_STDDEF_H)
if (NOT BITD_HAVE_STDDEF_H)
  message(FATAL_ERROR "stddef.h not found.")
endif()

check_include_files(stdlib.h BITD_HAVE_STDLIB_H)
if (NOT BITD_HAVE_STDLIB_H)
  message(FATAL_ERROR "stdlib.h not found.")
endif()

check_include_files(stdarg.h BITD_HAVE_STDARG_H)
if (NOT BITD_HAVE_STDARG_H)
  message(FATAL_ERROR "stdarg.h not found.")
endif()

check_include_files(unistd.h BITD_HAVE_UNISTD_H)
if (NOT BITD_HAVE_UNISTD_H)
  message(FATAL_ERROR "unistd.h not found.")
endif()

check_include_files(string.h BITD_HAVE_STRING_H)
if (NOT BITD_HAVE_STRING_H)
  message(FATAL_ERROR "string.h not found.")
endif()

check_include_files(ctype.h BITD_HAVE_CTYPE_H)
if (NOT BITD_HAVE_CTYPE_H)
  message(FATAL_ERROR "ctype.h not found.")
endif()

check_include_files(sys/types.h BITD_HAVE_SYS_TYPES_H)
if (NOT BITD_HAVE_SYS_TYPES_H)
  message(FATAL_ERROR "sys/types.h not found.")
endif()

check_include_files(sys/socket.h BITD_HAVE_SYS_SOCKET_H)
if (NOT WIN32 AND NOT BITD_HAVE_SYS_SOCKET_H)
  message(FATAL_ERROR "sys/socket.h not found.")
endif()

check_include_files(sys/stat.h BITD_HAVE_SYS_STAT_H)
if (NOT BITD_HAVE_SYS_STAT_H)
  message(FATAL_ERROR "sys/stat.h not found.")
endif()

check_include_files(sys/wait.h BITD_HAVE_SYS_WAIT_H)
if (NOT WIN32 AND NOT BITD_HAVE_SYS_WAIT_H)
  message(FATAL_ERROR "sys/wait.h not found.")
endif()

check_include_files(sys/time.h BITD_HAVE_SYS_TIME_H)
if (NOT BITD_HAVE_SYS_TIME_H)
  message(FATAL_ERROR "sys/time.h not found.")
endif()

check_include_files(time.h BITD_HAVE_TIME_H)

check_include_files(netinet/in.h BITD_HAVE_NETINET_IN_H)
if (NOT WIN32 AND NOT BITD_HAVE_NETINET_IN_H)
  message(FATAL_ERROR "netinet/in.h not found.")
endif()

check_include_files(arpa/inet.h BITD_HAVE_ARPA_INET_H)
if (NOT WIN32 AND NOT BITD_HAVE_ARPA_INET_H)
  message(FATAL_ERROR "arpa/inet.h not found.")
endif()

check_include_files(fcntl.h BITD_HAVE_FCNTL_H)
if (NOT BITD_HAVE_FCNTL_H)
  message(FATAL_ERROR "fcntl.h not found.")
endif()

check_include_files(signal.h BITD_HAVE_SIGNAL_H)
if (NOT BITD_HAVE_SIGNAL_H)
  message(FATAL_ERROR "signal.h not found.")
endif()

check_include_files(inttypes.h BITD_HAVE_INTTYPES_H)
if (NOT BITD_HAVE_INTTYPES_H)
  message(FATAL_ERROR "inttypes.h not found.")
endif()

check_include_files(limits.h BITD_HAVE_LIMITS_H)
if (NOT BITD_HAVE_LIMITS_H)
  message(FATAL_ERROR "limits.h not found.")
endif()

check_include_files(netdb.h BITD_HAVE_NETDB_H)

check_include_files(netdb.h BITD_HAVE_POLL_H)

check_include_files(assert.h BITD_HAVE_ASSERT_H)
if (NOT BITD_HAVE_ASSERT_H)
  message(FATAL_ERROR "assert.h not found.")
endif()

check_include_files(errno.h BITD_HAVE_ERRNO_H)
if (NOT BITD_HAVE_ERRNO_H)
  message(FATAL_ERROR "errno.h not found.")
endif()

if (NOT WIN32)
  # Use dl linker on non-WIN32 platforms
  check_include_files(dlfcn.h BITD_HAVE_DLFCN_H)
  if (NOT BITD_HAVE_DLFCN_H)
    message(FATAL_ERROR "dlfcn.h not found.")
  endif()
endif()

if (NOT WIN32)
  check_include_files(spawn.h BITD_HAVE_SPAWN_H)
  if (NOT BITD_HAVE_SPAWN_H)
     message(FATAL_ERROR "spawn.h not found.")
  endif()
endif()

#
# Check for Win32 headers
#
if (WIN32)
  check_include_files(winsock2.h BITD_HAVE_WINSOCK2_H)
  if (NOT BITD_HAVE_WINSOCK2_H)
    message(FATAL_ERROR "winsock2.h not found.")
  endif()

  check_include_files(ws2tcpip.h BITD_HAVE_WS2TCPIP_H)
  if (NOT BITD_HAVE_WS2TCPIP_H)
    message(FATAL_ERROR "ws2tcpip.h not found.")
  endif()

  check_include_files(windows.h BITD_HAVE_WINDOWS_H)
  if (NOT BITD_HAVE_WINDOWS_H)
    message(FATAL_ERROR "windows.h not found.")
  endif()

  check_include_files(process.h BITD_HAVE_PROCESS_H)
  if (NOT BITD_HAVE_PROCESS_H)
    message(FATAL_ERROR "process.h not found.")
  endif()
endif()

if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
 check_struct_has_member("struct scm_timestamping" ts
   "time.h;linux/errqueue.h" BITD_HAVE_STRUCT_SCM_TIMESTAMPING)
endif()

check_struct_has_member("struct sockaddr_in" sin_len
  "netinet/in.h" BITD_HAVE_STRUCT_SOCKADDR_IN_SIN_LEN)

#
# Support library headers
#
check_include_files(yaml.h BITD_HAVE_YAML_H)
if (NOT BITD_HAVE_YAML_H)
  message(FATAL_ERROR "yaml.h not found. Ensure libyaml development package is installed.")
endif()

check_include_files(expat.h BITD_HAVE_EXPAT_H)
if (NOT BITD_HAVE_EXPAT_H)
  message(FATAL_ERROR "expat.h not found. Ensure libexpat development package is installed.")
endif()

check_include_files(jansson.h BITD_HAVE_JANSSON_H)
if (NOT BITD_HAVE_JANSSON_H)
  message(FATAL_ERROR "jansson.h not found. Ensure libjansson development package is installed.")
endif()

check_include_files(curl/curl.h BITD_HAVE_CURL_H)
if (NOT BITD_HAVE_CURL_H)
  message(FATAL_ERROR "curl/curl.h not found. Ensure libcurl development package is installed.")
endif()

check_include_files(microhttpd.h BITD_HAVE_MICROHTTPD_H)
if (NOT BITD_HAVE_MICROHTTPD_H)
  message(FATAL_ERROR "microhttpd.h not found. Ensure libmicrohttpd development package is installed.")
endif()

#
# System library setup
#
if (WIN32)
  set(BITD_LIBRARIES ${BITD_LIBRARIES} ws2_32 user32 advapi32)
endif()
if (UNIX)
  set(BITD_LIBRARIES ${BITD_LIBRARIES} stdc++ c ${CMAKE_DL_LIBS})
endif()
if (UNIX AND NOT APPLE)
  set(BITD_LIBRARIES ${BITD_LIBRARIES} gcc rt)
endif()
if (CMAKE_USE_PTHREADS_INIT AND NOT CMAKE_USE_WIN32_THREADS_INIT)
  set(BITD_LIBRARIES ${BITD_LIBRARIES} pthread)
endif()

# Append the expat and libyaml libraries
set(BITD_LIBRARIES ${BITD_LIBRARIES} ${EXPAT_LIBRARIES} ${LIBYAML_LIBRARIES} ${JANSSON_LIBRARIES})
set(CMAKE_REQUIRED_LIBRARIES ${BITD_LIBRARIES})

#message("BITD_LIBRARIES = ${BITD_LIBRARIES}")

#
# Function checks
#

# Use these system libraries when checking for functions
if (NOT WIN32)
    check_function_exists(inet_pton BITD_HAVE_INET_PTON)
    check_function_exists(inet_pton BITD_HAVE_INET_NTOP)
endif()

check_function_exists(gethostbyname BITD_HAVE_GETHOSTBYNAME)
check_function_exists(gethostbyname_r BITD_HAVE_GETHOSTBYNAME_R)
if (NOT BITD_HAVE_GETHOSTBYNAME AND NOT BITD_HAVE_GETHOSTBYNAME_R)
    message(FATAL_ERROR "gethostbyname() not found.")
endif()

check_function_exists(gethostbyaddr BITD_HAVE_GETHOSTBYADDR)
check_function_exists(gethostbyaddr_r BITD_HAVE_GETHOSTBYADDR_R)
if (NOT BITD_HAVE_GETHOSTBYADDR AND NOT BITD_HAVE_GETHOSTBYADDR_R)
    message(FATAL_ERROR "gethostbyaddr() not found.")
endif()

check_function_exists(getaddrinfo BITD_HAVE_GETADDRINFO)
check_function_exists(getnameinfo BITD_HAVE_GETNAMEINFO)
check_function_exists(gai_strerror BITD_HAVE_GAI_STRERROR)

# OSX poll() is reputed to be broken in some releases
if(NOT APPLE)
  check_function_exists(poll BITD_HAVE_POLL)
endif()

check_function_exists(gettimeofday BITD_HAVE_GETTIMEOFDAY)

if (NOT BITD_HAVE_GETTIMEOFDAY AND NOT WIN32)
    message(FATAL_ERROR "gettimeofday() not found.")
endif()

check_function_exists(clock_gettime BITD_HAVE_CLOCK_GETTIME)

if (NOT BITD_HAVE_CLOCK_GETTIME AND NOT WIN32)
    message(FATAL_ERROR "clock_gettime() not found.")
endif()

check_function_exists(posix_spawn BITD_HAVE_POSIX_SPAWN)

check_function_exists(pipe2 BITD_HAVE_PIPE2)

check_function_exists(random BITD_HAVE_RANDOM)
check_function_exists(rand BITD_HAVE_RAND)
if (NOT BITD_HAVE_RANDOM AND NOT BITD_HAVE_RAND)
    message(FATAL_ERROR "random() or rand() not found.")
endif()

check_function_exists(srandom BITD_HAVE_SRANDOM)
check_function_exists(srand BITD_HAVE_SRAND)
if (NOT BITD_HAVE_RANDOM AND NOT BITD_HAVE_RAND)
    message(FATAL_ERROR "srandom() or srand() not found.")
endif()

#
# Check for symbols in utility libraries
#
check_function_exists(XML_ParserCreate BITD_HAVE_XML_PARSERCREATE)
if (NOT BITD_HAVE_XML_PARSERCREATE)
    message(FATAL_ERROR "XML_ParserCreate() not found. Libexpat not installed.")
endif()

check_function_exists(yaml_parser_parse BITD_HAVE_YAML_PARSER_PARSE)
if (NOT BITD_HAVE_YAML_PARSER_PARSE)
    message(FATAL_ERROR "yaml_parser_parse() not found. Libyaml not installed.")
endif()

check_function_exists(json_load_file BITD_HAVE_JSON_LOAD_FILE)
if (NOT BITD_HAVE_JSON_LOAD_FILE)
    message(FATAL_ERROR "json_load_file() not found. Libjansson not installed.")
endif()

# Change the CMAKE_REQUIRED_LIBRARIES to target specific util libraries
set(CMAKE_REQUIRED_LIBRARIES ${CURL_LIBRARIES})
check_function_exists(curl_easy_init BITD_HAVE_CURL_EASY_INIT)
if (NOT BITD_HAVE_CURL_EASY_INIT)
    message(FATAL_ERROR "curl_easy_init() not found. Libcurl not installed.")
endif()

set(CMAKE_REQUIRED_LIBRARIES ${MICROHTTPD_LIBRARIES})
check_function_exists(MHD_start_daemon BITD_HAVE_MHD_START_DAEMON)
if (NOT BITD_HAVE_MHD_START_DAEMON)
    message(FATAL_ERROR "MHD_start_daemon() not found. Libmicrohttpd not installed.")
endif()
