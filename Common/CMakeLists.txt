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
  Compositor.cpp
  Image.cpp
  ImageRGBAFloatColorOnly.cpp
  ImageRGBAUByteColorFloatDepth.cpp
  ImageRGBAUByteColorOnly.cpp
  ImageRGBFloatColorDepth.cpp
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
  Image.hpp
  ImageColorDepth.hpp
  ImageColorOnly.hpp
  ImageRGBAFloatColorOnly.hpp
  ImageRGBAUByteColorFloatDepth.hpp
  ImageRGBAUByteColorOnly.hpp
  ImageRGBFloatColorDepth.hpp
  MainLoop.hpp
  MakeBox.hpp
  Mesh.hpp
  MeshHelper.hpp
  ReadSTL.hpp
  SavePPM.hpp
  Timer.hpp
  Triangle.hpp
  YamlWriter.hpp
  )

miniGraphics_create_config_header("common library")

add_library(miniGraphicsCommon ${srcs} ${headers})

set_source_files_properties(${headers} HEADER_ONLY TRUE)

miniGraphics_target_features(miniGraphicsCommon)
