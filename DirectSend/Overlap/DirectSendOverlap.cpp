// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "DirectSendOverlap.hpp"

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

struct IncomingDirectSendImage {
  std::unique_ptr<Image> imageBuffer;
  std::vector<MPI_Request> receiveRequests;
  enum { WAITING, READY, EMPTY } status;
};

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
    std::vector<IncomingDirectSendImage>& incomingImagesOut) {
  int recvGroupRank;
  MPI_Group_rank(recvGroup, &recvGroupRank);
  if (recvGroupRank == MPI_UNDEFINED) {
    // I am not receiving anything. Just create an "empty" incoming image
    incomingImagesOut.resize(1);
    incomingImagesOut[0].imageBuffer = localImage->copySubrange(0, 0);
    incomingImagesOut[0].status = IncomingDirectSendImage::READY;
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
      incomingImagesOut[sendGroupIndex].imageBuffer =
          localImage->createNew(rangeBegin, rangeEnd);
      incomingImagesOut[sendGroupIndex].receiveRequests =
          incomingImagesOut[sendGroupIndex].imageBuffer->IReceive(
              getRealRank(sendGroup, sendGroupIndex, communicator),
              communicator);
      incomingImagesOut[sendGroupIndex].status =
          IncomingDirectSendImage::WAITING;
    } else {
      // "Sending" to self. Just record a shallow copy of the image.
      std::unique_ptr<const Image> selfSendImage =
          localImage->window(rangeBegin, rangeEnd);
      // I know, this const cast is bad form. But the next thing to happen to
      // this image is to get blended with something else. The risk is low
      // and it's just too much trouble to get the const-ness exact.
      incomingImagesOut[sendGroupIndex].imageBuffer.reset(
          const_cast<Image*>(selfSendImage.release()));
      incomingImagesOut[sendGroupIndex].status = IncomingDirectSendImage::READY;
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
    std::vector<IncomingDirectSendImage>& incoming) {
  assert(!incoming.empty());

  // Collect the last request for each incoming image. We will wait for these
  // last requests to see which image gets here first.
  std::vector<MPI_Request> lastRequests;
  lastRequests.reserve(incoming.size());
  int numPending = 0;
  for (auto&& in : incoming) {
    switch (in.status) {
      case IncomingDirectSendImage::WAITING:
        lastRequests.push_back(in.receiveRequests.back());
        in.receiveRequests.pop_back();
        ++numPending;
        break;
      case IncomingDirectSendImage::READY:
      case IncomingDirectSendImage::EMPTY:
        assert(in.receiveRequests.empty());
        lastRequests.push_back(MPI_REQUEST_NULL);
        break;
    }
  }

  while (numPending > 0) {
    int receiveIndex;
    MPI_Waitany(lastRequests.size(),
                lastRequests.data(),
                &receiveIndex,
                MPI_STATUSES_IGNORE);
    --numPending;
    // Make sure all the messages have come in
    MPI_Waitall(incoming[receiveIndex].receiveRequests.size(),
                incoming[receiveIndex].receiveRequests.data(),
                MPI_STATUSES_IGNORE);
    incoming[receiveIndex].status = IncomingDirectSendImage::READY;

    // Check all incoming images and find candidates to blend
    for (auto targetIn = incoming.begin(); targetIn != incoming.end();
         ++targetIn) {
      if (targetIn->status == IncomingDirectSendImage::READY) {
        // This image is ready to blend. Find any other images that can be
        // blended with it.
        for (auto sourceIn = targetIn + 1; sourceIn != incoming.end();
             ++sourceIn) {
          if (sourceIn->status == IncomingDirectSendImage::READY) {
            // Blend these two images together. Store the result in target and
            // zero out the source.
            targetIn->imageBuffer =
                targetIn->imageBuffer->blend(*sourceIn->imageBuffer);
            sourceIn->status = IncomingDirectSendImage::EMPTY;
            sourceIn->imageBuffer.reset();
          } else if (sourceIn->status == IncomingDirectSendImage::WAITING) {
            if (targetIn->imageBuffer->blendIsOrderDependent()) {
              // If blend is order dependent, we cannot blend any other images
              break;
            }
          } else /* sourceIn->status == EMPTY */ {
            // Just skip over empty images.
          }
        }
      }
    }
  }

  // Resulting image should be in first incoming state.
  assert(incoming.front().status == IncomingDirectSendImage::READY);

  return std::unique_ptr<Image>(incoming.front().imageBuffer.release());
}

std::unique_ptr<Image> DirectSendOverlap::compose(Image* localImage,
                                                  MPI_Group sendGroup,
                                                  MPI_Group recvGroup,
                                                  MPI_Comm communicator,
                                                  YamlWriter&) {
  std::vector<IncomingDirectSendImage> incomingImages;
  PostReceives(localImage, sendGroup, recvGroup, communicator, incomingImages);

  std::vector<MPI_Request> sendRequests;
  std::vector<std::unique_ptr<const Image>> outgoingImages;
  PostSends(localImage,
            sendGroup,
            recvGroup,
            communicator,
            sendRequests,
            outgoingImages);

  std::unique_ptr<Image> resultImage = ProcessIncomingImages(incomingImages);

  if (sendRequests.size() > 0) {
    MPI_Waitall(sendRequests.size(), sendRequests.data(), MPI_STATUSES_IGNORE);
  }

  return resultImage;
}

DirectSendOverlap::DirectSendOverlap() : maxSplit(DEFAULT_MAX_IMAGE_SPLIT) {}

std::unique_ptr<Image> DirectSendOverlap::compose(Image* localImage,
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

std::vector<option::Descriptor> DirectSendOverlap::getOptionVector() {
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

bool DirectSendOverlap::setOptions(const std::vector<option::Option>& options,
                                   MPI_Comm,
                                   YamlWriter& yaml) {
  if (options[MAX_IMAGE_SPLIT]) {
    this->maxSplit = atoi(options[MAX_IMAGE_SPLIT].arg);
  }
  yaml.AddDictionaryEntry("max-image-split", this->maxSplit);

  return true;
}
