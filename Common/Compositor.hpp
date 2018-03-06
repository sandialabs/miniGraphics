// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include <miniGraphicsConfig.h>

#include <Common/Image.hpp>
#include <Common/YamlWriter.hpp>

#include <optionparser.h>

#include <mpi.h>

class Compositor {
 public:
  /// Subclasses need to implement this function. It takes images of the local
  /// partition of the data and combines them into a single image. The
  /// composite algorithm should use the given MPI group, which is defined on
  /// the given MPI communicator. The group may be a subset of the
  /// communicator, in which case the \c compose method is only called on those
  /// processes that are part of the group. The group may also have its ranks
  /// reordered. When blending the images together, the image on the first
  /// process of the group is on top with subsequent processes of the group
  /// underneath the ones before.
  ///
  /// Typically a compositing algorithm will split up an image. Each process
  /// should return an Image object containing the fully composited pixels of a
  /// distinct subregion.
  ///
  virtual std::unique_ptr<Image> compose(Image *localImage,
                                         MPI_Group group,
                                         MPI_Comm communicator,
                                         YamlWriter &yaml) = 0;

  /// If a compositor can be controled by some custom command line arguments,
  /// it should override this method to get the options and set up the state.
  /// It shouldl also use the given YamlWriter to record the options used (even
  /// if default values are used).
  ///
  /// This method should return true if all the options are OK, false
  /// otherwise. If false is returned, then a usage statement will be outputted
  /// to standard error and the program will halt. If the method returns false,
  /// it should print a descriptive reason why the error occured to standard
  /// error before returning.
  ///
  virtual bool setOptions(const std::vector<option::Option> &options,
                          YamlWriter &yaml);

  virtual ~Compositor() = default;
};

#endif  // COMPOSITOR_H
