// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "BinarySwapRemainder.hpp"

enum PairRole { PAIR_ROLE_EVEN, PAIR_ROLE_ODD };

static int getRealRank(MPI_Group group, int rank, MPI_Comm communicator) {
  MPI_Group commGroup;
  MPI_Comm_group(communicator, &commGroup);

  int realRank;
  MPI_Group_translate_ranks(group, 1, &rank, commGroup, &realRank);

  MPI_Group_free(&commGroup);
  return realRank;
}

std::unique_ptr<Image> BinarySwapRemainder::compose(Image *localImage,
                                                    MPI_Group group,
                                                    MPI_Comm communicator,
                                                    YamlWriter &) {
  // Binary-swap-remainder is a recursive algorithm that operates very similar
  // to the base algorithm. (You should understand the base algorithm before
  // reading this one.) The only difference is that at an iteration if the
  // group cannot be divided in two even parts, the remaining process transfers
  // all its data to members of a group.
  MPI_Group workingGroup;
  int dummy;
  MPI_Group_excl(group, 0, &dummy, &workingGroup);  // Copies group

  int rank;
  MPI_Group_rank(workingGroup, &rank);

  int numProc;
  MPI_Group_size(workingGroup, &numProc);

  std::unique_ptr<Image> workingImage = localImage->shallowCopy();

  while (numProc > 1) {
    // At each iteration of the binary-swap algorithm, divide the image in half.
    std::unique_ptr<const Image> topHalf =
        workingImage->window(0, workingImage->getNumberOfPixels() / 2);
    std::unique_ptr<const Image> bottomHalf =
        workingImage->window(workingImage->getNumberOfPixels() / 2,
                             workingImage->getNumberOfPixels());

    bool haveRemainder = ((numProc % 2) == 1);
    if (haveRemainder && (rank == (numProc - 1))) {
      // My process is in the remainder. Offload my images to processes in
      // another group and drop out of the composition.
      std::vector<MPI_Request> topSendRequests = topHalf->ISend(
          getRealRank(workingGroup, numProc - 3, communicator), communicator);
      std::vector<MPI_Request> bottomSendRequests = bottomHalf->ISend(
          getRealRank(workingGroup, numProc - 2, communicator), communicator);

      MPI_Waitall(
          topSendRequests.size(), topSendRequests.data(), MPI_STATUSES_IGNORE);
      MPI_Waitall(bottomSendRequests.size(),
                  bottomSendRequests.data(),
                  MPI_STATUSES_IGNORE);

      workingImage = workingImage->copySubrange(0, 0);
      break;
    }

    std::unique_ptr<const Image> toKeep;
    std::unique_ptr<const Image> toSend;

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
    MPI_Waitall(recvRequests.size(), recvRequests.data(), MPI_STATUSES_IGNORE);

    // Blend the incoming image and set the workingImage to the result.
    switch (role) {
      case PAIR_ROLE_EVEN:
        workingImage = toKeep->blend(*recvImage);
        break;
      case PAIR_ROLE_ODD:
        workingImage = recvImage->blend(*toKeep);
        break;
    }

    // Receive any images from the remainder if necessary
    if (haveRemainder && (rank >= numProc - 3)) {
      recvImage->Receive(getRealRank(workingGroup, numProc - 1, communicator),
                         communicator);
      workingImage = workingImage->blend(*recvImage);
    }

    // Wait for my images to finish sending.
    MPI_Waitall(sendRequests.size(), sendRequests.data(), MPI_STATUSES_IGNORE);

    // Create a sub-communicator containing all the processes with same portion
    // of the image as me.
    int rankRange[1][3];
    rankRange[0][0] = (role == PAIR_ROLE_EVEN) ? 0 : 1;
    rankRange[0][1] = numProc - (haveRemainder ? 2 : 1);
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
