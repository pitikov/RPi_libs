cmake_minimum_required(VERSION 2.8)

project(RPi_libs)
find_library(PIGPIOD_LIBS NAMES pigpiod_if2 rt)

add_library(mcp3008 SHARED mcp3008.cpp)
set(PUBLIC_HEADER mcp3008.h)
target_link_libraries(mcp3008 ${PIGPIOD_LIBS} pthread)
target_link_options(mcp3008 PUBLIC ${THREADS_PREFER_PTHREAD_FLAG})

add_executable(RPi_libs_test main.cpp)
target_link_libraries(RPi_libs_test ${PIGPIOD_LIBS} pthread mcp3008)
target_link_options(RPi_libs_test PUBLIC ${THREADS_PREFER_PTHREAD_FLAG})

install(TARGETS mcp3008 LIBRARY DESTINATION lib)
install(FILES mcp3008.h DESTINATION include)
