cmake_minimum_required(VERSION 3.15)
project(rtos_scheduler CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_library(rtsched_core
    src/Simulator.cpp
    src/TaskLoader.cpp
    src/JsonExport.cpp
    src/Report.cpp
)
target_include_directories(rtsched_core PUBLIC include)

add_executable(rtsched cli/main.cpp)
target_link_libraries(rtsched PRIVATE rtsched_core)

enable_testing()
add_executable(rtsched_tests tests/basic_test.cpp)
target_link_libraries(rtsched_tests PRIVATE rtsched_core)
add_test(NAME basic_test COMMAND rtsched_tests)
