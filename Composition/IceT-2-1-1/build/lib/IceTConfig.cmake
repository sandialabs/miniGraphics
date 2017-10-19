#-----------------------------------------------------------------------------
#
# ICETConfig.cmake - IceT CMake configuration file for external projects.
#
# This file is configured by IceT and used by other CMake projects to load
# IceT's settings.
#
# Copyright 2003 Sandia Coporation
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
#
# This source code is released under the New BSD License.
#

# This file should be installed in the lib directory.  Find the root directory.
GET_FILENAME_COMPONENT(_dir "${CMAKE_CURRENT_LIST_FILE}" PATH)
GET_FILENAME_COMPONENT(_install_dir "${_dir}/.." ABSOLUTE)

# Load the targets include (next to this one).
INCLUDE("${_dir}/IceTTargets.cmake")

# The IceT include file directories.
SET(ICET_INCLUDE_DIRS "/home/mgpeter/Documents/XViz/miniGraphics/Composition/IceT-2-1-1/src/include;/home/mgpeter/Documents/XViz/miniGraphics/Composition/IceT-2-1-1/build/src/include")

# The IceT version number
SET(ICET_MAJOR_VERSION "2")
SET(ICET_MINOR_VERSION "1")
SET(ICET_PATCH_VERSION "1")
SET(ICET_VERSION "2.1.1")

# Main IceT configuration options
SET(ICET_USE_OPENGL "ON")
SET(ICET_USE_MPI "ON")
SET(ICET_BUILD_SHARED_LIBS "OFF")

# The IceT libraries
SET(ICET_CORE_LIBS "IceTCore")
SET(ICET_GL_LIBS "IceTGL")
SET(ICET_MPI_LIBS "IceTMPI")

# MPI configuration used to build IceT.
SET(ICET_MPI_INCLUDE_PATH "/usr/include/mpich")
SET(ICET_MPI_LIBRARY "/usr/lib/x86_64-linux-gnu/libmpich.so")
SET(ICET_MPI_EXTRA_LIBRARY "/usr/lib/x86_64-linux-gnu/libopa.so;/usr/lib/x86_64-linux-gnu/libmpl.so;/usr/lib/x86_64-linux-gnu/librt.so;/usr/lib/libcr.so;/usr/lib/x86_64-linux-gnu/libpthread.so")
SET(ICET_MPIRUN_EXE "/usr/bin/mpiexec")
SET(ICET_MPI_MAX_NUMPROCS "2")
SET(ICET_MPI_PREFLAGS "")
SET(ICET_MPI_POSTFLAGS "")

