// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef RENDERER_EXAMPLE_H
#define RENDERER_EXAMPLE_H

#include <iostream>
#include "../Objects/Renderer.hpp"

class Renderer_Example : public Renderer {
 private:
  void calcPlane(double *,
                 double *,
                 double *,
                 double *,
                 const Vertex *,
                 const Vertex *,
                 const Vertex *);
  int whichSide(const Vertex *, double, double, double, const Vertex *);
  bool correctSide(const Vertex *,
                   const Vertex *,
                   const Vertex *,
                   double *,
                   double *,
                   double *,
                   double,
                   double,
                   double);
  bool planeThroughCube(
      const Vertex *, double *, double *, double *, double, double, double);
  bool isIn(
      double *, double *, double *, double *, const Triangle &, int, int, int);
  int tripleMin(double, double, double);
  int tripleMax(double, double, double);
  void fillTriangle(Image *image, const Triangle &triangle);

 public:
  void render(const std::vector<Triangle> &triangles, Image *image) final;
};

#endif
