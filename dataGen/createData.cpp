// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <fstream>
#include <iostream>

using namespace std;

double num_dims = 3;
double range_min = 0;
double range_max = 1.0;

int resolution = 100;

int num_triangles = 10;

double fRand(double fMin, double fMax) {
  double f = (double)rand() / RAND_MAX;
  return fMin + f * (fMax - fMin);
}

void createVertices(ofstream& myfile) {
  myfile << "#Vertices" << endl;
  for (int i = 0; i < num_triangles * 3; i++) {
    double x = fRand(range_min, range_max);
    double y = fRand(range_min, range_max);
    double z = fRand(range_min, range_max);

    myfile << x << "," << y << "," << z << endl;
  }
}

void createTriangles(ofstream& myfile) {
  myfile << "#Triangles" << endl;
  for (int i = 0; i < num_triangles; i++) {
    double t1 = 0 + i * 3;
    double t2 = 1 + i * 3;
    double t3 = 2 + i * 3;

    myfile << t1 << "," << t2 << "," << t3 << endl;
  }
}

void createColors(ofstream& myfile) {
  myfile << "#Colors" << endl;
  for (int i = 0; i < num_triangles; i++) {
    double r = rand() % 256;
    double g = rand() % 256;
    double b = rand() % 256;

    myfile << i << "," << r << "," << g << "," << b << endl;
  }
}

int main(int argv, char* argc[]) {
  srand(static_cast<unsigned int>(time(0)));
  ofstream myfile("triangles.dat");
  if (myfile.is_open()) {
    myfile << "#Resolution" << endl;
    myfile << resolution << "," << resolution << "," << resolution << endl;
    createVertices(myfile);
    createTriangles(myfile);
    createColors(myfile);
    myfile.close();
  }
  return 0;
}
