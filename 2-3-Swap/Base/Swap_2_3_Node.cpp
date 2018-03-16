// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include <Swap_2_3_Node.hpp>

#include <Common/Image.hpp>

#include <algorithm>
#include <array>

Swap_2_3_Node::Swap_2_3_Node(MPI_Group _group, int imageSize) {
  MPI_Group_size(_group, &this->groupSize);

  int largerPowerOfTwo = 1;
  while (largerPowerOfTwo <= groupSize) {
    largerPowerOfTwo *= 2;
  }

  this->setup(_group, imageSize, largerPowerOfTwo);
}

Swap_2_3_Node::Swap_2_3_Node(MPI_Group _group,
                             int imageSize,
                             int largerPowerOfTwo) {
  this->setup(_group, imageSize, largerPowerOfTwo);
}

void Swap_2_3_Node::setup(MPI_Group _group,
                          int imageSize,
                          int largerPowerOfTwo) {
  MPI_Group_size(_group, &this->groupSize);

  if (this->groupSize == 1) {
    int dummy;
    MPI_Group_excl(_group, 0, &dummy, &this->group);

    this->regionIndices = {0, imageSize};
    return;
  }

  if (this->groupSize < largerPowerOfTwo - 1) {
    // Divide by 2
    int subSize2 = this->groupSize / 2;
    int subSize1 = this->groupSize - subSize2;
    this->subnodes.resize(2);

    MPI_Group subGroup;
    std::array<int[3], 1> range = {0, subSize1 - 1, 1};
    MPI_Group_range_incl(_group, 1, range.data(), &subGroup);
    this->subnodes[0].reset(
        new Swap_2_3_Node(subGroup, imageSize, largerPowerOfTwo / 2));
    MPI_Group_free(&subGroup);

    range = {subSize1, this->groupSize - 1, 1};
    MPI_Group_range_incl(_group, 1, range.data(), &subGroup);
    this->subnodes[1].reset(
        new Swap_2_3_Node(subGroup, imageSize, largerPowerOfTwo / 2));
    MPI_Group_free(&subGroup);

    this->regionIndices.resize(this->groupSize + 1);
    if (subSize1 == subSize2) {
      // Split regions evenly
      for (int i = 0; i < subSize1; ++i) {
        this->regionIndices[2 * i] = this->subnodes[0]->regionIndices[i];
        this->regionIndices[2 * i + 1] =
            (this->subnodes[0]->regionIndices[i] +
             this->subnodes[0]->regionIndices[i + 1]) /
            2;
      }
    } else {
      // Divide image evenly
      int imagePieceSize = imageSize / this->groupSize;
      for (int i = 0; i < this->groupSize; ++i) {
        this->regionIndices[i] = i * imagePieceSize;
      }
    }
    this->regionIndices[this->groupSize] = imageSize;

    std::vector<int> mergedRanks(this->groupSize);
    std::vector<int> count(subSize1);
    for (int i = 0; i < subSize1; ++i) {
      count[i] = i;
    }
    std::vector<int> subRanks(subSize1);
    MPI_Group_translate_ranks(this->subnodes[0]->group,
                              subSize1,
                              count.data(),
                              _group,
                              subRanks.data());
    for (int i = 0; i < subSize1; ++i) {
      mergedRanks[i * 2] = subRanks[i];
    }

    MPI_Group_translate_ranks(this->subnodes[1]->group,
                              subSize2,
                              count.data(),
                              _group,
                              subRanks.data());
    for (int i = 0; i < subSize2; ++i) {
      mergedRanks[i * 2 + 1] = subRanks[i];
    }

    MPI_Group_incl(_group, groupSize, mergedRanks.data(), &this->group);
  } else {
    // Divide by 3
    int subSize2 = this->groupSize / 3;
    int subSize1 = this->groupSize - 2 * subSize2;
    this->subnodes.resize(3);

    MPI_Group subGroup;
    std::array<int[3], 1> range = {0, subSize1 - 1, 1};
    MPI_Group_range_incl(_group, 1, range.data(), &subGroup);
    this->subnodes[0].reset(
        new Swap_2_3_Node(subGroup, imageSize, largerPowerOfTwo / 2));
    MPI_Group_free(&subGroup);

    range = {subSize1, subSize1 + subSize2 - 1, 1};
    MPI_Group_range_incl(_group, 1, range.data(), &subGroup);
    this->subnodes[1].reset(
        new Swap_2_3_Node(subGroup, imageSize, largerPowerOfTwo / 2));
    MPI_Group_free(&subGroup);

    range = {subSize1 + subSize2, this->groupSize - 1, 1};
    MPI_Group_range_incl(_group, 1, range.data(), &subGroup);
    this->subnodes[2].reset(
        new Swap_2_3_Node(subGroup, imageSize, largerPowerOfTwo / 2));
    MPI_Group_free(&subGroup);

    this->regionIndices.resize(this->groupSize + 1);
    if (subSize1 == subSize2) {
      // Split regions evenly
      for (int i = 0; i < subSize1; ++i) {
        this->regionIndices[3 * i] = this->subnodes[0]->regionIndices[i];
        this->regionIndices[3 * i + 1] =
            (2 * this->subnodes[0]->regionIndices[i] +
             this->subnodes[0]->regionIndices[i + 1]) /
            3;
        this->regionIndices[3 * i + 2] =
            (this->subnodes[0]->regionIndices[i] +
             2 * this->subnodes[0]->regionIndices[i + 1]) /
            3;
      }
    } else {
      // Divide image evenly
      int imagePieceSize = imageSize / this->groupSize;
      for (int i = 0; i < this->groupSize; ++i) {
        this->regionIndices[i] = i * imagePieceSize;
      }
    }
    this->regionIndices[this->groupSize] = imageSize;

    std::vector<int> mergedRanks(this->groupSize);
    std::vector<int> count(subSize1);
    for (int i = 0; i < subSize1; ++i) {
      count[i] = i;
    }
    std::vector<int> subRanks(subSize1);
    MPI_Group_translate_ranks(this->subnodes[0]->group,
                              subSize1,
                              count.data(),
                              _group,
                              subRanks.data());
    for (int i = 0; i < subSize1; ++i) {
      mergedRanks[i * 3] = subRanks[i];
    }

    MPI_Group_translate_ranks(this->subnodes[1]->group,
                              subSize2,
                              count.data(),
                              _group,
                              subRanks.data());
    for (int i = 0; i < subSize2; ++i) {
      mergedRanks[i * 3 + 1] = subRanks[i];
    }

    MPI_Group_translate_ranks(this->subnodes[2]->group,
                              subSize2,
                              count.data(),
                              _group,
                              subRanks.data());
    for (int i = 0; i < subSize2; ++i) {
      mergedRanks[i * 3 + 2] = subRanks[i];
    }

    MPI_Group_incl(_group, groupSize, mergedRanks.data(), &this->group);
  }
}

Swap_2_3_Node::~Swap_2_3_Node() { MPI_Group_free(&this->group); }

void Swap_2_3_Node::PrintSummary(MPI_Group refGroup, int indent) const {
  for (int i = 0; i < indent; ++i) {
    std::cout << "  ";
  }
  std::vector<int> count(this->groupSize);
  for (int i = 0; i < this->groupSize; ++i) {
    count[i] = i;
  }
  std::vector<int> ranks(this->groupSize);
  MPI_Group_translate_ranks(
      this->group, this->groupSize, count.data(), refGroup, ranks.data());
  for (auto&& r : ranks) {
    std::cout << r << " ";
  }
  std::cout << "----";
  for (auto&& pixel : this->regionIndices) {
    std::cout << " " << pixel;
  }
  std::cout << std::endl;
  for (auto&& subnode : this->subnodes) {
    subnode->PrintSummary(refGroup, indent + 1);
  }
}
