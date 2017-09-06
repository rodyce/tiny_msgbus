cmake_minimum_required (VERSION 2.8)

include(CTest)

# Add directory with the 'Unity' unit test library
add_subdirectory(unity)

set(TINY_MSGBUS_UT tiny_msgbus_ut)

set(UT_SOURCE
    api_tests.c
)

add_executable(${TINY_MSGBUS_UT}
    ${SOURCE}
    ${UT_SOURCE}
)

set_target_properties(${TINY_MSGBUS_UT}
    PROPERTIES
    COMPILE_FLAGS "-std=c11 -Wall -Werror -ggdb -g3 -rdynamic -O0"
    COMPILE_DEFINITIONS UNITTEST
)


add_dependencies(${TINY_MSGBUS_UT}
    ${TINY_MSGBUS}
)

target_include_directories(${TINY_MSGBUS_UT} PUBLIC
    ${ZMQ_LIBRARY_INCLUDEDIR}
)

target_link_libraries(${TINY_MSGBUS_UT} PUBLIC
    ${ZMQ_LIBRARY}
    unity
    pthread
)

enable_testing()
add_test(tiny_msgbus_tests ${TINY_MSGBUS_UT})

add_custom_target(check COMMAND ${TINY_MSGBUS_UT}
    DEPENDS ${TINY_MSGBUS_UT}
)
