## miniGraphics is distributed under the OSI-approved BSD 3-clause License.
## See LICENSE.txt for details.
##
## Copyright (c) 2017
## National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
## the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
## certain rights in this software.

cmake_minimum_required(VERSION 3.3)

project(miniGraphics)

option(MINIGRAPHICS_ENABLE_TESTING "Turn on/off building tests." ON)
if (MINIGRAPHICS_ENABLE_TESTING)
  enable_testing()
endif()

# These macros are not stricly necessary for this top-level CMake file, but
# by including it here, we can find the necessary packages once and this
# will significantly speed up when this file is included in subdirectories.
include(CMake/miniGraphicsMacros.cmake)

add_subdirectory(BinarySwap)
add_subdirectory(DirectSend)
add_subdirectory(2-3-Swap)
add_subdirectory(RadixK)

option(MINIGRAPHICS_ENABLE_ICET "Turn on/off building IceT miniapp." ON)
if (MINIGRAPHICS_ENABLE_ICET)
  add_subdirectory(IceT)
endif()
