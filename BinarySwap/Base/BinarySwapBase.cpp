// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "BinarySwapBase.hpp"

enum PairRole { PAIR_ROLE_EVEN, PAIR_ROLE_ODD };

static bool isPowerOfTwo(int x) {
  while (x > 1) {
    if ((x % 2) != 0) {
      return false;
    }
    x /= 2;
  }
  return true;
}

static int getRealRank(MPI_Group group, int rank, MPI_Comm communicator) {
  MPI_Group commGroup;
  MPI_Comm_group(communicator, &commGroup);

  int realRank;
  MPI_Group_translate_ranks(group, 1, &rank, commGroup, &realRank);
  return realRank;
}

std::unique_ptr<Image> BinarySwap::compose(Image *localImage,
                                           MPI_Group group,
                                           MPI_Comm communicator) {
  // Binary-swap is a recursive algorithm. We start with a process group with
  // all the processes, then divide and conquer the group until we only have
  // groups of size 1.
  MPI_Group workingGroup;
  MPI_Group_excl(group, 0, nullptr, &workingGroup);  // Copies group

  int rank;
  MPI_Group_rank(workingGroup, &rank);

  int numProc;
  MPI_Group_size(workingGroup, &numProc);

  std::unique_ptr<Image> workingImage = localImage->shallowCopy();

  // This version of binary swap only works if the communicator size is a power
  // of two.
  if (!isPowerOfTwo(numProc)) {
    std::cerr << "Binary-swap only works with powers-of-two processors"
              << std::endl;
    exit(1);
  }

  while (numProc > 1) {
    // At each iteration of the binary-swap algorithm, divide the image in half.
    std::unique_ptr<Image> topHalf =
        workingImage->copySubrange(0, workingImage->getNumberOfPixels() / 2);
    std::unique_ptr<Image> bottomHalf =
        workingImage->copySubrange(workingImage->getNumberOfPixels() / 2,
                                   workingImage->getNumberOfPixels());

    std::unique_ptr<Image> toKeep;
    std::unique_ptr<Image> toSend;

    int partnerRank;

    // At each iteration of the binary-swap algorithm, each process pairs with
    // one other process. Because we want to have a correct front-to-back
    // ordering of images (and the mini-app arranges the communicator ranks
    // reflect the order of the images), we pair with a process adjacent to
    // ours. We use whether the rank is even or odd to determine which member
    // of the pair we are.
    PairRole role;
    if (rank % 2 == 0) {
      // The "even" role has the smaller rank. It has the image that goes on
      // top, and we will collect the first half of the image.
      role = PAIR_ROLE_EVEN;
      toKeep.swap(topHalf);
      toSend.swap(bottomHalf);
      partnerRank = rank + 1;
    } else {
      // The "odd" role has the larger rank. It has the image that goes on
      // the bottom, and we will collect the second half of the image.
      role = PAIR_ROLE_ODD;
      toKeep.swap(bottomHalf);
      toSend.swap(topHalf);
      partnerRank = rank - 1;
    }

    // Receive our half of the image and send out our partner's half.
    std::unique_ptr<Image> recvImage = toKeep->createNew();
    std::vector<MPI_Request> recvRequests = recvImage->IReceive(
        getRealRank(workingGroup, partnerRank, communicator), communicator);
    std::vector<MPI_Request> sendRequests = toSend->ISend(
        getRealRank(workingGroup, partnerRank, communicator), communicator);

    // Wait for my image to come in.
    std::vector<MPI_Status> statuses(recvRequests.size());
    MPI_Waitall(recvRequests.size(), &recvRequests.front(), &statuses.front());

    // Blend the incoming image and set the workingImage to the result.
    switch (role) {
      case PAIR_ROLE_EVEN:
        workingImage = toKeep->blend(recvImage.get());
        break;
      case PAIR_ROLE_ODD:
        workingImage = recvImage->blend(toKeep.get());
        break;
    }

    // Wait for my images to finish sending.
    statuses.resize(sendRequests.size());
    MPI_Waitall(sendRequests.size(), &sendRequests.front(), &statuses.front());

    // Create a sub-communicator containing all the processes with same portion
    // of the image as me.
    int rankRange[1][3];
    rankRange[0][0] = (role == PAIR_ROLE_EVEN) ? 0 : 1;
    rankRange[0][1] = numProc - 1;
    rankRange[0][2] = 2;

    MPI_Group subGroup;
    MPI_Group_range_incl(workingGroup, 1, rankRange, &subGroup);

    // Decend into the sub-communicator and repeat.
    MPI_Group_free(&workingGroup);
    workingGroup = subGroup;
    MPI_Group_rank(workingGroup, &rank);
    MPI_Group_size(workingGroup, &numProc);
  }

  // Clean up internal objects and return image.
  MPI_Group_free(&workingGroup);

  return workingImage;
}
