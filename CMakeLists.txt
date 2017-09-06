# Copyright (C) 2017 Hewlett Packard Enterprise Development LP.
# All Rights Reserved.
#
# The contents of this software are proprietary and confidential
# to the Hewlett Packard Enterprise Development LP.Â Â No part of this
# program may be photocopied, reproduced, or translated into another
# programming language without prior written consent of the
# Hewlett Packard Enterprise Development LP.

cmake_minimum_required (VERSION 2.8)
project (tiny_msgbus)

set (TINY_MSGBUS tiny_msgbus)

include(FindPkgConfig)
find_package(Threads REQUIRED)
find_library(ZMQ_LIBRARY zmq REQUIRED)

include_directories (
    include
)

set (SOURCE
    ${CMAKE_SOURCE_DIR}/src/tiny_msgbus.c
)

add_library (${TINY_MSGBUS}
    ${SOURCE}
)

set_target_properties(${TINY_MSGBUS}
    PROPERTIES
    COMPILE_FLAGS "-std=c11 -Wall -Werror -ggdb -g3 -rdynamic -O0"
)

target_include_directories(${TINY_MSGBUS} PUBLIC
    ${ZMQ_LIBRARY_INCLUDEDIR}
)
target_link_libraries(${TINY_MSGBUS} PUBLIC
    ${ZMQ_LIBRARY}
)


add_subdirectory(tests)
