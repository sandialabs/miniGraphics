// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2018
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "BinarySwap234Schedule.hpp"
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

  MPI_Group_free(&commGroup);
  return realRank;
}

// Performs a typical binary-swap step
static std::unique_ptr<Image> swapHalves(Image *localImage,
                                         MPI_Group group,
                                         MPI_Comm communicator,
                                         int subgroupStart) {
  int rank;
  MPI_Group_rank(group, &rank);

  // Divide the image into halves.
  std::unique_ptr<const Image> firstHalf =
      localImage->window(0, localImage->getNumberOfPixels() / 2);
  std::unique_ptr<const Image> secondHalf = localImage->window(
      localImage->getNumberOfPixels() / 2, localImage->getNumberOfPixels());

  std::unique_ptr<const Image> toKeep;
  std::unique_ptr<const Image> toSend;

  int partnerRank;

  // Determine which half of the image is being received
  enum struct PairRole { FIRST, SECOND };
  PairRole role;
  if (rank == subgroupStart) {
    role = PairRole::FIRST;
    toKeep.swap(firstHalf);
    toSend.swap(secondHalf);
    partnerRank = rank + 1;
  } else if (rank == subgroupStart + 1) {
    role = PairRole::SECOND;
    toKeep.swap(secondHalf);
    toSend.swap(firstHalf);
    partnerRank = rank - 1;
  } else {
    std::cerr << "Binary Swap - 234 Schedule - swapHalves "
              << "called with invalid subgroupStart" << std::endl;
    exit(1);
  }

  // Receive our half of the image and send out our partner's half.
  std::unique_ptr<Image> recvImage = toKeep->createNew();
  std::vector<MPI_Request> recvRequests = recvImage->IReceive(
      getRealRank(group, partnerRank, communicator), communicator);
  std::vector<MPI_Request> sendRequests = toSend->ISend(
      getRealRank(group, partnerRank, communicator), communicator);

  // Wait for my image to come in.
  MPI_Waitall(recvRequests.size(), recvRequests.data(), MPI_STATUSES_IGNORE);

  // Blend the incoming image.
  std::unique_ptr<Image> blendedImage;
  switch (role) {
    case PairRole::FIRST:
      blendedImage = toKeep->blend(*recvImage);
      break;
    case PairRole::SECOND:
      blendedImage = recvImage->blend(*toKeep);
      break;
  }

  // Wait for my images to finish sending.
  MPI_Waitall(sendRequests.size(), sendRequests.data(), MPI_STATUSES_IGNORE);

  // Return result
  return blendedImage;
}

// Takes a group of 3 processes, divides their images in half, and composites
// them such that the first two processes have the first and second pieces,
// respectively, and the 3 is empty.
static std::unique_ptr<Image> Eliminate32(Image *localImage,
                                          MPI_Group group,
                                          MPI_Comm communicator,
                                          int subgroupStart) {
  int rank;
  MPI_Group_rank(group, &rank);

  if ((rank == subgroupStart) || (rank == subgroupStart + 1)) {
    // This rank will hold one of the two image halves.
    // First, get the receive ready for the half sent by the third process.
    std::unique_ptr<Image> recvImage = localImage->createNew();
    std::vector<MPI_Request> recvRequests = recvImage->IReceive(
        getRealRank(group, subgroupStart + 2, communicator), communicator);

    // Next, do a normal binary swap between the first two processes.
    std::unique_ptr<Image> blendedImage =
        swapHalves(localImage, group, communicator, subgroupStart);

    // Wait for the incoming image to finish.
    MPI_Waitall(recvRequests.size(), recvRequests.data(), MPI_STATUSES_IGNORE);

    // Finally, blend the third process' image.
    return blendedImage->blend(*recvImage);
  } else if (rank == subgroupStart + 2) {
    // This rank gives away its two halves.

    // Divide the image into halves.
    std::unique_ptr<const Image> firstHalf =
        localImage->window(0, localImage->getNumberOfPixels() / 2);
    std::unique_ptr<const Image> secondHalf = localImage->window(
        localImage->getNumberOfPixels() / 2, localImage->getNumberOfPixels());
    // Send the images out.
    std::vector<MPI_Request> firstSendRequests = firstHalf->ISend(
        getRealRank(group, subgroupStart, communicator), communicator);
    std::vector<MPI_Request> secondSendRequests = secondHalf->ISend(
        getRealRank(group, subgroupStart + 1, communicator), communicator);

    // Wait for the messages to finish.
    MPI_Waitall(firstSendRequests.size(),
                firstSendRequests.data(),
                MPI_STATUSES_IGNORE);
    MPI_Waitall(secondSendRequests.size(),
                secondSendRequests.data(),
                MPI_STATUSES_IGNORE);

    // Return empty image
    return localImage->copySubrange(0, 0);
  } else {
    std::cerr << "Binary Swap - 234 Schedule - Eliminate32 "
              << "called with invalid subgroupStart" << std::endl;
    exit(1);
  }
}

// Takes a group of 4 processes, does a standard swap on two pairs, and then
// blends the data from the second group to the first.
static std::unique_ptr<Image> Eliminate42(Image *localImage,
                                          MPI_Group group,
                                          MPI_Comm communicator,
                                          int subgroupStart) {
  int rank;
  MPI_Group_rank(group, &rank);

  if ((rank == subgroupStart) || (rank == subgroupStart + 1)) {
    // This rank is part of the first group.
    // First, get the receive ready for the half sent by the second group.
    std::unique_ptr<Image> recvImage = localImage->createNew();
    std::vector<MPI_Request> recvRequests = recvImage->IReceive(
        getRealRank(group, rank + 2, communicator), communicator);

    // Next, do a normal binary swap.
    std::unique_ptr<Image> blendedImage =
        swapHalves(localImage, group, communicator, subgroupStart);

    // Wait for the incoming image to finish.
    MPI_Waitall(recvRequests.size(), recvRequests.data(), MPI_STATUSES_IGNORE);

    // Finally, blend the other group's image.
    return blendedImage->blend(*recvImage);
  } else if ((rank == subgroupStart + 2) || (rank == subgroupStart + 3)) {
    // This rank is part of the second group.
    // First, do a normal binary swap.
    std::unique_ptr<Image> blendedImage =
        swapHalves(localImage, group, communicator, subgroupStart + 2);

    // Next, send the image to the first group.
    blendedImage->Send(getRealRank(group, rank - 2, communicator),
                       communicator);

    // Return empty image
    return localImage->copySubrange(0, 0);
  } else {
    std::cerr << "Binary Swap - 234 Schedule - Eliminate42 "
              << "called with invalid subgroupStart" << std::endl;
    exit(1);
  }
}

std::unique_ptr<Image> BinarySwap234Schedule::compose(Image *localImage,
                                                      MPI_Group group,
                                                      MPI_Comm communicator,
                                                      YamlWriter &yaml) {
  // The 234composite algorithm (or binary swap with 234 scheduling) spends its
  // first iteration reducing the number of processes to a power of two. It
  // does this with a combination of 3-2 and 4-2 elimination where 3 or 4
  // processes are reduced to 2.

  int rank;
  MPI_Group_rank(group, &rank);

  int numProc;
  MPI_Group_size(group, &numProc);

  int targetP2 = getLargestPowerOfTwoNoBiggerThan(numProc);

  if (numProc == targetP2) {
    // Case 0, started with a power of 2 number of processes.
    return BinarySwapBase().compose(localImage, group, communicator, yaml);
  } else if (numProc < targetP2 + (targetP2 / 2)) {
    // Case 1, use 3-2 elimination with standard swaps
    // Each 3-2 elimination takes one process out of the composite. Thus, we
    // need this many.
    int num32Eliminations = numProc - targetP2;
    int firstSwapGroup = num32Eliminations * 3;

    // Do the prescribed swap or elemination.
    enum struct PairRole { FIRST, SECOND };
    PairRole role;
    std::unique_ptr<Image> workingImage;
    if (rank < firstSwapGroup) {
      // Case 1.a: This rank in a 3-2 elimination
      int subgroupStart = (rank / 3) * 3;
      workingImage =
          Eliminate32(localImage, group, communicator, subgroupStart);
      assert(rank >= subgroupStart);
      assert((rank - subgroupStart) < 3);
      switch (rank - subgroupStart) {
        case 0:
          role = PairRole::FIRST;
          break;
        case 1:
          role = PairRole::SECOND;
          break;
        default:
          return workingImage;
      }
    } else {
      // Case 1.b: This rank in a standard swap
      int subgroupStart = (((rank - firstSwapGroup) / 2) * 2) + firstSwapGroup;
      workingImage = swapHalves(localImage, group, communicator, subgroupStart);
      assert(rank >= subgroupStart);
      assert((rank - subgroupStart) < 2);
      switch (rank - subgroupStart) {
        case 0:
          role = PairRole::FIRST;
          break;
        case 1:
          role = PairRole::SECOND;
          break;
      }
    }

    // Make a subgroup containing all the groups with the same half as mine and
    // decend into binary swap (now that we have power of 2 in that group).
    int rankRange[2][3];
    switch (role) {
      case PairRole::FIRST:
        rankRange[0][0] = 0;
        rankRange[1][0] = firstSwapGroup;
        break;
      case PairRole::SECOND:
        rankRange[0][0] = 1;
        rankRange[1][0] = firstSwapGroup + 1;
        break;
    }
    rankRange[0][1] = firstSwapGroup - 1;
    rankRange[0][2] = 3;
    rankRange[1][1] = numProc - 1;
    rankRange[1][2] = 2;

    MPI_Group subgroup;
    MPI_Group_range_incl(group, 2, rankRange, &subgroup);

    std::unique_ptr<Image> result = BinarySwapBase().compose(
        workingImage.get(), subgroup, communicator, yaml);

    MPI_Group_free(&subgroup);
    return result;
  } else {
    // Case 2, use 3-2 and 4-2 elimination
    // Each 3-2 elimination takes one process out of the composite, and each
    // 4-2 elimination takes two processes out. Thus, we need this many 4-2
    // eliminations.
    int num42Eliminations = numProc - targetP2 - (targetP2 / 2);
    int first32Group = num42Eliminations * 4;

    // Do the prescribed swap or elemination.
    enum struct PairRole { FIRST, SECOND };
    PairRole role;
    std::unique_ptr<Image> workingImage;
    if (rank < first32Group) {
      // Case 2.a: This rank in a 4-2 elimination
      int subgroupStart = (rank / 4) * 4;
      workingImage =
          Eliminate42(localImage, group, communicator, subgroupStart);
      assert(rank >= subgroupStart);
      assert((rank - subgroupStart) < 4);
      switch (rank - subgroupStart) {
        case 0:
          role = PairRole::FIRST;
          break;
        case 1:
          role = PairRole::SECOND;
          break;
        default:
          return workingImage;
      }
    } else {
      // Case 2.b: This rank in a 3-2 elimination
      int subgroupStart = (((rank - first32Group) / 3) * 3) + first32Group;
      workingImage =
          Eliminate32(localImage, group, communicator, subgroupStart);
      assert(rank >= subgroupStart);
      assert((rank - subgroupStart) < 3);
      switch (rank - subgroupStart) {
        case 0:
          role = PairRole::FIRST;
          break;
        case 1:
          role = PairRole::SECOND;
          break;
        default:
          return workingImage;
      }
    }

    // Make a subgroup containing all the groups with the same half as mine and
    // decend into binary swap (now that we have power of 2 in that group).
    int rankRange[2][3];
    switch (role) {
      case PairRole::FIRST:
        rankRange[0][0] = 0;
        rankRange[1][0] = first32Group;
        break;
      case PairRole::SECOND:
        rankRange[0][0] = 1;
        rankRange[1][0] = first32Group + 1;
        break;
    }
    rankRange[0][1] = first32Group - 1;
    rankRange[0][2] = 4;
    rankRange[1][1] = numProc - 1;
    rankRange[1][2] = 3;

    MPI_Group subgroup;
    if (first32Group > 0) {
      MPI_Group_range_incl(group, 2, rankRange, &subgroup);
    } else {
      // Special case where they are all 3-2 eliminations.
      MPI_Group_range_incl(group, 1, &rankRange[1], &subgroup);
    }

    std::unique_ptr<Image> result = BinarySwapBase().compose(
        workingImage.get(), subgroup, communicator, yaml);

    MPI_Group_free(&subgroup);
    return result;
  }
}
