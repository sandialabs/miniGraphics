cmake_minimum_required(VERSION 3.3)

project(miniGraphics)

set(miniGraphics_CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${miniGraphics_CMAKE_MODULE_PATH})

add_executable(createData dataGen/createData.cpp)

set(srcs
  main.cpp
  Objects/Triangle.cpp
  IO/ReadData.cpp
  Rendering/Renderer_Example.cpp
  Rendering/OpenGL_Example.cpp
  Rendering/OpenGL_common/shader.cpp
  Composition/Composition_Example.cpp
  #Composition/IceT_Example.cpp
  )

find_package(GLEW REQUIRED)
find_package(GLFW REQUIRED)

add_executable(run ${srcs})
target_include_directories(run
  PRIVATE ${GLEW_INCLUDE_DIRS} ${GLFW_INCLUDE_DIRS})
target_link_libraries(run
  PRIVATE ${GLEW_LIBRARIES} ${GLFW_LIBRARIES})
