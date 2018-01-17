# Binary Swap #

This directory contains the binary swap algorithms of miniGraphics. The
binary swap algorithm is a divide-and-conquer algorithm that takes place in
iterations. At each iteration each process divides its image in two. It
sends half of its image to a paired processor and receives the other half
from its pair. Each process then blends its incoming image with its
contained image. The processes are then grouped by those that have the same
image part, and the algorithm recurses in parallel.

The following subdirectories contain variations of the binary swap
algorithm.

  * **Base** The base version of the binary swap algorithm. This version only
    supports process counts that are a power of 2.

See [the root README.md file](../README.md) for more information about
miniGraphics and compiling it.

## License ##

miniGraphics is distributed under the OSI-approved BSD 3-clause License.
See [LICENSE.txt]() for details.

Copyright (c) 2017
National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
certain rights in this software.
