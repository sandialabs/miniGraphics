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
static const float ProcessColors[][3] = {{0.5529f, 0.8274f, 0.7803f},
                                         {1.0000f, 1.0000f, 0.7019f},
                                         {0.7450f, 0.7294f, 0.8549f},
                                         {0.9843f, 0.5019f, 0.4470f},
                                         {0.5019f, 0.6941f, 0.8274f},
                                         {0.9921f, 0.7058f, 0.3843f},
                                         {0.7019f, 0.8705f, 0.4117f},
                                         {0.9882f, 0.8039f, 0.8980f},
                                         {0.8509f, 0.8509f, 0.8509f},
                                         {0.7372f, 0.5019f, 0.7411f},
                                         {0.8000f, 0.9215f, 0.7725f},
                                         {1.0000f, 0.9294f, 0.4352f}

};
static const int NumProcessColors = sizeof(ProcessColors) / (3 * sizeof(float));

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

enum PlanePosition { OVER_PLANE, UNDER_PLANE, IN_PLANE };

// Determines the position of a point relative to the plane of a triangle.
static PlanePosition pointInPlane(const glm::vec3& p,
                                  const Triangle& triangle) {
  float elevation = glm::dot(triangle.normal, p - triangle.vertex[0]);
  if (elevation > 0.001) {
    return OVER_PLANE;
  } else if (elevation < -0.001) {
    return UNDER_PLANE;
  } else {
    return IN_PLANE;
  }
}

// Determines if all the points in the first triangle are over or under the
// second triangle's plane. If the plane intersects the triangle, IN_PLANE is
// returned.
static PlanePosition triInPlane(const Triangle& targetTriangle,
                                const Triangle& planeTriangle) {
  PlanePosition positions[3];
  positions[0] = pointInPlane(targetTriangle.vertex[0], planeTriangle);
  positions[1] = pointInPlane(targetTriangle.vertex[1], planeTriangle);
  positions[2] = pointInPlane(targetTriangle.vertex[2], planeTriangle);

  switch (positions[0]) {
    case OVER_PLANE:
      if ((positions[1] == UNDER_PLANE) || (positions[2] == UNDER_PLANE)) {
        return IN_PLANE;
      } else {
        return OVER_PLANE;
      }
    case UNDER_PLANE:
      if ((positions[1] == OVER_PLANE) || (positions[2] == OVER_PLANE)) {
        return IN_PLANE;
      } else {
        return UNDER_PLANE;
      }
    case IN_PLANE:
      switch (positions[1]) {
        case OVER_PLANE:
          if (positions[2] == UNDER_PLANE) {
            return IN_PLANE;
          } else {
            return OVER_PLANE;
          }
        case UNDER_PLANE:
          if (positions[2] == OVER_PLANE) {
            return IN_PLANE;
          } else {
            return UNDER_PLANE;
          }
        case IN_PLANE:
          return positions[2];
      }
  }

  assert(0 && "Invalid code path");
  return IN_PLANE;
}

Mesh meshVisibilitySort(const Mesh& mesh, const glm::mat4& modelview) {
  glm::mat3 normalTransform = glm::inverseTranspose(glm::mat3(modelview));

  // Set up array of transformed triangles.
  using TriIndex = std::pair<Triangle, int>;
  std::vector<TriIndex> triList;
  triList.reserve(mesh.getNumberOfTriangles());

  for (int triIndex = 0; triIndex < mesh.getNumberOfTriangles(); ++triIndex) {
    Triangle originalTri = mesh.getTriangle(triIndex);
    Triangle transformedTri;
    for (int vertIndex = 0; vertIndex < 3; ++vertIndex) {
      transformedTri.vertex[vertIndex] =
          glm::vec3(modelview * glm::vec4(originalTri.vertex[vertIndex], 1.0f));
    }
    transformedTri.normal =
        glm::normalize(normalTransform * originalTri.normal);

    triList.push_back(TriIndex(transformedTri, triIndex));
  }

  // Sort the array with a compare function that returns true if the first
  // object is on top of (i.e. in front of) the second object.
  std::sort(triList.begin(),
            triList.end(),
            [](const TriIndex& a, const TriIndex& b) -> bool {
              switch (triInPlane(a.first, b.first)) {
                case OVER_PLANE:
                  return true;
                case UNDER_PLANE:
                  return false;
                case IN_PLANE:
                  return (triInPlane(b.first, a.first) == UNDER_PLANE);
                default:
                  assert(0 && "Invalid code path");
                  return false;
              }
            });

  // Construct a new mesh with the sorted order.
  Mesh sortedMesh(mesh.getNumberOfVertices(), mesh.getNumberOfTriangles());
  std::copy(mesh.getPointCoordinatesBuffer(0),
            mesh.getPointCoordinatesBuffer(mesh.getNumberOfVertices()),
            sortedMesh.getPointCoordinatesBuffer(0));

  for (int outputTriIndex = 0; outputTriIndex < mesh.getNumberOfTriangles();
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
