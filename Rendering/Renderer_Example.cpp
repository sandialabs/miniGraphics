// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (column) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "Renderer_Example.hpp"
#include <math.h>
#include <algorithm>


inline void Renderer_Example::fillLine(Image *image,
                                       int y,
                                       const glm::vec3 &edgeDir1,
                                       const glm::vec3 &edgeBase1,
                                       const glm::vec3 &edgeDir2,
                                       const glm::vec3 &edgeBase2,
                                       const Color &color) {
  float interp1 = ((float)y - edgeBase1.y) / edgeDir1.y;
  float interp2 = ((float)y - edgeBase2.y) / edgeDir2.y;

  glm::vec3 left;
  glm::vec3 right;
  left = edgeBase1 + interp1 * edgeDir1;
  right = edgeBase2 + interp2 * edgeDir2;
  if (right.x < left.x) {
    std::swap(left, right);
  }

  int xMin = std::max((int)left.x, 0);
  int xMax = std::min((int)right.x, image->getWidth());

  float deltaDepth = (right.z - left.z) / (right.x - left.x);
  float depth = left.z + deltaDepth * (xMin - left.x);
  for (int x = xMin; x < xMax; ++x) {
    if (depth < image->getDepth(x, y)) {
      image->setColor(x, y, color);
      image->setDepth(x, y, depth);
      depth += deltaDepth;
    }
  }
}

void Renderer_Example::fillTriangle(Image *image, const Triangle &triangle) {
  const Color &color = triangle.color;

  // Sort vertices by location along Y axis.
  glm::vec3 vMin = triangle.vertex[0];
  glm::vec3 vMid = triangle.vertex[1];
  glm::vec3 vMax = triangle.vertex[2];

  std::cout << vMin.x << " " << vMin.y << " " << vMin.z << std::endl;
  std::cout << vMid.x << " " << vMid.y << " " << vMid.z << std::endl;
  std::cout << vMax.x << " " << vMax.y << " " << vMax.z << std::endl;

  if (vMin.y > vMid.y) {
    std::swap(vMin, vMid);
  }
  if (vMin.y > vMax.y) {
    std::swap(vMin, vMax);
  }
  if (vMid.y > vMax.y) {
    std::swap(vMid, vMax);
  }

  // Get vectors along edge point from min to max.
  glm::vec3 dirMin2Max = vMax - vMin;
  glm::vec3 dirMin2Mid = vMid - vMin;
  glm::vec3 dirMid2Max = vMax - vMid;

  int yMin = std::max((int)vMin.y, 0);
  int yMid = std::max((int)vMid.y, 0);
  int yMax = std::min((int)vMax.y, image->getHeight());

  // Rasterize bottom half
  for (int y = yMin; y < yMid; ++y) {
    this->fillLine(image, y, dirMin2Max, vMin, dirMin2Mid, vMin, color);
  }

  // Rasterize top half
  for (int y = yMid; y < yMax; ++y) {
    this->fillLine(image, y, dirMin2Max, vMin, dirMid2Max, vMax, color);
  }
}

void Renderer_Example::render(const Mesh &mesh, Image *image) {
  // TODO: depth resolution is broken. When fixed, changed this back to the
  // defaults.
  image->clear(Color(0, 0, 0), 100.0f);

  for (int i = 0; i < mesh.getNumberOfTriangles(); i++) {
    this->fillTriangle(image, mesh.getTriangle(i));
  }
}
