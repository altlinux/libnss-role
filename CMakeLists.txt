cmake_minimum_required(VERSION 3.10)
project(libnss-role VERSION 0.4.1 DESCRIPTION "NSS library to work with roles")
set(C_STANDARD 89)
set(C_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

add_compile_options(-pedantic -g3)

option(ENABLE_TESTS "Enable unit tests for libnss-role" ON)

if(ENABLE_TESTS)
	find_package(CMocka CONFIG REQUIRED)
	include(AddCMockaTest)
	include(AddMockedTest)
	enable_testing()
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test)
endif()

set(NSS_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/common.c
	${CMAKE_CURRENT_SOURCE_DIR}/lock_file.c
	${CMAKE_CURRENT_SOURCE_DIR}/pam_check.c
	${CMAKE_CURRENT_SOURCE_DIR}/parser.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/fileop.c)

set(LIBROLE_VERSION 1.0.0)
set(LIBNSSROLE_VERSION 2.0.0)

configure_file(version.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/role/version.h)

include_directories(BEFORE
	${CMAKE_CURRENT_BINARY_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(role SHARED
	${CMAKE_CURRENT_SOURCE_DIR}/graph.c
	${NSS_SOURCES})

set_target_properties(role PROPERTIES SOVERSION ${LIBROLE_VERSION})

add_library(nss_role SHARED
	${CMAKE_CURRENT_SOURCE_DIR}/nss_role.c
	${NSS_SOURCES})

set_target_properties(nss_role PROPERTIES SOVERSION ${LIBNSSROLE_VERSION})

add_executable(roleadd
	${CMAKE_CURRENT_SOURCE_DIR}/roleadd.c)

target_link_libraries(roleadd role pam pam_misc)

add_executable(roledel
	${CMAKE_CURRENT_SOURCE_DIR}/roledel.c)

target_link_libraries(roledel role pam pam_misc)

add_executable(rolelst
	${CMAKE_CURRENT_SOURCE_DIR}/rolelst.c)

target_link_libraries(rolelst role pam pam_misc)

