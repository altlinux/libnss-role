add_mocked_test(main
	SOURCES test_main.c
	COMPILE_OPTIONS -g3 -I${CMAKE_CURRENT_SOURCE_DIR}/../include
	LINK_LIBRARIES role pam pam_misc)

