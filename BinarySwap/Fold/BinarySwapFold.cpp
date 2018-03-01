// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "BinarySwapFold.hpp"
#include "../Base/BinarySwapBase.hpp"

static int getLargestPowerOfTwoNoBiggerThan(int x) {
  int power2 = 1;
  while (power2 <= x) {
    power2 *= 2;
  }
  return power2 / 2;
}

static int getRealRank(MPI_Group group, int rank, MPI_Comm communicator) {
  MPI_Group commGroup;
  MPI_Comm_group(communicator, &commGroup);

  int realRank;
  MPI_Group_translate_ranks(group, 1, &rank, commGroup, &realRank);
  return realRank;
}

std::unique_ptr<Image> BinarySwapFold::compose(Image *localImage,
                                               MPI_Group group,
                                               MPI_Comm communicator) {
  // The base binary-swap algorithm only operates on process groups with a size
  // of a power-of-two. This compositing algorithm first shrinks the given
  // process group to one that is a power-of-two. It does this by taking an
  // excess processes, folding their image into another process, and then going
  // idle the rest of the time.

  int myGroupRank;
  MPI_Group_rank(group, &myGroupRank);

  int originalGroupSize;
  MPI_Group_size(group, &originalGroupSize);

  int targetGroupSize = getLargestPowerOfTwoNoBiggerThan(originalGroupSize);

  int numProcsToRemove = originalGroupSize - targetGroupSize;
  // The number of processes to remove can never be more than the target group
  // size (because math).
  assert(numProcsToRemove < targetGroupSize);

  // Special case: we already have a power of two. Just call the base
  // binary-swap and return.
  if (numProcsToRemove == 0) {
    return BinarySwapBase().compose(localImage, group, communicator);
  }

  std::vector<int> procsToRemove(numProcsToRemove);

  std::unique_ptr<Image> workingImage = localImage->shallowCopy();

  // We have to transfer the images from numProcsToRemove processes to another
  // process and blend them there. We have to match up adjacent processes so
  // that order-dependent blending will be correct. Do that by alternating
  // processes to pick and processes to remove.
  for (int i = 0; i < numProcsToRemove; ++i) {
    int rankToRecv = 2 * i;
    int rankToSend = 2 * i + 1;
    procsToRemove[i] = rankToSend;
    if (myGroupRank == rankToRecv) {
      // This process absorbs an image from another process.
      std::unique_ptr<Image> incomingImage = workingImage->createNew();
      incomingImage->Receive(getRealRank(group, rankToSend, communicator),
                             communicator);
      workingImage = workingImage->blend(*incomingImage);
    } else if (myGroupRank == rankToSend) {
      // This process sends its image out and drops out of the composition by
      // returning an empty image.
      workingImage->Send(getRealRank(group, rankToRecv, communicator),
                         communicator);
      return workingImage->copySubrange(0, 0);
    }
  }

  // Create a new group with the folded processes removed.
  MPI_Group subGroup;
  MPI_Group_excl(group, numProcsToRemove, &procsToRemove.front(), &subGroup);

  // Now call the base binary-swap algorithm.
  std::unique_ptr<Image> resultImage =
      BinarySwapBase().compose(workingImage.get(), subGroup, communicator);

  // Cleanup
  MPI_Group_free(&subGroup);

  return resultImage;
}
