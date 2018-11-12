# The bitd-agent executable
add_executable(bitd-object bitd-object.c)
target_link_libraries(bitd-object bitd)

install(TARGETS bitd-object
        RUNTIME DESTINATION bin COMPONENT Runtime
	LIBRARY DESTINATION lib${BITD_LIBSUFFIX} COMPONENT Development
	ARCHIVE DESTINATION lib${BITD_LIBSUFFIX} COMPONENT Development
	)
