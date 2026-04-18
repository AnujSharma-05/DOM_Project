# CMake generated Testfile for 
# Source directory: E:/Codes/DOM_Project/dom-engine
# Build directory: E:/Codes/DOM_Project/dom-engine/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[phase1_tests]=] "E:/Codes/DOM_Project/dom-engine/build/test_phase1.exe")
set_tests_properties([=[phase1_tests]=] PROPERTIES  LABELS "unit" _BACKTRACE_TRIPLES "E:/Codes/DOM_Project/dom-engine/CMakeLists.txt;62;add_test;E:/Codes/DOM_Project/dom-engine/CMakeLists.txt;0;")
add_test([=[phase2_tests]=] "E:/Codes/DOM_Project/dom-engine/build/test_phase2.exe")
set_tests_properties([=[phase2_tests]=] PROPERTIES  LABELS "unit" _BACKTRACE_TRIPLES "E:/Codes/DOM_Project/dom-engine/CMakeLists.txt;64;add_test;E:/Codes/DOM_Project/dom-engine/CMakeLists.txt;0;")
add_test([=[phase2_integration_tests]=] "E:/Codes/DOM_Project/dom-engine/build/test_phase2_integration.exe")
set_tests_properties([=[phase2_integration_tests]=] PROPERTIES  LABELS "unit" _BACKTRACE_TRIPLES "E:/Codes/DOM_Project/dom-engine/CMakeLists.txt;66;add_test;E:/Codes/DOM_Project/dom-engine/CMakeLists.txt;0;")
add_test([=[phase3_tests]=] "E:/Codes/DOM_Project/dom-engine/build/test_phase3.exe")
set_tests_properties([=[phase3_tests]=] PROPERTIES  LABELS "unit" _BACKTRACE_TRIPLES "E:/Codes/DOM_Project/dom-engine/CMakeLists.txt;68;add_test;E:/Codes/DOM_Project/dom-engine/CMakeLists.txt;0;")
add_test([=[phase4_tests]=] "E:/Codes/DOM_Project/dom-engine/build/test_phase4.exe")
set_tests_properties([=[phase4_tests]=] PROPERTIES  LABELS "unit" _BACKTRACE_TRIPLES "E:/Codes/DOM_Project/dom-engine/CMakeLists.txt;70;add_test;E:/Codes/DOM_Project/dom-engine/CMakeLists.txt;0;")
add_test([=[coverage_tests]=] "E:/Codes/DOM_Project/dom-engine/build/test_coverage.exe")
set_tests_properties([=[coverage_tests]=] PROPERTIES  LABELS "unit" _BACKTRACE_TRIPLES "E:/Codes/DOM_Project/dom-engine/CMakeLists.txt;72;add_test;E:/Codes/DOM_Project/dom-engine/CMakeLists.txt;0;")
add_test([=[stress_tests]=] "E:/Codes/DOM_Project/dom-engine/build/test_stress.exe")
set_tests_properties([=[stress_tests]=] PROPERTIES  LABELS "stress" _BACKTRACE_TRIPLES "E:/Codes/DOM_Project/dom-engine/CMakeLists.txt;74;add_test;E:/Codes/DOM_Project/dom-engine/CMakeLists.txt;0;")
add_test([=[bench_layout_tests]=] "E:/Codes/DOM_Project/dom-engine/build/bench_layout.exe")
set_tests_properties([=[bench_layout_tests]=] PROPERTIES  LABELS "bench" _BACKTRACE_TRIPLES "E:/Codes/DOM_Project/dom-engine/CMakeLists.txt;76;add_test;E:/Codes/DOM_Project/dom-engine/CMakeLists.txt;0;")
