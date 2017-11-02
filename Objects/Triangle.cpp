// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "Triangle.hpp"

Triangle::Triangle() {
  color = new int[3];
  this->addColor(0, 0, 0);
  this->addOpacity(1.0);
}

Triangle::Triangle(const Triangle& other) {
  v1 = *(new Vertex(other.v1));
  v2 = *(new Vertex(other.v2));
  v3 = *(new Vertex(other.v3));
  color = new int[3];
  this->addColor(other.color[0], other.color[1], other.color[2]);
  this->addOpacity(other.opacity);
}

Triangle::Triangle(Vertex v_1, Vertex v_2, Vertex v_3) {
  v1 = *(new Vertex(v_1));
  v2 = *(new Vertex(v_2));
  v3 = *(new Vertex(v_3));
  color = new int[3];
  this->addColor(0, 0, 0);
  this->addOpacity(1.0);
}

void Triangle::addColor(int r, int g, int b) {
  color[0] = r;
  color[1] = g;
  color[2] = b;
}

void Triangle::addOpacity(double o) { opacity = o; }

void Triangle::addFeatures(int r, int g, int b, double o) {
  this->addColor(r, g, b);
  this->addOpacity(o);
}

void Triangle::print() {
  v1.print();
  v2.print();
  v3.print();
  cout << color[0] << ',' << color[1] << ',' << color[2] << ',' << opacity
       << endl;
}
