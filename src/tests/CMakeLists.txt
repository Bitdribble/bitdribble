#
# Unit testing
#
find_program(VALGRIND "valgrind")
# message("Valgrind = ${VALGRIND}")

# All tests link against bitd library
link_libraries(bitd)

function(tt_add_test name command)
  add_test(NAME ${name} COMMAND ${command} ${ARGN} WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endfunction()

function(ttv_add_test name command)
  if (VALGRIND)
    add_test(NAME valgrind-${name} COMMAND valgrind --leak-check=full --track-fds=yes --error-exitcode=1 ${command} ${ARGN} WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  else()
    add_test(NAME ${name} COMMAND ${command} ${ARGN} WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  endif()
endfunction()

# Test executables and dlls
add_executable(test-dll test-dll.c)
add_library(test-lib1 STATIC test-lib1.c)
add_library(test-lib2 STATIC test-lib2.c)
add_library(test-dll1 SHARED test-dll1.c)
target_link_libraries(test-dll1 test-lib1)
add_library(test-dll2 SHARED test-dll2.c)
target_link_libraries(test-dll2 test-lib2)
add_executable(test-getaddrinfo test-getaddrinfo.c)
add_executable(test-gethostbyname test-gethostbyname.c)
add_executable(test-mutex test-mutex.c)
add_executable(test-sleep test-sleep.c)
add_executable(test-tcp test-tcp.c)
add_executable(test-thread test-thread.c)
add_executable(test-time test-time.c)
add_executable(test-base64 test-base64.c)
add_executable(test-hash test-hash.c)
add_executable(test-log test-log.c)
add_executable(test-msg test-msg.c)
add_executable(test-nvp-string test-nvp-string.c)
add_executable(test-pack test-pack.c)
add_executable(test-resolve-hostport test-resolve-hostport.c)
add_executable(test-tcp-ts test-tcp-ts.c)
add_executable(test-timer-list test-timer-list.c)
add_executable(test-timer-thread test-timer-thread.c)
add_executable(test-lambda test-lambda.c)

add_executable(test-nvp-merge test-nvp-merge.c)

if (WIN32)
  # Ensure dlls do not use the 'lib' prefix when compiled on Cygwin mingw
  set_target_properties(test-dll1 PROPERTIES PREFIX "")
  set_target_properties(test-dll2 PROPERTIES PREFIX "")
endif()

# Get the name of the folder that stores DLLs
if (CYGWIN OR WIN32)
  set(TEST_DLL_DIR "bin")
else()
  set(TEST_DLL_DIR "lib")
endif()

# Get relative path to the config directory. This is necessary for WIN32
# tests, which can't parse /cygwin/c/... but can parse c:/... and
# can also parse relative paths
file(RELATIVE_PATH TEST_CONFIG ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src/tests/config)

# 
# The unit tests. Long tests have 'long' in the name - they can be skipped
# by running 'ctest -E long'
#
# Warning: running tests in parallel with '-j' will cause errors
#
tt_add_test(test-dll1 bin/test-dll -lp ${TEST_DLL_DIR} -dll-load test-dll1 -dll-load test-dll2 -dll-exec-ret test-dll1 test_dll 1)
tt_add_test(test-dll2 bin/test-dll -lp ${TEST_DLL_DIR} -dll-load test-dll1 -dll-load test-dll2 -dll-exec-ret test-dll2 test_dll 2)
tt_add_test(test-mutex bin/test-mutex -n 500)
ttv_add_test(test-mutex2 bin/test-mutex -n 5)
tt_add_test(test-mutex-long bin/test-mutex -n 500 -ms 1 -ts 5000)
ttv_add_test(test-tcp bin/test-tcp -n 1 -v 0)
ttv_add_test(test-getaddrinfo bin/test-getaddrinfo -v 0 localhost)
ttv_add_test(test-getaddrinfo-reverse bin/test-getaddrinfo -v 0 127.0.0.1)
ttv_add_test(test-gethostbyname bin/test-gethostbyname -v 0 localhost)
ttv_add_test(test-gethostbyname-reverse bin/test-gethostbyname -v 0 127.0.0.1)
ttv_add_test(test-hash bin/test-hash -n 10)
ttv_add_test(test-msg bin/test-msg -n 50)
ttv_add_test(test-timer-list bin/test-timer-list -v 0 -t 1 -t 5 -t 25)
tt_add_test(test-timer-list-long bin/test-timer-list -thc 10 100 -thc 10 200 -thc 0 300)
ttv_add_test(test-timer-thread bin/test-timer-thread -v 0 -tp 1 -tp 2 -t 5 -t 10 -s 120)
ttv_add_test(test-lambda bin/test-lambda -v 0 -s 0)
tt_add_test(test-lambda-long1 bin/test-lambda -n 1 -tc 10 -ts 0 -tbs 1100 --idle-tmo 1000 -s 1000)
tt_add_test(test-lambda-long2 bin/test-lambda -n 100 -tc 300 -ts 1000)
ttv_add_test(test-resolve-hostport bin/test-resolve-hostport -v 0 localhost:1)
ttv_add_test(test-resolve-hostport-ip6 bin/test-resolve-hostport -v 0 [::1]:1)
ttv_add_test(test-pack-bool-true bin/test-pack -t boolean true)
ttv_add_test(test-pack-bool-false bin/test-pack -t boolean false)
ttv_add_test(test-pack-int64-1 bin/test-pack -t int64 1)
ttv_add_test(test-pack-int64-2 bin/test-pack -t int64 1234567812345678)
ttv_add_test(test-pack-int64-3 bin/test-pack -t int64 -1)
ttv_add_test(test-pack-int64-4 bin/test-pack -t int64 -1234567812345678)
ttv_add_test(test-pack-uint64-1 bin/test-pack -t uint64 1)
ttv_add_test(test-pack-uint64-2 bin/test-pack -t uint64 1234567812345678)
ttv_add_test(test-pack-double-1 bin/test-pack -t double 1.10101010101)
ttv_add_test(test-pack-double-2 bin/test-pack -t double 1.101010101010101010101010101)
ttv_add_test(test-pack-string bin/test-pack -t string abc)
ttv_add_test(test-pack-blob-1 bin/test-pack -t blob abcd)
ttv_add_test(test-pack-blob-2 bin/test-pack -t blob 01)
ttv_add_test(bitd-object-xml-input bin/bitd-object -ix ${TEST_CONFIG}/nvp.xml)
ttv_add_test(bitd-object-xml-pack bin/bitd-object -ix ${TEST_CONFIG}/nvp.xml -p)
ttv_add_test(bitd-object-xml-chunk bin/bitd-object -ix ${TEST_CONFIG}/nvp.xml -chunk -unchunk -sort)
ttv_add_test(bitd-object-xml-to-string bin/bitd-object -ix ${TEST_CONFIG}/string.xml -oxe ${TEST_CONFIG}/string.xml)
ttv_add_test(bitd-object-xml bin/bitd-object -ix ${TEST_CONFIG}/nvp.xml -oxe ${TEST_CONFIG}/nvp.xml)
ttv_add_test(bitd-object-xml-full bin/bitd-object -ix ${TEST_CONFIG}/nvp.xml -of -oxe ${TEST_CONFIG}/nvp-full.xml)
ttv_add_test(bitd-object-xml-sorted bin/bitd-object -ix ${TEST_CONFIG}/nvp.xml -of -sort -oxe ${TEST_CONFIG}/nvp-sorted.xml)
ttv_add_test(bitd-object-yaml bin/bitd-object -iy ${TEST_CONFIG}/nvp.yaml -oye ${TEST_CONFIG}/nvp.yaml)
ttv_add_test(bitd-object-yaml-full bin/bitd-object -iy ${TEST_CONFIG}/nvp.yaml -of -oye ${TEST_CONFIG}/nvp-full.yaml)
ttv_add_test(bitd-object-yaml-anchor bin/bitd-object -iy ${TEST_CONFIG}/nvp-anchor.yaml -oye ${TEST_CONFIG}/nvp-merged-anchor.yaml)
ttv_add_test(bitd-object-json bin/bitd-object -i ${TEST_CONFIG}/nvp.json -oje ${TEST_CONFIG}/nvp.json)
ttv_add_test(bitd-object-json-full bin/bitd-object -i ${TEST_CONFIG}/nvp-full.json -of -oje ${TEST_CONFIG}/nvp-full.json)
ttv_add_test(test-nvp-merge-a bin/test-nvp-merge -ix ${TEST_CONFIG}/merge-old.xml -ixb ${TEST_CONFIG}/merge-base.xml -ixn ${TEST_CONFIG}/merge-new.xml -oxe ${TEST_CONFIG}/merge-result.xml)
ttv_add_test(test-tcp-ts bin/test-tcp-ts -n 1 -enq -ack -v 0)
ttv_add_test(test-log bin/test-log -reg foo -msg trace foo test -unreg foo -s 1)

# Task instance tests
ttv_add_test(test-bitd-agent-echo-json bin/bitd-agent -c ${TEST_CONFIG}/echo/echo.json -mrc 1)
ttv_add_test(test-bitd-agent-echo-xml bin/bitd-agent -c ${TEST_CONFIG}/echo/echo.xml -mrc 1)
ttv_add_test(test-bitd-agent-echo-yml bin/bitd-agent -c ${TEST_CONFIG}/echo/echo.yml -mrc 1)
ttv_add_test(test-bitd-agent-exec bin/bitd-agent -c ${TEST_CONFIG}/exec/exec.yml -mrc 6)
