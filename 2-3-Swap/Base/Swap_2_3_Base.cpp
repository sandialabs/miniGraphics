// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "Swap_2_3_Base.hpp"

#include "Swap_2_3_Node.hpp"

#include <Common/SavePPM.hpp>
#include <Common/Timer.hpp>

#include <algorithm>
#include <cmath>

static int getRealRank(MPI_Group group, int rank, MPI_Comm communicator) {
  MPI_Group commGroup;
  MPI_Comm_group(communicator, &commGroup);

  int realRank;
  MPI_Group_translate_ranks(group, 1, &rank, commGroup, &realRank);

  MPI_Group_free(&commGroup);
  return realRank;
}

struct Incoming_2_3_SwapImage {
  std::unique_ptr<Image> imageBuffer;
  std::vector<MPI_Request> receiveRequests;
  int relativeSubtreeIndex;
};

static void PostReceivesFromSubtree(
    const Image& myImage,
    const Swap_2_3_Node& subtree,
    int relativeSubtreeIndex,
    MPI_Comm communicator,
    std::vector<Incoming_2_3_SwapImage>& incoming) {
  for (int groupIndex = 0; groupIndex < subtree.groupSize; ++groupIndex) {
    int regionEnd = subtree.regionIndices[groupIndex + 1];
    if (regionEnd <= myImage.getRegionBegin()) {
      // Have not yet got to the parts of the image intersecting mine
      continue;
    }
    int regionBegin = subtree.regionIndices[groupIndex];
    if (regionBegin >= myImage.getRegionEnd()) {
      // Past any image part that intersects mine
      break;
    }

    incoming.resize(incoming.size() + 1);
    incoming.back().relativeSubtreeIndex = relativeSubtreeIndex;
    incoming.back().imageBuffer =
        myImage.createNew(std::max(regionBegin, myImage.getRegionBegin()),
                          std::min(regionEnd, myImage.getRegionEnd()));
    incoming.back().receiveRequests = incoming.back().imageBuffer->IReceive(
        getRealRank(subtree.group, groupIndex, communicator), communicator);
  }
}

static std::unique_ptr<const Image> PostReceives(
    const Image& image,
    const Swap_2_3_Node& tree,
    MPI_Comm communicator,
    std::vector<Incoming_2_3_SwapImage>& primaryIncoming,
    std::vector<Incoming_2_3_SwapImage>& secondaryIncoming) {
  int myGroupRank;
  MPI_Group_rank(tree.group, &myGroupRank);

  int myRegionBegin = tree.regionIndices[myGroupRank];
  int myRegionEnd = tree.regionIndices[myGroupRank + 1];
  std::unique_ptr<const Image> windowedImage =
      image.window(myRegionBegin - image.getRegionBegin(),
                   myRegionEnd - image.getRegionBegin());

  int mySubtree = -1;
  int mySubtreeRank;
  for (int subtree = 0; subtree < tree.subnodes.size(); ++subtree) {
    MPI_Group_rank(tree.subnodes[subtree]->group, &mySubtreeRank);
    if (mySubtreeRank != MPI_UNDEFINED) {
      mySubtree = subtree;
      break;
    }
  }
  assert(mySubtreeRank != MPI_UNDEFINED);
  assert(mySubtree >= 0);

  for (int subtree = 0; subtree < tree.subnodes.size(); ++subtree) {
    int relativeSubtreeIndex = subtree - mySubtree;
    if (relativeSubtreeIndex == 0) {
      // Not receiving from my own subtree
      continue;
    } else if (std::abs(relativeSubtreeIndex) == 1) {
      PostReceivesFromSubtree(*windowedImage,
                              *tree.subnodes[subtree],
                              relativeSubtreeIndex,
                              communicator,
                              primaryIncoming);
    } else {
      PostReceivesFromSubtree(*windowedImage,
                              *tree.subnodes[subtree],
                              relativeSubtreeIndex,
                              communicator,
                              secondaryIncoming);
    }
  }

  return windowedImage;
}

static void PostSends(const Image& image,
                      const Swap_2_3_Node& tree,
                      MPI_Comm communicator,
                      std::vector<MPI_Request>& requests,
                      std::vector<std::unique_ptr<const Image>>& sendBuffers) {
  int myGroupRank;
  MPI_Group_rank(tree.group, &myGroupRank);

  for (int groupRank = 0; groupRank < tree.groupSize; ++groupRank) {
    if (groupRank == myGroupRank) {
      // Don't need to send to myself.
      continue;
    }
    int regionBegin =
        std::max(tree.regionIndices[groupRank], image.getRegionBegin());
    int regionEnd =
        std::min(tree.regionIndices[groupRank + 1], image.getRegionEnd());
    if (regionEnd <= regionBegin) {
      // Nothing to send to this process.
      continue;
    }

    // Make buffer to send. This needs to stick around until the send completes.
    sendBuffers.push_back(image.window(regionBegin - image.getRegionBegin(),
                                       regionEnd - image.getRegionBegin()));

    std::vector<MPI_Request> newRequests = sendBuffers.back()->ISend(
        getRealRank(tree.group, groupRank, communicator), communicator);

    requests.insert(requests.end(), newRequests.begin(), newRequests.end());
  }
}

static std::unique_ptr<Image> ProcessIncomingImages(
    const Image& startingImage, std::vector<Incoming_2_3_SwapImage>& incoming) {
  assert(!incoming.empty());

  // Collect the last request for each incoming image. We will wait for these
  // last requests to see which image gets here first.
  std::vector<MPI_Request> lastRequests;
  lastRequests.reserve(incoming.size());
  for (auto&& in : incoming) {
    lastRequests.push_back(in.receiveRequests.back());
    in.receiveRequests.pop_back();
  }

  std::unique_ptr<Image> workingBuffer;
  const Image* workingImage = &startingImage;

  while (!incoming.empty()) {
    assert(incoming.size() == lastRequests.size());

    // Wait for MPI to tell us what image comes in next.
    int receiveIndex;
    MPI_Waitany(lastRequests.size(),
                lastRequests.data(),
                &receiveIndex,
                MPI_STATUS_IGNORE);
    Incoming_2_3_SwapImage& in = incoming[receiveIndex];

    // Make sure all messages for this image are complete
    MPI_Waitall(in.receiveRequests.size(),
                in.receiveRequests.data(),
                MPI_STATUSES_IGNORE);

    // All data is now guaranteed to be in the image. Do composite.
    if (in.relativeSubtreeIndex < 0) {
      workingBuffer = in.imageBuffer->blend(*workingImage);
    } else {
      workingBuffer = workingImage->blend(*in.imageBuffer);
    }
    workingImage = workingBuffer.get();

    // Remove incoming information
    lastRequests.erase(lastRequests.begin() + receiveIndex);
    incoming.erase(incoming.begin() + receiveIndex);
  }

  assert(workingBuffer != nullptr);
  return workingBuffer;
}

static std::unique_ptr<Image> Do_2_3_Swap(Image* localImage,
                                          const Swap_2_3_Node& tree,
                                          MPI_Comm communicator) {
  if (tree.subnodes.size() == 0) {
    // At leaf. Nothing to do.
    return localImage->shallowCopy();
  }

  // Do the compositing for the subnode
  std::unique_ptr<Image> startingImage;
  for (auto&& subnode : tree.subnodes) {
    int myGroupRank;
    MPI_Group_rank(subnode->group, &myGroupRank);
    if (myGroupRank != MPI_UNDEFINED) {
      startingImage = Do_2_3_Swap(localImage, *subnode, communicator);
      break;
    }
  }
  assert(startingImage && "Malformed 2-3 swap compositing tree");

  std::vector<Incoming_2_3_SwapImage> primaryIncoming;
  std::vector<Incoming_2_3_SwapImage> secondaryIncoming;
  std::unique_ptr<const Image> myStartingWindow = PostReceives(
      *startingImage, tree, communicator, primaryIncoming, secondaryIncoming);

  std::vector<MPI_Request> sendRequests;
  std::vector<std::unique_ptr<const Image>> sendBuffers;
  PostSends(*startingImage, tree, communicator, sendRequests, sendBuffers);

  // Receive primary images, which have to be blended first.
  std::unique_ptr<Image> workingImage =
      ProcessIncomingImages(*myStartingWindow, primaryIncoming);

  // Receive secondary image, if any
  if (!secondaryIncoming.empty()) {
    workingImage = ProcessIncomingImages(*workingImage, secondaryIncoming);
  }

  // Wait for sends to complete
  MPI_Waitall(sendRequests.size(), sendRequests.data(), MPI_STATUSES_IGNORE);

  return workingImage;
}

std::unique_ptr<Image> Swap_2_3_Base::compose(Image* localImage,
                                              MPI_Group group,
                                              MPI_Comm communicator,
                                              YamlWriter& yaml) {
  Timer timer(yaml, "construct-tree-seconds");
  Swap_2_3_Node compositeTree(group, localImage->getNumberOfPixels());
  timer.stop();

  return Do_2_3_Swap(localImage, compositeTree, communicator);
}
