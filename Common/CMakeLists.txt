## miniGraphics is distributed under the OSI-approved BSD 3-clause License.
## See LICENSE.txt for details.
##
## Copyright (c) 2017
## National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
## the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
## certain rights in this software.

# This file should only be called from miniGraphicsMacros.cmake

cmake_minimum_required(VERSION 3.3)

project(miniGraphicsCommon CXX)

set(srcs
  Image.cpp
  ImageRGBAUByteColorFloatDepth.cpp
  MakeBox.cpp
  MainLoop.cpp
  Mesh.cpp
  MeshHelper.cpp
  ReadSTL.cpp
  SavePPM.cpp
  Timer.cpp
  YamlWriter.cpp
  )

set(headers
  ${CMAKE_CURRENT_BINARY_DIR}/miniGraphicsConfig.h
  Color.hpp
  Compositor.hpp
  Triangle.hpp
  )

miniGraphics_create_config_header()

add_library(miniGraphicsCommon ${srcs} ${headers})

set_source_files_properties(${headers} HEADER_ONLY TRUE)

miniGraphics_target_features(miniGraphicsCommon)
