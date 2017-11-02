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

set(srcs
  main.cpp
  Objects/Triangle.cpp
  Objects/Vertex.cpp
  IO/ReadData.cpp
  Rendering/Renderer_Example.cpp
  Rendering/OpenGL_Example.cpp
  Rendering/OpenGL_common/shader.cpp
  Composition/Composition_Example.cpp
  #Composition/IceT_Example.cpp
  )

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(GLFW REQUIRED)

add_executable(run ${srcs})
target_include_directories(run PRIVATE
  ${OPENGL_INCLUDE_DIR}
  ${GLEW_INCLUDE_DIRS}
  ${GLFW_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glm/include
  )
target_link_libraries(run PRIVATE
  ${OPENGL_LIBRARIES}
  ${GLEW_LIBRARIES}
  ${GLFW_LIBRARIES}
  )
target_compile_features(run PRIVATE
  cxx_raw_string_literals
  )
