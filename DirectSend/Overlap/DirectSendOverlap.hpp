// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef DIRECTSENDOVERLAP_HPP
#define DIRECTSENDOVERLAP_HPP

#include <Common/Compositor.hpp>

class DirectSendOverlap : public Compositor {
  int maxSplit;

 public:
  DirectSendOverlap();

  std::unique_ptr<Image> compose(Image *localImage,
                                 MPI_Group group,
                                 MPI_Comm communicator,
                                 YamlWriter &yaml) final;

  /// Performs the direct-send compositing by sending a piece of the image from
  /// every process in sendGroup to each process in recvGroup. The end result
  /// will be a composited piece in each member of recvGroup.
  ///
  /// sendGroup and recvGroup can contain common processes. In fact, it is
  /// common for the two groups to be the same and even more common for every
  /// process in recvGroup to be in sendGroup (although that is not necessary).
  /// Any process in sendGroup that is not in recvGroup will return an image
  /// with an empty range.
  ///
  static std::unique_ptr<Image> compose(Image *localImage,
                                        MPI_Group sendGroup,
                                        MPI_Group recvGroup,
                                        MPI_Comm communicator,
                                        YamlWriter &yaml);

  bool setOptions(const std::vector<option::Option> &options,
                  YamlWriter &yaml) override;
  static std::vector<option::Descriptor> getOptionVector();
};

#endif  // DIRECTSENDOVERLAP_HPP
