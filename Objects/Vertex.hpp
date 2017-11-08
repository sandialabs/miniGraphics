// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef VERTEX_H
#define VERTEX_H

#include <iostream>
using namespace std;

class Vertex {
 public:
  double p1, p2, p3;
  Vertex();
  Vertex(double, double, double);
  Vertex(const Vertex &);
  Vertex(const Vertex &, double, double, double);
  void print();
  void getVector(Vertex *, double[]);
  void getVector(double, double, double, double *);
  double dotProduct(const Vertex *) const;
};

#endif
