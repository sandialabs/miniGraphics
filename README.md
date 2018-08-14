# miniGraphics #

The miniGraphics miniapp demonstrates parallel rendering in an MPI
environment using a sort-last parallel rendering approach. The main
intention of miniGraphics is to demonstrate and compare the image
compositing step where multiple images are combined to a single image
(essentially an optimized reduce operation); however, miniGraphics also has
multiple rendering backends that can be used in conjunction with the
compositing algorithms.

The miniGraphics miniapp is actually divided into many small applications
that support the same parallel rendering operations using different
sort-last parallel rendering algorithms. These applications can be compiled
either independently or en masse (see [compiling](#compiling) below). It is
also simple to add new applications if you wish to contribute your own
image compositing algorithms.

## Compiling ##

The following are the minimum requirements for building miniGraphics:

  * CMake version 3.3 or better
  * A C++ compiler (C++11 compliant)
  * MPI

To compile all miniGraphics applications at once, simply run CMake for the
base directory of miniGraphics (the directory containing this file). The
basic steps for compiling with CMake are (1) create a build directory,
(2) run cmake in that directory, and (3) run the build program for the
project files generated (usually make unless otherwise specified). The
following are typical commands although they can vary between systems.

    mkdir miniGraphics-build
    cd miniGraphics-build
    cmake ../miniGraphics
    make -j

It is also possible to independently compile each miniGraphics application.
This is done by following the same steps but for the subdirectory
containing the specific application in question. (See the
[directories](#directories) section below for a reference on what miniapps
are implemented and where they are located.)

## Directories ##

Parallel sort-last rendering algorithms are (mostly) differentiated by
their image compositing algorithm. The miniGraphics miniapps are divided by
image compositing algorithms and comes with several implementations. The
directories in the following list each contain implementation of a
particular compositing algorithm. Within each directory is subdirectories
of various perturbations of the algorithm.

  * **[BinarySwap](BinarySwap/README.md)** A basic but effective recursive
    algorithm in which at each iteration the image is divided and swapped.

Note that there is a symbolic link named **Reference** that points to a
reference implementation of parallel rendering.

In addition to the basic compositing algorithm, there are also some
supporting directories containing code that is used by all or some of the
miniGraphics miniapps.

  * **Paint** Contains the drawing algorithms used in the parallel
    rendering. sort-last parallel rendering happens in 2 phases: a local
    geometry rendering (what we call here a "paint" to prevent name
    overloading) and a parallel image compositing. The painting algorithms
    are located in this directory (and contributed painting algorithms
    should also go here).
  * **Common** A collection of common objects used by miniGraphics
    miniapps. Examples include image objects, mesh objects, and boilerplate
    main loop code.
  * **CMake** Contains auxiliary CMake scripts used for building.
  * **ThirdParty** Contains code imported from third party sources that are
    used by miniGraphics.

## License ##

miniGraphics is distributed under the OSI-approved BSD 3-clause License.
See [LICENSE.txt]() for details.

Copyright (c) 2017
National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
certain rights in this software.

SDR# 2261
