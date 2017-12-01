// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include "Image.hpp"

#include <mpi.h>

class Compositor {
 public:
  /// Subclasses need to implement this function. It takes an image of the
  /// local partition of the data and combines them into a single image. Use
  /// the given MPI communicator. When blending the images together, the image
  /// on node 0 is on top with subsequent ranks underneath the ones before.
  ///
  /// Typically a compositing algorithm will split up an image. Each process
  /// should return an Image object containing  the fully composited pixels
  /// of a distinct subregion.
  ///
  virtual std::unique_ptr<Image> compose(Image *localImage,
                                         MPI_Comm communicator) = 0;

  virtual ~Compositor() = default;
};

#endif  // COMPOSITOR_H
