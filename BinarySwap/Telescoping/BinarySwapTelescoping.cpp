// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "BinarySwapTelescoping.hpp"
#include "../Base/BinarySwapBase.hpp"

#include <array>

static int getLargestPowerOfTwoNoBiggerThan(int x) {
  int power2 = 1;
  while (power2 <= x) {
    power2 *= 2;
  }
  return power2 / 2;
}

// Given the rank of a group of a given size (which must be a power of 2),
// returns the index of the image piece that that rank is holding.
static int rankToImagePiece(int rank, int numProc) {
  // It turns out you can get the piece index by reversing the bits of the rank
  // (and vice versa).
  int piece = 0;
  int bitStack = rank;
  for (int i = 1; i < numProc; i <<= 1) {
    piece <<= 1;
    piece |= bitStack & 0x0001;
    bitStack >>= 1;
  }
  return piece;
}

// Given the image piece index that a process is holding and the size of a
// group (which must be a power of 2), returns the rank of the process in the
// group holding that piece of image.
static int imagePieceToRank(int piece, int numProc) {
  // Turns out the conversion from piece to rank is the same as rank to piece.
  return rankToImagePiece(piece, numProc);
}

static std::unique_ptr<const Image> getSubregion(const Image &image,
                                                 int pieceIndex,
                                                 int numPieces) {
  int subRegionBegin = 0;
  int subRegionEnd = image.getNumberOfPixels();

  // We have to be careful about how we break up the subregions of the image.
  // We need to make sure we match how binary-swap does it.
  while (numPieces > 1) {
    int midIndex = (subRegionEnd + subRegionBegin) / 2;
    numPieces /= 2;
    if (pieceIndex < numPieces) {
      subRegionEnd = midIndex;
    } else {
      subRegionBegin = midIndex;
      pieceIndex -= numPieces;
    }
  }

  return image.window(subRegionBegin, subRegionEnd);
}

static int getRealRank(MPI_Group group, int rank, MPI_Comm communicator) {
  MPI_Group commGroup;
  MPI_Comm_group(communicator, &commGroup);

  int realRank;
  MPI_Group_translate_ranks(group, 1, &rank, commGroup, &realRank);
  return realRank;
}

std::unique_ptr<Image> BinarySwapTelescoping::composeBigGroup(
    Image *localImage,
    MPI_Group bigGroup,
    MPI_Group littleGroup,
    MPI_Comm communicator,
    YamlWriter &yaml) {
  int myGroupRank;
  MPI_Group_rank(bigGroup, &myGroupRank);
  assert(myGroupRank != MPI_UNDEFINED);

  int myGroupSize;
  MPI_Group_size(bigGroup, &myGroupSize);

  int otherGroupSize;
  MPI_Group_size(littleGroup, &otherGroupSize);
  // All data will be in the biggest power-of-two partition.
  otherGroupSize = getLargestPowerOfTwoNoBiggerThan(otherGroupSize);

  // Start by doing a normal binary swap.
  std::unique_ptr<Image> composedImage =
      BinarySwapBase().compose(localImage, bigGroup, communicator, yaml);

  // Figure out how many image pieces each process in the little group holds.
  int numImagePiecesInEachLittleProc = myGroupSize / otherGroupSize;

  // The ordering that binary-swap leaves image pieces is weird. Figure out
  // which piece I have.
  int myPieceIndex = rankToImagePiece(myGroupRank, myGroupSize);

  // The little group has different piece indexing since it has fewer large
  // pieces. Figure out which piece from the little group has my piece.
  int correspondingPieceIndex = myPieceIndex / numImagePiecesInEachLittleProc;

  // Figure out which rank in the little group has my corresponding piece.
  int correspondingRank =
      imagePieceToRank(correspondingPieceIndex, otherGroupSize);

  // Now receive the image and blend it into my own.
  std::unique_ptr<Image> incomingImage = composedImage->createNew();
  incomingImage->Receive(
      getRealRank(littleGroup, correspondingRank, communicator), communicator);
  composedImage = composedImage->blend(*incomingImage);

  return composedImage;
}

void BinarySwapTelescoping::composeLittleGroup(Image *localImage,
                                               MPI_Group bigGroup,
                                               MPI_Group littleGroup,
                                               MPI_Comm communicator,
                                               YamlWriter &yaml) {
  int myGroupRank;
  MPI_Group_rank(littleGroup, &myGroupRank);
  assert(myGroupRank != MPI_UNDEFINED);

  int myGroupSize;
  MPI_Group_size(littleGroup, &myGroupSize);

  int otherGroupSize;
  MPI_Group_size(bigGroup, &otherGroupSize);

  // Recursively call myself to composite my group.
  std::unique_ptr<Image> composedImage =
      this->compose(localImage, littleGroup, communicator, yaml);

  // The compose method will only return an image in the top power-of-two
  // processes. The rest are empty. The rest of this function only deals with
  // this part of the group.
  myGroupSize = getLargestPowerOfTwoNoBiggerThan(myGroupSize);
  if (myGroupSize <= myGroupRank) {
    assert(composedImage->getNumberOfPixels() == 0);
    return;
  }

  // Figure out how many pieces I have to break up my image into.
  int numImagePiecesInEachLittleProc = otherGroupSize / myGroupSize;

  // The ordering that binary-swap leaves image pieces is weird. Figure out
  // which piece I have.
  int myPieceIndex = rankToImagePiece(myGroupRank, myGroupSize);

  // The big group has different piece indexing since it has more small
  // pieces. Figure out the index of the first piece that I have.
  int firstCorrespondingPieceIndex =
      myPieceIndex * numImagePiecesInEachLittleProc;

  std::vector<std::unique_ptr<const Image>> subImages;
  std::vector<MPI_Request> sendRequests;
  for (int subPieceIndex = 0; subPieceIndex < numImagePiecesInEachLittleProc;
       ++subPieceIndex) {
    subImages.push_back(getSubregion(
        *composedImage, subPieceIndex, numImagePiecesInEachLittleProc));
    int destRank = imagePieceToRank(
        firstCorrespondingPieceIndex + subPieceIndex, otherGroupSize);
    std::vector<MPI_Request> newRequests = subImages.back()->ISend(
        getRealRank(bigGroup, destRank, communicator), communicator);
    sendRequests.insert(
        sendRequests.end(), newRequests.begin(), newRequests.end());
  }

  // Wait for everything to finish sending.
  MPI_Waitall(sendRequests.size(), sendRequests.data(), MPI_STATUSES_IGNORE);
}

std::unique_ptr<Image> BinarySwapTelescoping::compose(Image *localImage,
                                                      MPI_Group group,
                                                      MPI_Comm communicator,
                                                      YamlWriter &yaml) {
  // The base binary-swap algorithm only operates on process groups with a size
  // of a power-of-two. This compositing algorithm first identifies a large
  // subgroup that is a power-of-two and runs the base binary-swap on that
  // partition. Concurrently, it recursively calls the telescoping method on
  // the remaining partition. After both complete, image pieces from the
  // smaller group are transferred to corresponding processes in the larger
  // group.

  int originalGroupSize;
  MPI_Group_size(group, &originalGroupSize);

  int targetGroupSize = getLargestPowerOfTwoNoBiggerThan(originalGroupSize);

  // Special case: we already have a power of two. Just call the base
  // binary-swap and return.
  if (targetGroupSize == originalGroupSize) {
    return BinarySwapBase().compose(localImage, group, communicator, yaml);
  }

  // Split up the group into two partitions.
  std::array<int[3], 1> largeGroupRange = {{0, targetGroupSize - 1, 1}};

  MPI_Group bigGroup;
  MPI_Group_range_incl(group, 1, largeGroupRange.data(), &bigGroup);

  MPI_Group littleGroup;
  MPI_Group_range_excl(group, 1, largeGroupRange.data(), &littleGroup);

  // Call the appropriate method depending on whether I am in the big group
  // or little group.
  int bigGroupRank;
  MPI_Group_rank(bigGroup, &bigGroupRank);

  std::unique_ptr<Image> resultImage;

  if (bigGroupRank != MPI_UNDEFINED) {
    resultImage = this->composeBigGroup(
        localImage, bigGroup, littleGroup, communicator, yaml);
  } else {
    this->composeLittleGroup(
        localImage, bigGroup, littleGroup, communicator, yaml);
    resultImage = localImage->copySubrange(0, 0);
  }

  // Cleanup
  MPI_Group_free(&bigGroup);
  MPI_Group_free(&littleGroup);

  return resultImage;
}
