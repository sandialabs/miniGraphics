// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef SWAP_2_3_NODE_HPP
#define SWAP_2_3_NODE_HPP

#include <mpi.h>

#include <memory>
#include <utility>
#include <vector>

struct Swap_2_3_Node {
  MPI_Group group;
  int groupSize;
  std::vector<int> regionIndices;
  std::vector<std::unique_ptr<const Swap_2_3_Node>> subnodes;

  Swap_2_3_Node(MPI_Group _group, int imageSize);
  ~Swap_2_3_Node();

  Swap_2_3_Node(const Swap_2_3_Node&) = delete;
  void operator=(const Swap_2_3_Node&) = delete;

  void PrintSummary(MPI_Group refGroup, int indent = 0) const;

 private:
  Swap_2_3_Node(MPI_Group _group, int imageSize, int largerPowerOfTwo);
  void setup(MPI_Group _group, int imageSize, int largerPowerOfTwo);
};

#endif  // SWAP_2_3_NODE_HPP
