# The bitdribble dll
add_library(bitd SHARED 
            platform-addrinfo.c
            platform-sys.c
            platform-mutex.c
            platform-thread.c
            platform-poll-event.c
            platform-event.c
            platform-time.c
            platform-hostent.c
            platform-inetutil.c
            platform-file.c
	    platform-netdb.c
            platform-socket.c
            platform-random.c
            platform-dll.c
            base64.c
            file.c
            format.c
            hash.c
            lambda.c
            log.c
            msg.c
            pack.c
            timer-list.c
            timer-thread.c
            tstamp.c
            types.c
            types-assert.c
            types-xml.c
            types-yaml.c
            mmr-api.c
            mmr-module.c
            mmr-log.c
            mmr-task.c
            mmr-task-inst.c
            mmr-event-loop.c
            mmr-sched.c
            mmr-utils.c
)

#
# Platform-specific target sources
#
if (CMAKE_USE_WIN32_THREADS_INIT)
   target_sources(bitd PRIVATE platform-thread-windows.c)
elseif (CMAKE_USE_PTHREADS_INIT)
   target_sources(bitd PRIVATE platform-thread-pthread.c)
else()
   message(FATAL_ERROR "Platform pthread library not found.")
endif()

#
# Set target link libraries
#
target_link_libraries(bitd ${BITD_LIBRARIES})

if (WIN32)
   set_target_properties(bitd PROPERTIES PREFIX "")
endif(WIN32)

# The library version
set_target_properties(bitd PROPERTIES
  SOVERSION ${BITD_VERSION_MAJOR}
  VERSION ${BITD_VERSION})

# All exes and dlls link with the bitdribble library
link_libraries(bitd)

# All exes and libs will include bitdribble headers
target_include_directories(bitd PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>)

install(TARGETS bitd
        RUNTIME DESTINATION bin COMPONENT Runtime
	LIBRARY DESTINATION lib${BITD_LIBSUFFIX} COMPONENT Development
	ARCHIVE DESTINATION lib${BITD_LIBSUFFIX} COMPONENT Development
	)

