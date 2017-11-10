// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "Mesh.hpp"

#include <map>

Mesh::Mesh() {
  this->setNumberOfVertices(0);
  this->setNumberOfTriangles(0);
}

Mesh::Mesh(int numVertices, int numTriangles) {
  this->setNumberOfVertices(numVertices);
  this->setNumberOfTriangles(numTriangles);
}

Mesh::~Mesh() {}

void Mesh::updateBounds() const {
  if (this->boundsValid) {
    return;
  }

  // Technically, this changes the state, but logically the state is the same,
  // so we consider this OK.
  Mesh *mutableMesh = const_cast<Mesh *>(this);
  mutableMesh->computeBounds();
}

void Mesh::computeBounds() {
  assert(!this->boundsValid);

  this->boundsMin = glm::vec3(std::numeric_limits<float>::max(),
                              std::numeric_limits<float>::max(),
                              std::numeric_limits<float>::max());
  this->boundsMax = glm::vec3(std::numeric_limits<float>::min(),
                              std::numeric_limits<float>::min(),
                              std::numeric_limits<float>::min());

  int numVerts = this->getNumberOfVertices();
  for (int vertexIndex = 0; vertexIndex < numVerts; ++vertexIndex) {
    glm::vec3 p = this->getPointCoordinates(vertexIndex);

    this->boundsMin.x = std::min(this->boundsMin.x, p.x);
    this->boundsMin.y = std::min(this->boundsMin.y, p.y);
    this->boundsMin.z = std::min(this->boundsMin.z, p.z);

    this->boundsMax.x = std::max(this->boundsMax.x, p.x);
    this->boundsMax.y = std::max(this->boundsMax.y, p.y);
    this->boundsMax.z = std::max(this->boundsMax.z, p.z);
  }
}

void Mesh::setNumberOfVertices(int numVertices) {
  this->numberOfVertices = numVertices;
  this->pointCoordinates.resize(3 * numVertices);
  this->boundsValid = false;
}

void Mesh::setNumberOfTriangles(int numTriangles) {
  this->numberOfTriangles = numTriangles;
  this->triangleConnections.resize(3 * numTriangles);
  this->triangleColors.resize(4 * numTriangles);
  this->boundsValid = false;
}

inline glm::vec3 Mesh::getPointCoordinates(int vertexIndex) const {
  assert((vertexIndex >= 0) && (vertexIndex) < this->getNumberOfVertices());
  const float *v = this->getPointCoordinatesBuffer(vertexIndex);
  return glm::vec3(v[0], v[1], v[2]);
}

Triangle Mesh::getTriangle(int triangleIndex) const {
  assert((triangleIndex >= 0) &&
         (triangleIndex < this->getNumberOfTriangles()));
  const int *connections = this->getTriangleConnectionsBuffer(triangleIndex);
  const float *color = this->getTriangleColorsBuffer(triangleIndex);
  return Triangle(this->getPointCoordinates(connections[0]),
                  this->getPointCoordinates(connections[1]),
                  this->getPointCoordinates(connections[2]),
                  Color(color));
}

void Mesh::setVertex(int vertexIndex, const glm::vec3 &pointCoordinate) {
  assert((vertexIndex >= 0) && (vertexIndex) < this->getNumberOfVertices());
  float *v = this->getPointCoordinatesBuffer(vertexIndex);
  v[0] = pointCoordinate.x;
  v[1] = pointCoordinate.y;
  v[2] = pointCoordinate.z;
  this->boundsValid = false;
}

void Mesh::setTriangle(int triangleIndex,
                       const int vertexIndices[3],
                       const Color &color) {
  assert((triangleIndex >= 0) &&
         (triangleIndex < this->getNumberOfTriangles()));
  int *connections = this->getTriangleConnectionsBuffer(triangleIndex);
  connections[0] = vertexIndices[0];
  connections[1] = vertexIndices[1];
  connections[2] = vertexIndices[2];

  this->setColor(triangleIndex, color);
}

void Mesh::setColor(int triangleIndex, const Color &color) {
  assert((triangleIndex >= 0) &&
         (triangleIndex < this->getNumberOfTriangles()));

  float *c = this->getTriangleColorsBuffer(triangleIndex);
  c[0] = color.Components[0];
  c[1] = color.Components[1];
  c[2] = color.Components[2];
  c[3] = color.Components[3];
}

void Mesh::addVertex(const glm::vec3 &pointCoordinate) {
  this->pointCoordinates.push_back(pointCoordinate.x);
  this->pointCoordinates.push_back(pointCoordinate.y);
  this->pointCoordinates.push_back(pointCoordinate.z);
  ++this->numberOfVertices;
  this->boundsValid = false;
}

void Mesh::addTriangle(const int vertexIndices[3], const Color &color) {
  this->triangleConnections.push_back(vertexIndices[0]);
  this->triangleConnections.push_back(vertexIndices[1]);
  this->triangleConnections.push_back(vertexIndices[2]);

  this->triangleColors.push_back(color.Components[0]);
  this->triangleColors.push_back(color.Components[1]);
  this->triangleColors.push_back(color.Components[2]);
  this->triangleColors.push_back(color.Components[3]);

  ++this->numberOfTriangles;
}

Mesh Mesh::copySubset(int beginTriangleIndex, int endTriangleIndex) const {
  assert(beginTriangleIndex <= endTriangleIndex);
  int numTrianglesToCopy = endTriangleIndex - beginTriangleIndex;

  // Figure out which vertices are used by the triangle subset and build a map
  // from the original vertices to a new compact set.
  std::map<int, int> vertexMapIn2Out;
  std::vector<int> vertexMapOut2In;
  int numVerticesToCopy = 0;
  for (const int *vertexIter =
           this->getTriangleConnectionsBuffer(beginTriangleIndex);
       vertexIter < this->getTriangleConnectionsBuffer(endTriangleIndex);
       ++vertexIter) {
    if (vertexMapIn2Out.find(*vertexIter) == vertexMapIn2Out.end()) {
      // Current vertex not yet found. Add it to the map.
      vertexMapIn2Out[*vertexIter] = numVerticesToCopy;
      vertexMapOut2In.push_back(*vertexIter);
      ++numVerticesToCopy;
    }
  }

  Mesh outputMesh(numVerticesToCopy, numTrianglesToCopy);

  // Copy over vertices that were found.
  for (int outVertexIndex = 0; outVertexIndex < numVerticesToCopy;
       ++outVertexIndex) {
    outputMesh.setVertex(
        outVertexIndex,
        this->getPointCoordinates(vertexMapOut2In[outVertexIndex]));
  }

  // Copy over triangle connections while mapping vertex indices.
  int *outVertexIter = outputMesh.getTriangleConnectionsBuffer();
  for (const int *inVertexIter =
           this->getTriangleConnectionsBuffer(beginTriangleIndex);
       inVertexIter < this->getTriangleConnectionsBuffer(endTriangleIndex);
       ++inVertexIter) {
    *outVertexIter = vertexMapIn2Out[*inVertexIter];
    ++outVertexIter;
  }

  // Copy over triangle colors (no mapping needed).
  std::copy(this->getTriangleColorsBuffer(beginTriangleIndex),
            this->getTriangleColorsBuffer(endTriangleIndex),
            outputMesh.getTriangleColorsBuffer());

  return outputMesh;
}

const glm::vec3 &Mesh::getBoundsMin() const {
  this->updateBounds();
  return this->boundsMin;
}

const glm::vec3 &Mesh::getBoundsMax() const {
  this->updateBounds();
  return this->boundsMax;
}
