## miniGraphics is distributed under the OSI-approved BSD 3-clause License.
## See LICENSE.txt for details.
##
## Copyright (c) 2017
## National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
## the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
## certain rights in this software.

# This file should only be called from miniGraphicsMacros.cmake

cmake_minimum_required(VERSION 3.3)

project(miniGraphicsPaint CXX)

option(MINIGRAPHICS_ENABLE_OPENGL
  "Turn on/off the OpenGL rendering system."
  OFF
  )

set(srcs
  PainterSimple.cpp
  )

set(headers
  ${CMAKE_CURRENT_BINARY_DIR}/miniGraphicsConfig.h
  Painter.hpp
  )

set(include_dirs)
set(libs)

if(MINIGRAPHICS_ENABLE_OPENGL)
  message(WARNING
    "OpenGL rendering currently is not working. "
    "It will (potentially) compile, and it looks like it runs correctly, "
    "but it does not create valid images.")

  find_package(OpenGL REQUIRED)
  find_package(GLEW REQUIRED)
  find_package(GLFW REQUIRED)

  set(srcs ${srcs}
    ${miniGraphics_SOURCE_DIR}/Paint/PainterOpenGL.cpp
    ${miniGraphics_SOURCE_DIR}/Paint/OpenGL_common/shader.cpp
    )

  set(include_dirs ${include_dirs}
    ${OPENGL_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    )

  set(libs ${libs}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${GLFW_LIBRARIES}
    )
endif()

miniGraphics_create_config_header()

add_library(miniGraphicsPaint ${srcs} ${headers})

miniGraphics_target_features(miniGraphicsPaint)

target_include_directories(miniGraphicsPaint PRIVATE ${include_dirs})

target_link_libraries(miniGraphicsPaint PRIVATE ${libs})

set_source_files_properties(miniGraphicsPaint HEADER_ONLY TRUE)
