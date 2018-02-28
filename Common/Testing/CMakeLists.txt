## miniGraphics is distributed under the OSI-approved BSD 3-clause License.
## See LICENSE.txt for details.
##
## Copyright (c) 2017
## National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
## the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
## certain rights in this software.

set(srcs
  ImageFullTest.cpp
  ImageSparseTest.cpp
  )

set(test_target miniGraphicsCommonTests)

create_test_sourcelist(test_srcs ${test_target}.cpp ${srcs})

miniGraphics_create_config_header("common library tests")

add_executable(${test_target} ${test_srcs})

target_link_libraries(${test_target} PRIVATE miniGraphicsCommon)
miniGraphics_target_features(${test_target})

foreach(test ${srcs})
  get_filename_component(test_name ${test} NAME_WE)
  add_test(NAME ${test_name}
    COMMAND ${test_target} ${test_name}
    )
endforeach()
