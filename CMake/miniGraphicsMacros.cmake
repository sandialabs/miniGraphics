## miniGraphics is distributed under the OSI-approved BSD 3-clause License.
## See LICENSE.txt for details.
##
## Copyright (c) 2017
## National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
## the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
## certain rights in this software.

cmake_minimum_required(VERSION 3.3)

include(CMakeParseArguments)

# Set up this directory in the CMAKE MODULE PATH
set(miniGraphics_CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
set(CMAKE_MODULE_PATH ${CMAK$E_MODULE_PATH} ${miniGraphics_CMAKE_MODULE_PATH})

# Set up the binary output paths
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib CACHE PATH
  "Output directory for building all libraries."
  )
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE PATH
  "Output directory for building all executables."
  )

# Get the base miniGraphics source dir
get_filename_component(miniGraphics_SOURCE_DIR
  ${CMAKE_CURRENT_LIST_DIR}
  DIRECTORY
  )

find_package(MPI REQUIRED)

# Create the config header file
function(miniGraphics_create_config_header miniapp_name)
  set(MINIGRAPHICS_APP_NAME ${miniapp_name})
  set(MINIGRAPHICS_WIN32 ${WIN32})
  configure_file(${miniGraphics_CMAKE_MODULE_PATH}/miniGraphicsConfig.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/miniGraphicsConfig.h
    )
endfunction(miniGraphics_create_config_header)

# Adds compile features to a given miniGraphics target.
function(miniGraphics_target_features target_name)
  set(include_dirs
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${miniGraphics_SOURCE_DIR}
    ${miniGraphics_SOURCE_DIR}/ThirdParty/glm/include
    ${miniGraphics_SOURCE_DIR}/ThirdParty/optionparser/include
    ${MPI_CXX_INCLUDE_PATH}
    )

  set(libs
    ${MPI_CXX_LINK_FLAGS}
    ${MPI_CXX_LIBRARIES}
    )

  set(cxx_flags
    ${MPI_CXX_COMPILE_FLAGS}
    )

  target_include_directories(${target_name} PRIVATE ${include_dirs})

  target_link_libraries(${target_name} PRIVATE ${libs})

  target_compile_options(${target_name} PRIVATE ${cxx_flags})

  target_compile_features(${target_name} PRIVATE
    cxx_raw_string_literals
    )

  if(WIN32)
    # Don't deal with MSVC's unportable safety warnings
    target_compile_definitions(${target_name}
      PUBLIC -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS
      )
  endif()
endfunction(miniGraphics_target_features)

# Find the largest power of two less than or equal to the given value.
function(miniGraphics_find_power_of_two var value)
  set(power2 1)
  while(power2 LESS value)
    math(EXPR power2 "${power2} * 2")
  endwhile()
  if (power2 GREATER value)
    math(EXPR power2 "${power2} / 2")
  endif()
  set(${var} ${power2} PARENT_SCOPE)
endfunction(miniGraphics_find_power_of_two)

# Call this function to build one of the miniGraphics miniapps.
# The first argument is the name of the miniapp. A target with that name will
# be created. The remaining arguments are source files.
function(miniGraphics_executable miniapp_name)
  set(options DISABLE_TESTS POWER_OF_TWO_ONLY)
  set(oneValueArgs)
  set(multiValueArgs HEADERS SOURCES)
  cmake_parse_arguments(miniGraphics_executable
    "${options}" "${oneValueArgs}" "${multiValueArgs}"
    ${ARGN}
    )

  set(srcs
    ${miniGraphics_executable_SOURCES}
    ${miniGraphics_executable_UNPARSED_ARGUMENTS}
    )

  set(headers
    ${CMAKE_CURRENT_BINARY_DIR}/miniGraphicsConfig.h
    ${miniGraphics_executable_HEADERS}
    )

  miniGraphics_create_config_header(${miniapp_name})

  add_executable(${miniapp_name} ${srcs} ${headers})

  miniGraphics_target_features(${miniapp_name})

  target_link_libraries(${miniapp_name}
    PRIVATE miniGraphicsCommon miniGraphicsPaint)

  set_source_files_properties(${headers} HEADER_ONLY TRUE)

  if(MINIGRAPHICS_ENABLE_TESTING AND NOT miniGraphics_executable_DISABLE_TESTS)
    set(base_options
      --width=100 --height=100
      --trials=1
      --yaml-output=test-runs.yaml
      )
    if(miniGraphics_executable_POWER_OF_TWO_ONLY)
      miniGraphics_find_power_of_two(np ${MPIEXEC_MAX_NUMPROCS})
    else()
      set(np ${MPIEXEC_MAX_NUMPROCS})
    endif()
    foreach(color_buffer_option --color-ubyte --color-float)
      foreach(depth_buffer_option --depth-float --depth-none)
        foreach(image_compress_option --disable-image-compress --enable-image-compress)
          set(test_name ${miniapp_name}${color_buffer_option}${depth_buffer_option}${image_compress_option})
          set(test_options
            ${base_options}
            ${color_buffer_option}
            ${depth_buffer_option}
            ${image_compress_option}
            )
          add_test(
            NAME ${test_name}
            COMMAND ${MPIEXEC}
              ${MPIEXEC_NUMPROC_FLAG} ${np}
              ${MPIEXEC_PREFLAGS}
              $<TARGET_FILE:${miniapp_name}>
              ${MPIEXEC_POSTFLAGS}
              ${test_options}
            )
        endforeach(image_compress_option)
      endforeach(depth_buffer_option)
    endforeach(color_buffer_option)
  endif()
endfunction(miniGraphics_executable)

if(NOT TARGET miniGraphicsCommon)
  add_subdirectory(${miniGraphics_SOURCE_DIR}/Common ${CMAKE_BINARY_DIR}/Common)
endif()

if(NOT TARGET miniGraphicsPaint)
  add_subdirectory(${miniGraphics_SOURCE_DIR}/Paint ${CMAKE_BINARY_DIR}/Paint)
endif()
