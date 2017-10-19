# CMake generated Testfile for 
# Source directory: /home/mgpeter/Documents/XViz/miniGraphics/Composition/IceT-2-1-1/tests
# Build directory: /home/mgpeter/Documents/XViz/miniGraphics/Composition/IceT-2-1-1/build/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(IceTBackgroundCorrect "/usr/bin/mpiexec" "-np" "2" "/home/mgpeter/Documents/XViz/miniGraphics/Composition/IceT-2-1-1/build/bin/icetTests_mpi" "BackgroundCorrect")
set_tests_properties(IceTBackgroundCorrect PROPERTIES  FAIL_REGULAR_EXPRESSION ":ERROR:;TEST NOT RUN;TEST NOT PASSED;TEST FAILED" PASS_REGULAR_EXPRESSION "Test Passed")
add_test(IceTCompressionSize "/usr/bin/mpiexec" "-np" "2" "/home/mgpeter/Documents/XViz/miniGraphics/Composition/IceT-2-1-1/build/bin/icetTests_mpi" "CompressionSize")
set_tests_properties(IceTCompressionSize PROPERTIES  FAIL_REGULAR_EXPRESSION ":ERROR:;TEST NOT RUN;TEST NOT PASSED;TEST FAILED" PASS_REGULAR_EXPRESSION "Test Passed")
add_test(IceTInterlace "/usr/bin/mpiexec" "-np" "2" "/home/mgpeter/Documents/XViz/miniGraphics/Composition/IceT-2-1-1/build/bin/icetTests_mpi" "Interlace")
set_tests_properties(IceTInterlace PROPERTIES  FAIL_REGULAR_EXPRESSION ":ERROR:;TEST NOT RUN;TEST NOT PASSED;TEST FAILED" PASS_REGULAR_EXPRESSION "Test Passed")
add_test(IceTMaxImageSplit "/usr/bin/mpiexec" "-np" "2" "/home/mgpeter/Documents/XViz/miniGraphics/Composition/IceT-2-1-1/build/bin/icetTests_mpi" "MaxImageSplit")
set_tests_properties(IceTMaxImageSplit PROPERTIES  FAIL_REGULAR_EXPRESSION ":ERROR:;TEST NOT RUN;TEST NOT PASSED;TEST FAILED" PASS_REGULAR_EXPRESSION "Test Passed")
add_test(IceTOddImageSizes "/usr/bin/mpiexec" "-np" "2" "/home/mgpeter/Documents/XViz/miniGraphics/Composition/IceT-2-1-1/build/bin/icetTests_mpi" "OddImageSizes")
set_tests_properties(IceTOddImageSizes PROPERTIES  FAIL_REGULAR_EXPRESSION ":ERROR:;TEST NOT RUN;TEST NOT PASSED;TEST FAILED" PASS_REGULAR_EXPRESSION "Test Passed")
add_test(IceTOddProcessCounts "/usr/bin/mpiexec" "-np" "2" "/home/mgpeter/Documents/XViz/miniGraphics/Composition/IceT-2-1-1/build/bin/icetTests_mpi" "OddProcessCounts")
set_tests_properties(IceTOddProcessCounts PROPERTIES  FAIL_REGULAR_EXPRESSION ":ERROR:;TEST NOT RUN;TEST NOT PASSED;TEST FAILED" PASS_REGULAR_EXPRESSION "Test Passed")
add_test(IceTRadixkUnitTests "/usr/bin/mpiexec" "-np" "2" "/home/mgpeter/Documents/XViz/miniGraphics/Composition/IceT-2-1-1/build/bin/icetTests_mpi" "RadixkUnitTests")
set_tests_properties(IceTRadixkUnitTests PROPERTIES  FAIL_REGULAR_EXPRESSION ":ERROR:;TEST NOT RUN;TEST NOT PASSED;TEST FAILED" PASS_REGULAR_EXPRESSION "Test Passed")
add_test(IceTSimpleTiming "/usr/bin/mpiexec" "-np" "2" "/home/mgpeter/Documents/XViz/miniGraphics/Composition/IceT-2-1-1/build/bin/icetTests_mpi" "SimpleTiming")
set_tests_properties(IceTSimpleTiming PROPERTIES  FAIL_REGULAR_EXPRESSION ":ERROR:;TEST NOT RUN;TEST NOT PASSED;TEST FAILED" PASS_REGULAR_EXPRESSION "Test Passed")
add_test(IceTSparseImageCopy "/usr/bin/mpiexec" "-np" "2" "/home/mgpeter/Documents/XViz/miniGraphics/Composition/IceT-2-1-1/build/bin/icetTests_mpi" "SparseImageCopy")
set_tests_properties(IceTSparseImageCopy PROPERTIES  FAIL_REGULAR_EXPRESSION ":ERROR:;TEST NOT RUN;TEST NOT PASSED;TEST FAILED" PASS_REGULAR_EXPRESSION "Test Passed")
