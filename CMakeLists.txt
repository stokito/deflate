cmake_minimum_required(VERSION 2.4.4)
project(zlib-deflate C)

# saves 4456 bytes
OPTION(NO_HELP "Build without --help" OFF)
IF(NO_HELP)
    ADD_DEFINITIONS(-DNO_HELP)
ENDIF()

# saves 184 bytes
OPTION(HELP_DESCRIPTION "Show option descriptions in --help" ON)
IF(HELP_DESCRIPTION)
    ADD_DEFINITIONS(-DHELP_DESCRIPTION)
ENDIF()

add_executable(deflate deflate.c)
target_link_libraries(deflate z)
