// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "MeshHelper.hpp"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include <algorithm>
#include <array>
#include <cmath>

// A set of colors automatically assigned to mesh regions on each process.
// These colors come from color brewer (qualitative set 3 with 12 colors).
// http://colorbrewer2.org/?type=qualitative&scheme=Set3&n=12
static const float ProcessColors[][4] = {{0.5529f, 0.8274f, 0.7803f, 1.0f},
                                         {1.0000f, 1.0000f, 0.7019f, 1.0f},
                                         {0.7450f, 0.7294f, 0.8549f, 1.0f},
                                         {0.9843f, 0.5019f, 0.4470f, 1.0f},
                                         {0.5019f, 0.6941f, 0.8274f, 1.0f},
                                         {0.9921f, 0.7058f, 0.3843f, 1.0f},
                                         {0.7019f, 0.8705f, 0.4117f, 1.0f},
                                         {0.9882f, 0.8039f, 0.8980f, 1.0f},
                                         {0.8509f, 0.8509f, 0.8509f, 1.0f},
                                         {0.7372f, 0.5019f, 0.7411f, 1.0f},
                                         {0.8000f, 0.9215f, 0.7725f, 1.0f},
                                         {1.0000f, 0.9294f, 0.4352f, 1.0f}

};
static const int NumProcessColors = sizeof(ProcessColors) / (4 * sizeof(float));

void meshBroadcast(Mesh& mesh, float overlap, MPI_Comm communicator) {
  int rank;
  MPI_Comm_rank(communicator, &rank);

  int numProc;
  MPI_Comm_size(communicator, &numProc);

  if (rank == 0) {
    int gridDims[3];
    gridDims[0] = gridDims[1] = gridDims[2] =
        static_cast<int>(std::floor(std::cbrt(numProc)));
    for (int dim = 0; dim < 3; ++dim) {
      if ((gridDims[0] * gridDims[1] * gridDims[2]) < numProc) {
        ++gridDims[dim];
      } else {
        break;
      }
    }
    glm::vec3 spacing =
        (1.0f - overlap) * (mesh.getBoundsMax() - mesh.getBoundsMin());

    for (int dest = 1; dest < numProc; ++dest) {
      glm::vec3 gridLocation(dest % gridDims[0],
                             (dest / gridDims[0]) % gridDims[1],
                             dest / (gridDims[0] * gridDims[1]));
      glm::mat4 transform =
          glm::translate(glm::mat4(1.0f), spacing * gridLocation);

      Mesh sendMesh = mesh.deepCopy();
      sendMesh.transform(transform);
      sendMesh.setHomogeneousColor(
          Color(ProcessColors[dest % NumProcessColors]));

      sendMesh.send(dest, communicator);
    }

    mesh.setHomogeneousColor(Color(ProcessColors[0]));
  } else {
    mesh.receive(0, communicator);
  }
}

void meshScatter(Mesh& mesh, MPI_Comm communicator) {
  int rank;
  MPI_Comm_rank(communicator, &rank);

  int numProc;
  MPI_Comm_size(communicator, &numProc);

  if (rank == 0) {
    int numTriangles = mesh.getNumberOfTriangles();
    int numTriPerProcess = numTriangles / numProc;
    int startTriRank1 = numTriPerProcess + numTriangles % numProc;

    for (int dest = 1; dest < numProc; ++dest) {
      Mesh submesh =
          mesh.copySubset(startTriRank1 + numTriPerProcess * (dest - 1),
                          startTriRank1 + numTriPerProcess * dest);
      submesh.setHomogeneousColor(
          Color(ProcessColors[dest % NumProcessColors]));
      submesh.send(dest, communicator);
    }

    mesh = mesh.copySubset(0, startTriRank1);
    mesh.setHomogeneousColor(Color(ProcessColors[0]));
  } else {
    mesh.receive(0, communicator);
  }
}

Mesh meshGather(const Mesh& mesh, MPI_Comm communicator) {
  int rank;
  MPI_Comm_rank(communicator, &rank);

  int numProc;
  MPI_Comm_size(communicator, &numProc);

  Mesh outMesh;
  if (rank == 0) {
    outMesh = mesh.deepCopy();
    for (int src = 1; src < numProc; ++src) {
      Mesh recvMesh;
      recvMesh.receive(src, communicator);
      outMesh.append(recvMesh);
    }
  } else {
    mesh.send(0, communicator);
  }

  return outMesh;
}

Mesh meshVisibilitySort(const Mesh& mesh,
                        const glm::mat4& modelview,
                        const glm::mat4& projection) {
  // Set up array of transformed centroids.
  using DistIndex = std::pair<float, int>;
  std::vector<DistIndex> triList;
  triList.reserve(mesh.getNumberOfTriangles());

  glm::mat4 fullTransform = projection * modelview;

  for (int triIndex = 0; triIndex < mesh.getNumberOfTriangles(); ++triIndex) {
    Triangle triangle = mesh.getTriangle(triIndex);

    std::array<float, 3> vertexDists;
    for (int vertI = 0; vertI < 3; ++vertI) {
      glm::vec4 transformedVert =
          fullTransform * glm::vec4(triangle.vertex[vertI], 1.0f);
      vertexDists[vertI] = transformedVert.z / transformedVert.w;
      // Make sure closest vert is in index 0
      if (vertexDists[0] > vertexDists[vertI]) {
        std::swap(vertexDists[0], vertexDists[vertI]);
      }
    }

    // Weight the point to sort by toward the closest vertex. Generally, the
    // closest vertex tends to be a better indicator of visibility depth than
    // centroid, but use the other two vertices to break ties in polygons that
    // share vertices.
    triList.push_back(DistIndex(
        0.9f * vertexDists[0] + 0.05f * (vertexDists[1] + vertexDists[2]),
        triIndex));
  }

  // Sort the array with a compare function that causes the array to be sorted
  // by the distance to the viewer with the largest distance first.
  std::sort(triList.begin(),
            triList.end(),
            [](const DistIndex& a, const DistIndex& b) -> bool {
              return (a.first > b.first);
            });

  // Construct a new mesh with the sorted order.
  Mesh sortedMesh(mesh.getNumberOfVertices(), triList.size());
  std::copy(mesh.getPointCoordinatesBuffer(0),
            mesh.getPointCoordinatesBuffer(mesh.getNumberOfVertices()),
            sortedMesh.getPointCoordinatesBuffer(0));

  for (int outputTriIndex = 0; outputTriIndex < triList.size();
       ++outputTriIndex) {
    int inputTriIndex = triList[outputTriIndex].second;
    std::copy(mesh.getTriangleConnectionsBuffer(inputTriIndex),
              mesh.getTriangleConnectionsBuffer(inputTriIndex + 1),
              sortedMesh.getTriangleConnectionsBuffer(outputTriIndex));
    std::copy(mesh.getTriangleNormalsBuffer(inputTriIndex),
              mesh.getTriangleNormalsBuffer(inputTriIndex + 1),
              sortedMesh.getTriangleNormalsBuffer(outputTriIndex));
    std::copy(mesh.getTriangleColorsBuffer(inputTriIndex),
              mesh.getTriangleColorsBuffer(inputTriIndex + 1),
              sortedMesh.getTriangleColorsBuffer(outputTriIndex));
  }

  return sortedMesh;
}
