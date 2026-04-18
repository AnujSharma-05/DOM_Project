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
    src/MutationQueue.cpp
    src/FrameLoop.cpp
    src/FrameRunner.cpp
    src/Serializer.cpp
    src/Document.cpp
    src/adapters/ChessBoardAdapter.cpp
)

target_include_directories(dom_engine_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (MSVC)
    target_compile_options(dom_engine_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(dom_engine_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(dom_engine_app src/main.cpp)
target_link_libraries(dom_engine_app PRIVATE dom_engine_core)

add_executable(chess_demo examples/chess_pgn_demo.cpp)
target_link_libraries(chess_demo PRIVATE dom_engine_core)

add_executable(test_phase1 tests/test_phase1.cpp)
target_link_libraries(test_phase1 PRIVATE dom_engine_core)

add_executable(bench_layout tests/bench_layout.cpp)
target_link_libraries(bench_layout PRIVATE dom_engine_core)

add_executable(test_phase2 tests/test_phase2.cpp)
target_link_libraries(test_phase2 PRIVATE dom_engine_core)

add_executable(test_phase2_integration tests/test_phase2_integration.cpp)
target_link_libraries(test_phase2_integration PRIVATE dom_engine_core)

add_executable(test_phase3 tests/test_phase3.cpp)
target_link_libraries(test_phase3 PRIVATE dom_engine_core)

add_executable(test_phase4 tests/test_phase4.cpp)
target_link_libraries(test_phase4 PRIVATE dom_engine_core)

add_executable(test_coverage tests/test_coverage.cpp)
target_link_libraries(test_coverage PRIVATE dom_engine_core)

add_executable(test_stress tests/test_stress.cpp)
target_link_libraries(test_stress PRIVATE dom_engine_core)

enable_testing()
add_test(NAME phase1_tests COMMAND test_phase1)
set_tests_properties(phase1_tests PROPERTIES LABELS "unit")
add_test(NAME phase2_tests COMMAND test_phase2)
set_tests_properties(phase2_tests PROPERTIES LABELS "unit")
add_test(NAME phase2_integration_tests COMMAND test_phase2_integration)
set_tests_properties(phase2_integration_tests PROPERTIES LABELS "unit")
add_test(NAME phase3_tests COMMAND test_phase3)
set_tests_properties(phase3_tests PROPERTIES LABELS "unit")
add_test(NAME phase4_tests COMMAND test_phase4)
set_tests_properties(phase4_tests PROPERTIES LABELS "unit")
add_test(NAME coverage_tests COMMAND test_coverage)
set_tests_properties(coverage_tests PROPERTIES LABELS "unit")
add_test(NAME stress_tests COMMAND test_stress)
set_tests_properties(stress_tests PROPERTIES LABELS "stress")
add_test(NAME bench_layout_tests COMMAND bench_layout)
set_tests_properties(bench_layout_tests PROPERTIES LABELS "bench")
