link_libraries(bitd)

# Module dlls
add_library(bitd-echo SHARED echo.c)

add_library(bitd-assert SHARED assert.c)
target_link_libraries(bitd-assert bitd)

if (WIN32 OR BITD_HAVE_POSIX_SPAWN)
  add_library(bitd-exec SHARED exec.c)
  set(BITD_MODULE_EXEC bitd-exec)
endif()

add_library(bitd-ssl SHARED base/ssl.c)
target_link_libraries(bitd-ssl ${OPENSSL_LIBRARIES})

add_library(bitd-curl SHARED base/curl.c)
target_link_libraries(bitd-curl ${CURL_LIBRARIES} bitd-ssl)

add_library(bitd-config-log SHARED config/log.c)

add_library(bitd-sink-graphite SHARED sink/graphite.c)
target_link_libraries(bitd-sink-graphite bitd-curl)

add_library(bitd-sink-influxdb SHARED sink/influxdb.c)
target_link_libraries(bitd-sink-influxdb bitd-curl)

if (WIN32)
  # Ensure dlls do not use the 'lib' prefix when compiled on Cygwin mingw
  set_target_properties(bitd-assert PROPERTIES PREFIX "")
  set_target_properties(bitd-echo PROPERTIES PREFIX "")
  set_target_properties(bitd-exec PROPERTIES PREFIX "")
  set_target_properties(bitd-ssl PROPERTIES PREFIX "")
  set_target_properties(bitd-curl PROPERTIES PREFIX "")
  set_target_properties(bitd-config-log PROPERTIES PREFIX "")
  set_target_properties(bitd-sink-graphite PROPERTIES PREFIX "")
  set_target_properties(bitd-sink-influxdb PROPERTIES PREFIX "")
endif()

install(TARGETS bitd-echo ${BITD_MODULE_EXEC} 
	bitd-ssl bitd-curl bitd-config-log bitd-sink-graphite bitd-sink-influxdb
        RUNTIME DESTINATION bin COMPONENT Runtime
	LIBRARY DESTINATION lib${BITD_LIBSUFFIX} COMPONENT Development
	ARCHIVE DESTINATION lib${BITD_LIBSUFFIX} COMPONENT Development
	)
