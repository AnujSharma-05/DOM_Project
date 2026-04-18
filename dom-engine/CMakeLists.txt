cmake_minimum_required(VERSION 3.22)
project(dom_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(dom_engine_core
    src/Node.cpp
    src/Diff.cpp
    src/Renderer.cpp
    src/LayoutEngine.cpp
    src/CharBuffer.cpp
    src/MutationObserver.cpp
    src/FrameLoop.cpp
    src/FrameRunner.cpp
)

target_include_directories(dom_engine_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (MSVC)
    target_compile_options(dom_engine_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(dom_engine_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(dom_engine_app src/main.cpp)
target_link_libraries(dom_engine_app PRIVATE dom_engine_core)

add_executable(test_phase1 tests/test_phase1.cpp)
target_link_libraries(test_phase1 PRIVATE dom_engine_core)

add_executable(bench_layout tests/bench_layout.cpp)
target_link_libraries(bench_layout PRIVATE dom_engine_core)

add_executable(test_phase2 tests/test_phase2.cpp)
target_link_libraries(test_phase2 PRIVATE dom_engine_core)

add_executable(test_phase2_integration tests/test_phase2_integration.cpp)
target_link_libraries(test_phase2_integration PRIVATE dom_engine_core)

enable_testing()
add_test(NAME phase1_tests COMMAND test_phase1)
add_test(NAME phase2_tests COMMAND test_phase2)
add_test(NAME phase2_integration_tests COMMAND test_phase2_integration)
