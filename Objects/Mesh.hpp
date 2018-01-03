// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef MESH_HPP
#define MESH_HPP

#include <miniGraphicsConfig.h>

#include <vector>

#include "Triangle.hpp"

#include <glm/vec3.hpp>

#include <mpi.h>

class Mesh {
 private:
  std::vector<float> pointCoordinates;   // Three (x,y,z) coordinates per vertex
  std::vector<int> triangleConnections;  // Three indices per triangle
  std::vector<float> triangleNormals;    // Three (x,y,z) normals per vertex
  std::vector<float> triangleColors;  // Four (r,g,b,a) components per triangle

  int numberOfVertices;
  int numberOfTriangles;

  glm::vec3 boundsMin;
  glm::vec3 boundsMax;
  bool boundsValid;

  void updateBounds() const;
  void computeBounds();

  void updateTriangleNormal(int triangleIndex);

  static const int NUM_VERTEX_TAG = 39146;
  static const int NUM_TRIANGLE_TAG = 13644;
  static const int POINT_COORDINATES_TAG = 76418;
  static const int TRIANGLE_CONNECTIONS_TAG = 68934;
  static const int TRIANGLE_NORMALS_TAG = 81498;
  static const int TRIANGLE_COLORS_TAG = 55176;

 public:
  Mesh();
  Mesh(int numVertices, int numTriangles);
  ~Mesh();

  int getNumberOfVertices() const { return this->numberOfVertices; }
  void setNumberOfVertices(int numVertices);

  int getNumberOfTriangles() const { return this->numberOfTriangles; }
  void setNumberOfTriangles(int numTriangles);

  float* getPointCoordinatesBuffer(int vertexIndex = 0) {
    assert((vertexIndex >= 0) && (vertexIndex) <= this->getNumberOfVertices());
    this->boundsValid = false;
    return &this->pointCoordinates.front() + (3 * vertexIndex);
  }
  const float* getPointCoordinatesBuffer(int vertexIndex = 0) const {
    assert((vertexIndex >= 0) && (vertexIndex) <= this->getNumberOfVertices());
    return &this->pointCoordinates.front() + (3 * vertexIndex);
  }

  int* getTriangleConnectionsBuffer(int triangleIndex = 0) {
    assert((triangleIndex >= 0) &&
           (triangleIndex <= this->getNumberOfTriangles()));
    return &this->triangleConnections.front() + (3 * triangleIndex);
  }
  const int* getTriangleConnectionsBuffer(int triangleIndex = 0) const {
    assert((triangleIndex >= 0) &&
           (triangleIndex <= this->getNumberOfTriangles()));
    return &this->triangleConnections.front() + (3 * triangleIndex);
  }

  float* getTriangleNormalsBuffer(int triangleIndex = 0) {
    assert((triangleIndex >= 0) &&
           (triangleIndex <= this->getNumberOfTriangles()));
    return &this->triangleNormals.front() + (3 * triangleIndex);
  }
  const float* getTriangleNormalsBuffer(int triangleIndex = 0) const {
    assert((triangleIndex >= 0) &&
           (triangleIndex <= this->getNumberOfTriangles()));
    return &this->triangleNormals.front() + (3 * triangleIndex);
  }

  float* getTriangleColorsBuffer(int triangleIndex = 0) {
    assert((triangleIndex >= 0) &&
           (triangleIndex <= this->getNumberOfTriangles()));
    return &this->triangleColors.front() + (4 * triangleIndex);
  }
  const float* getTriangleColorsBuffer(int triangleIndex = 0) const {
    assert((triangleIndex >= 0) &&
           (triangleIndex <= this->getNumberOfTriangles()));
    return &this->triangleColors.front() + (4 * triangleIndex);
  }

  glm::vec3 getPointCoordinates(int vertexIndex) const;
  Triangle getTriangle(int triangleIndex) const;

  void setVertex(int vertexIndex, const glm::vec3& pointCoordinate);
  void setTriangle(int triangleIndex,
                   const int vertexIndices[3],
                   const Color& color = Color(1, 1, 1, 1));
  void setTriangle(int triangleIndex,
                   const int vertexIndices[3],
                   const glm::vec3& normal,
                   const Color& color = Color(1, 1, 1, 1));
  void setNormal(int triangleIndex, const glm::vec3& normal);
  void setColor(int triangleIndex, const Color& color);

  void addVertex(const glm::vec3& pointCoordinate);
  void addTriangle(const int vertexIndices[3],
                   const Color& color = Color(1, 1, 1, 1));
  void addTriangle(const int vertexIndices[3],
                   const glm::vec3& normal,
                   const Color& color = Color(1, 1, 1, 1));

  void setHomogeneousColor(const Color& color);

  Mesh copySubset(int beginTriangleIndex, int endTriangleIndex) const;

  const glm::vec3& getBoundsMin() const;
  const glm::vec3& getBoundsMax() const;

  void send(int destRank, MPI_Comm communicator) const;

  void receive(int srcRank, MPI_Comm communicator);
};

#endif  // MESH_HPP
