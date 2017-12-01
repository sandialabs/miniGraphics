## miniGraphics is distributed under the OSI-approved BSD 3-clause License.
## See LICENSE.txt for details.
##
## Copyright (c) 2017
## National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
## the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
## certain rights in this software.

cmake_minimum_required(VERSION 3.3)

project(miniGraphics)

set(miniGraphics_CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${miniGraphics_CMAKE_MODULE_PATH})

add_executable(createData dataGen/createData.cpp)

option(MINIGRAPHICS_ENABLE_OPENGL
  "Turn on/off the OpenGL rendering system."
  OFF
  )

set(srcs
  main.cpp
  Objects/Image.cpp
  Objects/ImageRGBAUByteColorFloatDepth.cpp
  Objects/Mesh.cpp
  IO/ReadData.cpp
  IO/SavePPM.cpp
  Rendering/Renderer_Example.cpp
  Composition/Composition_Example.cpp
  )

set(headers
  ${CMAKE_CURRENT_BINARY_DIR}/miniGraphicsConfig.h
  Objects/Color.hpp
  Objects/Composition.hpp
  Objects/Renderer.hpp
  Objects/Triangle.hpp
  )

set(include_dirs
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glm/include
  ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/optionparser/include
  )

set(libs
  )

if(WIN32)
  # Don't deal with MSVC's unportable safety warnings
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
endif()

if(MINIGRAPHICS_ENABLE_OPENGL)
  message(WARNING
    "OpenGL rendering currently is not working. "
    "It will (potentially) compile, and it looks like it runs correctly, "
    "but it does not create valid images.")

  find_package(OpenGL REQUIRED)
  find_package(GLEW REQUIRED)
  find_package(GLFW REQUIRED)

  set(srcs ${srcs}
    Rendering/OpenGL_Example.cpp
    Rendering/OpenGL_common/shader.cpp
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

configure_file(miniGraphicsConfig.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/miniGraphicsConfig.h
  )

add_executable(run ${srcs} ${headers})

set_source_files_properties(${headers} HEADER_ONLY TRUE)

target_include_directories(run PRIVATE ${include_dirs})

target_link_libraries(run PRIVATE ${libs})

target_compile_features(run PRIVATE
  cxx_raw_string_literals
  )
