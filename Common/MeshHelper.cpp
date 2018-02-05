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
    int gridDim = static_cast<int>(ceil(cbrt(numProc)));
    glm::vec3 spacing =
        (1.0f - overlap) * (mesh.getBoundsMax() - mesh.getBoundsMin());

    for (int dest = 1; dest < numProc; ++dest) {
      glm::vec3 gridLocation(dest % gridDim,
                             (dest / gridDim) % gridDim,
                             dest / (gridDim * gridDim));
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

    glm::vec3 originalCentroid =
        0.333f * (triangle.vertex[0] + triangle.vertex[1] + triangle.vertex[2]);

    glm::vec4 transformedCentroid =
        fullTransform * glm::vec4(originalCentroid, 1.0f);

    triList.push_back(
        DistIndex(transformedCentroid.z / transformedCentroid.w, triIndex));
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