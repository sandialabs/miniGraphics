## miniGraphics is distributed under the OSI-approved BSD 3-clause License.
## See LICENSE.txt for details.
##
## Copyright (c) 2017
## National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
## the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
## certain rights in this software.

if(NOT TARGET IceTCore)
  set(ICET_USE_OPENGL OFF CACHE INTERNAL "Always OFF")
  set(ICET_USE_OSMESA OFF CACHE INTERNAL "Always OFF")
  set(ICET_USE_OFFSCREEN_EGL OFF CACHE INTERNAL "Always OFF")
  set(ICET_USE_MPI ON CACHE INTERNAL "Always ON")
  set(ICET_BUILD_TESTING OFF CACHE INTERNAL "Always OFF")

  add_subdirectory(miniGraphicsIceT)
endif()
