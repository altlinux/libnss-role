set(WRAPPER_PASSWD "${CMAKE_CURRENT_BINARY_DIR}/data/passwd")
set(WRAPPER_GROUP "${CMAKE_CURRENT_BINARY_DIR}/data/group")
set(WRAPPER_HOSTS "${CMAKE_CURRENT_BINARY_DIR}/data/hosts")

configure_file(test_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/test_config.h)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(TEST_SOURCES
	test_main.c
	test_parser.c
	test_paths.c
	test_unavail.c
	test_memory.c
	test_fileop_rw.c
	test_roles.c
	test_files.c)

add_mocked_test(main
	SOURCES ${TEST_SOURCES}
	COMPILE_OPTIONS -g3
	LINK_LIBRARIES role nss_role pam pam_misc ${CMAKE_DL_LIBS})

target_include_directories(test_main PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../include
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/include)

set(TEST_ENVIRONMENT
	LD_PRELOAD=libnss_wrapper.so
	NSS_WRAPPER_PASSWD=${WRAPPER_PASSWD}
	NSS_WRAPPER_GROUP=${WRAPPER_GROUP}
	NSS_WRAPPER_HOSTS=${WRAPPER_HOSTS})

# Run with ctest too
set_tests_properties(test_main PROPERTIES
	ENVIRONMENT "${TEST_ENVIRONMENT}")

option(ENABLE_TESTS_VALGRIND "Also run unit tests under valgrind with ctest" OFF)
if(ENABLE_TESTS_VALGRIND)
	find_program(VALGRIND valgrind)
	if(NOT VALGRIND)
		message(FATAL_ERROR "valgrind is required for ENABLE_TESTS_VALGRIND")
	endif()
	add_test(NAME test_main_valgrind
		COMMAND ${VALGRIND} --error-exitcode=1
			--leak-check=full --errors-for-leak-kinds=definite
			$<TARGET_FILE:test_main>
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
	set_tests_properties(test_main_valgrind PROPERTIES
		ENVIRONMENT "${TEST_ENVIRONMENT}")
endif()

add_custom_target(all_tests ALL
	DEPENDS test_main)

add_custom_command(TARGET all_tests
	COMMENT "Run tests under NSS wrapper"
	POST_BUILD COMMAND
		/usr/bin/env
			LD_PRELOAD=libnss_wrapper.so
			NSS_WRAPPER_PASSWD=${WRAPPER_PASSWD}
			NSS_WRAPPER_GROUP=${WRAPPER_GROUP}
			NSS_WRAPPER_HOSTS=${WRAPPER_HOSTS}
			NSS_WRAPPER_DEBUGLEVEL=3
			$<TARGET_FILE:test_main>
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	BYPRODUCTS
		${CMAKE_CURRENT_BINARY_DIR}/data/role.test.add.role
		${CMAKE_CURRENT_BINARY_DIR}/data/role.test.del.role
		${CMAKE_CURRENT_BINARY_DIR}/data/role.test.drop.role
		${CMAKE_CURRENT_BINARY_DIR}/data/role.test.int.new.role
		${CMAKE_CURRENT_BINARY_DIR}/data/role.test.set.role
		${CMAKE_CURRENT_BINARY_DIR}/data/role.test.string.new.role
	VERBATIM)
