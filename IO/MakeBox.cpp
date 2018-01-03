// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "MakeBox.hpp"

static const int NumVerts = 8;
static const float Verts[NumVerts][3] = {{0, 0, 0},
                                         {1, 0, 0},
                                         {0, 1, 0},
                                         {1, 1, 0},
                                         {0, 0, 1},
                                         {1, 0, 1},
                                         {0, 1, 1},
                                         {1, 1, 1}};

static const int NumTris = 12;
static const int Tries[NumTris][3] = {{0, 2, 1},
                                      {1, 2, 3},
                                      {4, 5, 6},
                                      {5, 7, 6},
                                      {0, 6, 2},
                                      {0, 4, 6},
                                      {1, 3, 7},
                                      {1, 7, 5},
                                      {0, 1, 5},
                                      {0, 5, 4},
                                      {2, 7, 3},
                                      {2, 6, 7}};

void MakeBox(Mesh &mesh) {
  mesh = Mesh(8, 12);

  for (int vertexIndex = 0; vertexIndex < NumVerts; ++vertexIndex) {
    mesh.setVertex(vertexIndex,
                   glm::vec3(Verts[vertexIndex][0],
                             Verts[vertexIndex][1],
                             Verts[vertexIndex][2]));
  }

  for (int triIndex = 0; triIndex < NumTris; ++triIndex) {
    mesh.setTriangle(triIndex, Tries[triIndex]);
  }
}
