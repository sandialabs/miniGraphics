// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "MeshHelper.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

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
