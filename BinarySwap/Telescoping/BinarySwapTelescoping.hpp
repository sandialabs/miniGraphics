// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef BINARYSWAPFOLD_HPP
#define BINARYSWAPFOLD_HPP

#include <Common/Compositor.hpp>

class BinarySwapTelescoping : public Compositor {
 private:
  std::unique_ptr<Image> composeBigGroup(Image *localImage,
                                         MPI_Group bigGroup,
                                         MPI_Group littleGroup,
                                         MPI_Comm communicator);
  void composeLittleGroup(Image *localImage,
                          MPI_Group bigGroup,
                          MPI_Group littleGroup,
                          MPI_Comm communicator);

 public:
  std::unique_ptr<Image> compose(Image *localImage,
                                 MPI_Group group,
                                 MPI_Comm communicator) final;
};

#endif  // BINARYSWAPFOLD_HPP