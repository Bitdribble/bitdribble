# The bitd-agent executable
add_executable(bitd-agent bitd-agent.c)
target_link_libraries(bitd-agent bitd)

install(TARGETS bitd-agent
        RUNTIME DESTINATION bin COMPONENT Runtime
	LIBRARY DESTINATION lib${BITD_LIBSUFFIX} COMPONENT Development
	ARCHIVE DESTINATION lib${BITD_LIBSUFFIX} COMPONENT Development
	)
