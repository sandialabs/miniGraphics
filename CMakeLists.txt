## miniGraphics is distributed under the OSI-approved BSD 3-clause License.
## See LICENSE.txt for details.
##
## Copyright (c) 2017
## National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
## the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
## certain rights in this software.

cmake_minimum_required(VERSION 3.3)

project(miniGraphics)

include(CMake/miniGraphicsMacros.cmake)

set(srcs
  main.cpp
  BinarySwap.cpp
  )

miniGraphics_executable(BinarySwap ${srcs})
