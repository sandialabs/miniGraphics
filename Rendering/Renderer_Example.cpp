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

void Renderer_Example::calcPlane(double* D,
                                 double* R,
                                 double* C,
                                 double* B,
                                 const Vertex* v1,
                                 const Vertex* v2,
                                 const Vertex* v3) {
  double v12[3] = {v2->p1 - v1->p1, v2->p2 - v1->p2, v2->p3 - v1->p3};
  double v13[3] = {v3->p1 - v1->p1, v3->p2 - v1->p2, v3->p3 - v1->p3};

  *D = v12[1] * v13[2] - v13[1] * v12[2];
  *R = -1 * (v12[0] * v13[2] - v13[0] * v12[2]);
  *C = v12[0] * v13[1] - v13[0] * v12[1];

  *B = *D * v2->p1 + *R * v2->p2 + *C * v2->p3;
}

int Renderer_Example::whichSide(const Vertex* v,
                                double depth,
                                double row,
                                double column,
                                const Vertex* norm) {
  Vertex* temp = new Vertex(depth - v->p1, row - v->p2, column - v->p3);
  double result = norm->dotProduct(temp);
  if (result == 0)
    return 0;
  else if (result < 0)
    return -1;
  else
    return 1;
}

bool Renderer_Example::correctSide(const Vertex* v1,
                                   const Vertex* v2,
                                   const Vertex* v3,
                                   double* D,
                                   double* R,
                                   double* C,
                                   double depth,
                                   double row,
                                   double column) {
  Vertex* vt = new Vertex(*v1, *D, *R, *C);
  double Dt, Rt, Ct, Bt;
  this->calcPlane(&Dt, &Rt, &Ct, &Bt, v1, v2, vt);
  Vertex* norm = new Vertex(Dt, Rt, Ct);

  int result = this->whichSide(vt, depth, row, column, norm);
  if (result == 0) return true;
  for (int i = 0; i <= 1; i++) {
    for (int j = 0; j <= 1; j++) {
      for (int k = 0; k <= 1; k++) {
        if (result ==
            this->whichSide(vt, v3->p1 + i, v3->p2 + j, v3->p3 + k, norm))
          return true;
      }
    }
  }

  return false;
}

bool Renderer_Example::planeThroughCube(const Vertex* vr,
                                        double* D,
                                        double* R,
                                        double* C,
                                        double depth,
                                        double row,
                                        double column) {
  int prev = -100;
  int result = -100;
  Vertex* norm = new Vertex(*D, *R, *C);
  for (int i = 0; i <= 1; i++) {
    for (int j = 0; j <= 1; j++) {
      for (int k = 0; k <= 1; k++) {
        result = this->whichSide(vr, depth + i, row + j, column + k, norm);
        if (i == 0 && j == 0 && k == 0)
          prev = result;
        else if (result == 0 || result != prev)
          return true;
      }
    }
  }
  return false;
}

bool Renderer_Example::isIn(double* D,
                            double* R,
                            double* C,
                            double* B,
                            const Triangle& triangle,
                            int depth,
                            int row,
                            int column) {
  if (this->planeThroughCube(&triangle.v1, D, R, C, depth, row, column)) {
    const Vertex* v1 = &triangle.v1;
    const Vertex* v2 = &triangle.v2;
    const Vertex* v3 = &triangle.v3;
    if (this->correctSide(v1, v2, v3, D, R, C, depth, row, column) &&
        correctSide(v2, v3, v1, D, R, C, depth, row, column) &&
        correctSide(v3, v1, v2, D, R, C, depth, row, column))
      return true;
  }
  return false;
}

int Renderer_Example::tripleMin(double x, double y, double z) {
  return (int)min(min(x, y), min(y, z));
}

int Renderer_Example::tripleMax(double x, double y, double z) {
  return (int)max(max(x, y), max(y, z));
}

void Renderer_Example::fillTriangle(Image* image, const Triangle& triangle) {
  double D, R, C, B;
  this->calcPlane(&D, &R, &C, &B, &triangle.v1, &triangle.v2, &triangle.v3);

  const Vertex* v1 = &triangle.v1;
  const Vertex* v2 = &triangle.v2;
  const Vertex* v3 = &triangle.v3;

  // Bounding Box
  int minDepth = this->tripleMin(v1->p1, v2->p1, v3->p1);
  int maxDepth = this->tripleMax(v1->p1, v2->p1, v3->p1);
  int minRow = this->tripleMin(v1->p2, v2->p2, v3->p2);
  int maxRow = this->tripleMax(v1->p2, v2->p2, v3->p2);
  int minColumn = this->tripleMin(v1->p3, v2->p3, v3->p3);
  int maxColumn = this->tripleMax(v1->p3, v2->p3, v3->p3);

  Color color;
  color.SetComponentFromByte(0, triangle.color[0]);
  color.SetComponentFromByte(1, triangle.color[1]);
  color.SetComponentFromByte(2, triangle.color[2]);

  for (int depth = minDepth; depth <= maxDepth; depth++) {
    for (int row = minRow; row <= maxRow; row++) {
      for (int column = minColumn; column <= maxColumn; column++) {
        if (this->isIn(&D, &R, &C, &B, triangle, depth, row, column)) {
          int index = image->pixelIndex(column, row);
          if (depth < image->getDepth(index)) {
            image->setColor(index, color);
            image->setDepth(index, depth);
          }
        }
      }
    }
  }
}

void Renderer_Example::render(const std::vector<Triangle>& triangles,
                              Image* image) {
  image->clear();

  for (int i = 0; i < triangles.size(); i++) {
    this->fillTriangle(image, triangles.at(i));
  }
}
