// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "DirectSendBase.hpp"

#include <Common/MainLoop.hpp>

#include <array>

constexpr int DEFAULT_MAX_IMAGE_SPLIT = 1000000;

static int getRealRank(MPI_Group group, int rank, MPI_Comm communicator) {
  MPI_Group commGroup;
  MPI_Comm_group(communicator, &commGroup);

  int realRank;
  MPI_Group_translate_ranks(group, 1, &rank, commGroup, &realRank);
  return realRank;
}

static void getPieceRange(int imageSize,
                          int pieceIndex,
                          int numPieces,
                          int& rangeBeginOut,
                          int& rangeEndOut) {
  assert(pieceIndex >= 0);
  assert(pieceIndex < numPieces);

  int pieceSize = imageSize / numPieces;
  rangeBeginOut = pieceSize * pieceIndex;
  if (pieceIndex < numPieces - 1) {
    rangeEndOut = rangeBeginOut + pieceSize;
  } else {
    rangeEndOut = imageSize;
  }
}

static void PostReceives(
    Image* localImage,
    MPI_Group sendGroup,
    MPI_Group recvGroup,
    MPI_Comm communicator,
    std::vector<MPI_Request>& requestsOut,
    std::vector<std::unique_ptr<const Image>>& incomingImagesOut) {
  int recvGroupRank;
  MPI_Group_rank(recvGroup, &recvGroupRank);
  if (recvGroupRank == MPI_UNDEFINED) {
    // I am not receiving anything. Just create an "empty" incoming image
    incomingImagesOut.push_back(localImage->window(0, 0));
    return;
  }
  int recvGroupSize;
  MPI_Group_size(recvGroup, &recvGroupSize);

  int sendGroupRank;
  MPI_Group_rank(sendGroup, &sendGroupRank);
  int sendGroupSize;
  MPI_Group_size(sendGroup, &sendGroupSize);

  int rangeBegin;
  int rangeEnd;
  getPieceRange(localImage->getNumberOfPixels(),
                recvGroupRank,
                recvGroupSize,
                rangeBegin,
                rangeEnd);

  incomingImagesOut.resize(sendGroupSize);
  for (int sendGroupIndex = 0; sendGroupIndex < sendGroupSize;
       ++sendGroupIndex) {
    if (sendGroupIndex != sendGroupRank) {
      std::unique_ptr<Image> recvImageBuffer =
          localImage->createNew(rangeBegin, rangeEnd);
      std::vector<MPI_Request> newRequests = recvImageBuffer->IReceive(
          getRealRank(sendGroup, sendGroupIndex, communicator), communicator);
      requestsOut.insert(
          requestsOut.end(), newRequests.begin(), newRequests.end());
      incomingImagesOut[sendGroupIndex].reset(recvImageBuffer.release());
    } else {
      // "Sending" to self. Just record a shallow copy of the image.
      incomingImagesOut[sendGroupIndex] =
          localImage->window(rangeBegin, rangeEnd);
    }
  }
}

static void PostSends(
    Image* localImage,
    MPI_Group sendGroup,
    MPI_Group recvGroup,
    MPI_Comm communicator,
    std::vector<MPI_Request>& requestsOut,
    std::vector<std::unique_ptr<const Image>>& outgoingImagesOut) {
  int sendGroupRank;
  MPI_Group_rank(sendGroup, &sendGroupRank);
  if (sendGroupRank == MPI_UNDEFINED) {
    // I am not sending anything. Nothing to do.
    return;
  }
  int sendGroupSize;
  MPI_Group_size(sendGroup, &sendGroupSize);

  int recvGroupRank;
  MPI_Group_rank(recvGroup, &recvGroupRank);
  int recvGroupSize;
  MPI_Group_size(recvGroup, &recvGroupSize);

  outgoingImagesOut.resize(recvGroupSize);
  for (int recvGroupIndex = 0; recvGroupIndex < recvGroupSize;
       ++recvGroupIndex) {
    if (recvGroupIndex != recvGroupRank) {
      int rangeBegin;
      int rangeEnd;
      getPieceRange(localImage->getNumberOfPixels(),
                    recvGroupIndex,
                    recvGroupSize,
                    rangeBegin,
                    rangeEnd);
      std::unique_ptr<const Image> outImage =
          localImage->window(rangeBegin, rangeEnd);
      std::vector<MPI_Request> newRequests = outImage->ISend(
          getRealRank(recvGroup, recvGroupIndex, communicator), communicator);
      requestsOut.insert(
          requestsOut.end(), newRequests.begin(), newRequests.end());
      outgoingImagesOut[recvGroupIndex].swap(outImage);
    } else {
      // Do not need to send. PostReceives just did a shallow copy of the data.
    }
  }
}

static std::unique_ptr<Image> ProcessIncomingImages(
    std::vector<MPI_Request>& requests,
    std::vector<std::unique_ptr<const Image>>& incomingImages) {
  if (requests.size() > 0) {
    MPI_Waitall(requests.size(), requests.data(), MPI_STATUSES_IGNORE);
  }

  assert(incomingImages.size() > 0);
  if (incomingImages.size() == 1) {
    // Unexpected corner case where there is just one image.
    return incomingImages[0]->deepCopy();
  }

  std::unique_ptr<Image> workingImage =
      incomingImages[0]->blend(*incomingImages[1]);
  for (int imageIndex = 2; imageIndex < incomingImages.size(); ++imageIndex) {
    workingImage = workingImage->blend(*incomingImages[imageIndex]);
  }

  return workingImage;
}

std::unique_ptr<Image> DirectSendBase::compose(Image* localImage,
                                               MPI_Group sendGroup,
                                               MPI_Group recvGroup,
                                               MPI_Comm communicator,
                                               YamlWriter&) {
  std::vector<MPI_Request> recvRequests;
  std::vector<std::unique_ptr<const Image>> incomingImages;
  PostReceives(localImage,
               sendGroup,
               recvGroup,
               communicator,
               recvRequests,
               incomingImages);

  std::vector<MPI_Request> sendRequests;
  std::vector<std::unique_ptr<const Image>> outgoingImages;
  PostSends(localImage,
            sendGroup,
            recvGroup,
            communicator,
            sendRequests,
            outgoingImages);

  std::unique_ptr<Image> resultImage =
      ProcessIncomingImages(recvRequests, incomingImages);

  if (sendRequests.size() > 0) {
    MPI_Waitall(sendRequests.size(), sendRequests.data(), MPI_STATUSES_IGNORE);
  }

  return resultImage;
}

DirectSendBase::DirectSendBase() : maxSplit(DEFAULT_MAX_IMAGE_SPLIT) {}

std::unique_ptr<Image> DirectSendBase::compose(Image* localImage,
                                               MPI_Group group,
                                               MPI_Comm communicator,
                                               YamlWriter& yaml) {
  int groupSize;
  MPI_Group_size(group, &groupSize);

  MPI_Group recvGroup;
  std::array<int[3], 1> procRange = {
      0, std::min(this->maxSplit, groupSize) - 1, 1};
  MPI_Group_range_incl(group, 1, procRange.data(), &recvGroup);

  std::unique_ptr<Image> result =
      this->compose(localImage, group, recvGroup, communicator, yaml);

  MPI_Group_free(&recvGroup);

  return result;
}

enum optionIndex { MAX_IMAGE_SPLIT };

std::vector<option::Descriptor> DirectSendBase::getOptionVector() {
  std::vector<option::Descriptor> usage;
  // clang-format off
  usage.push_back(
    {MAX_IMAGE_SPLIT, 0, "", "max-image-split", PositiveIntArg,
     "  --max-image-split=<num> Set the maximum number of times the image will\n"
     "                          be split during compositing. Setting this\n"
     "                          parameter can reduce the total network traffic,\n"
     "                          but at the expense of load imbalance.\n"});
  // clang-format on

  return usage;
}

bool DirectSendBase::setOptions(const std::vector<option::Option>& options,
                                MPI_Comm,
                                YamlWriter& yaml) {
  if (options[MAX_IMAGE_SPLIT]) {
    this->maxSplit = atoi(options[MAX_IMAGE_SPLIT].arg);
  }
  yaml.AddDictionaryEntry("max-image-split", this->maxSplit);

  return true;
}
