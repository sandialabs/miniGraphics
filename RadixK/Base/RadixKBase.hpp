// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef RADIXKBASE_HPP
#define RADIXKBASE_HPP

#include <Common/Compositor.hpp>

class RadixKBase : public Compositor {
  std::vector<int> kVector;

 public:
  std::unique_ptr<Image> compose(Image *localImage,
                                 MPI_Group group,
                                 MPI_Comm communicator,
                                 YamlWriter &yaml) final;

  void generateK(int targetK, int numProc);

  bool setOptions(const std::vector<option::Option> &options,
                  MPI_Comm communicator,
                  YamlWriter &yaml) override;
  static std::vector<option::Descriptor> getOptionVector();
};

#endif  // RADIXKBASE_HPP
