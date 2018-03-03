// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "Swap_2_3_Node.hpp"

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);

  MPI_Group group;
  MPI_Comm_group(MPI_COMM_WORLD, &group);

  Swap_2_3_Node *tree = new Swap_2_3_Node(group, 100);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    tree->PrintSummary(group);
  }

  delete tree;

  MPI_Finalize();
  return 0;
}
