// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef SWAP_2_3_BASE_HPP
#define SWAP_2_3_BASE_HPP

#include <Common/Compositor.hpp>

class Swap_2_3_Base : public Compositor {
 public:
  std::unique_ptr<Image> compose(Image *localImage,
                                 MPI_Group group,
                                 MPI_Comm communicator,
                                 YamlWriter &yaml) final;
};

#endif  // SWAP_2_3_BASE_HPP
