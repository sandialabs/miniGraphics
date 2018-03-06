## miniGraphics is distributed under the OSI-approved BSD 3-clause License.
## See LICENSE.txt for details.
##
## Copyright (c) 2017
## National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
## the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
## certain rights in this software.

cmake_minimum_required(VERSION 3.3)

project(miniGraphics2-3-SwapBase CXX)

include(../../CMake/miniGraphicsMacros.cmake)

set(srcs
  main.cpp
  Swap_2_3_Base.cpp
  Swap_2_3_Node.cpp
  )

set(headers
  Swap_2_3_Base.hpp
  Swap_2_3_Node.hpp
  )

miniGraphics_executable(2-3-SwapBase
  SOURCES ${srcs}
  HEADERS ${headers}
  )
