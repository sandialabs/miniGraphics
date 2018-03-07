// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "DirectSendBase.hpp"

static int getRealRank(MPI_Group group, int rank, MPI_Comm communicator) {
  MPI_Group commGroup;
  MPI_Comm_group(communicator, &commGroup);

  int realRank;
  MPI_Group_translate_ranks(group, 1, &rank, commGroup, &realRank);
  return realRank;
}

std::unique_ptr<Image> DirectSendBase::compose(Image* localImage,
                                               MPI_Group group,
                                               MPI_Comm communicator,
                                               YamlWriter&) {
  assert(0 && "Not implemented");
  return localImage->shallowCopy();
}
