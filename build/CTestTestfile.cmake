# CMake generated Testfile for 
# Source directory: /home/omid/magnetic-dipole-simulator
# Build directory: /home/omid/magnetic-dipole-simulator/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(dipole_tests "/home/omid/magnetic-dipole-simulator/build/tests")
set_tests_properties(dipole_tests PROPERTIES  _BACKTRACE_TRIPLES "/home/omid/magnetic-dipole-simulator/CMakeLists.txt;46;add_test;/home/omid/magnetic-dipole-simulator/CMakeLists.txt;0;")
subdirs("_deps/catch2-build")
