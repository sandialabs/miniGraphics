// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "Renderer_Example.hpp"
#include <math.h>
#include <algorithm>

void Renderer_Example::calcPlane(double* D, double* R, double* C, double* B,
                                 Vertex* v1, Vertex* v2, Vertex* v3) {
  double v12[3] = {v2->p1 - v1->p1, v2->p2 - v1->p2, v2->p3 - v1->p3};
  double v13[3] = {v3->p1 - v1->p1, v3->p2 - v1->p2, v3->p3 - v1->p3};

  *D = v12[1] * v13[2] - v13[1] * v12[2];
  *R = -1 * (v12[0] * v13[2] - v13[0] * v12[2]);
  *C = v12[0] * v13[1] - v13[0] * v12[1];

  *B = *D * v2->p1 + *R * v2->p2 + *C * v2->p3;
}

int Renderer_Example::whichSide(Vertex* v, double d, double r, double c,
                                Vertex* norm) {
  Vertex* temp = new Vertex(d - v->p1, r - v->p2, c - v->p3);
  double result = norm->dotProduct(temp);
  if (result == 0)
    return 0;
  else if (result < 0)
    return -1;
  else
    return 1;
}

bool Renderer_Example::correctSide(Vertex* v1, Vertex* v2, Vertex* v3,
                                   double* D, double* R, double* C, double d,
                                   double r, double c) {
  Vertex* vt = new Vertex(*v1, *D, *R, *C);
  double Dt, Rt, Ct, Bt;
  this->calcPlane(&Dt, &Rt, &Ct, &Bt, v1, v2, vt);
  Vertex* norm = new Vertex(Dt, Rt, Ct);

  int result = this->whichSide(vt, d, r, c, norm);
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

bool Renderer_Example::planeThroughCube(Vertex* vr, double* D, double* R,
                                        double* C, double d, double r,
                                        double c) {
  int prev = -100;
  int result = -100;
  Vertex* norm = new Vertex(*D, *R, *C);
  for (int i = 0; i <= 1; i++) {
    for (int j = 0; j <= 1; j++) {
      for (int k = 0; k <= 1; k++) {
        result = this->whichSide(vr, d + i, r + j, c + k, norm);
        if (i == 0 && j == 0 && k == 0)
          prev = result;
        else if (result == 0 || result != prev)
          return true;
      }
    }
  }
  return false;
}

bool Renderer_Example::isIn(double* D, double* R, double* C, double* B,
                            Triangle* t, int d, int r, int c) {
  if (this->planeThroughCube(&t->v1, D, R, C, d, r, c)) {
    Vertex* v1 = &t->v1;
    Vertex* v2 = &t->v2;
    Vertex* v3 = &t->v3;
    if (this->correctSide(v1, v2, v3, D, R, C, d, r, c) &&
        correctSide(v2, v3, v1, D, R, C, d, r, c) &&
        correctSide(v3, v1, v2, D, R, C, d, r, c))
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

void Renderer_Example::fillTriangle(int* s_red, int* s_green, int* s_blue,
                                    float* s_depth, Triangle* t, int* res) {
  double D, R, C, B;
  this->calcPlane(&D, &R, &C, &B, &t->v1, &t->v2, &t->v3);

  Vertex* v1 = &t->v1;
  Vertex* v2 = &t->v2;
  Vertex* v3 = &t->v3;

  // Bounding Box
  int mind = this->tripleMin(v1->p1, v2->p1, v3->p1);
  int maxd = this->tripleMax(v1->p1, v2->p1, v3->p1);
  int minr = this->tripleMin(v1->p2, v2->p2, v3->p2);
  int maxr = this->tripleMax(v1->p2, v2->p2, v3->p2);
  int minc = this->tripleMin(v1->p3, v2->p3, v3->p3);
  int maxc = this->tripleMax(v1->p3, v2->p3, v3->p3);

  for (int d = mind; d <= maxd; d++) {
    for (int r = minr; r <= maxr; r++) {
      for (int c = minc; c <= maxc; c++) {
        if (this->isIn(&D, &R, &C, &B, t, d, r, c)) {
          int index = r * res[2] + c;
          if (s_depth[index] > d) {
            s_red[index] = t->color[0];
            s_green[index] = t->color[1];
            s_blue[index] = t->color[2];
            s_depth[index] = d;
          }
        }
      }
    }
  }
}

void Renderer_Example::render(vector<Triangle>* triangles, int* resolution,
                              int* s_red, int* s_green, int* s_blue,
                              float* s_depth) {
  for (int i = 0; i < triangles->size(); i++) {
    Triangle* t = &triangles->at(i);
    this->fillTriangle(s_red, s_green, s_blue, s_depth, t, resolution);
  }
}
