// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "ReadData.hpp"

void intSplit(char line[], int result[], int size) {
  char *token = strtok(line, ",");
  int index = 0;
  while (token != NULL && index < size) {
    int temp = atoi(token);
    result[index] = temp;
    index++;
    token = strtok(NULL, ",");
  }
}

void doubleSplit(char line[], double result[], int size) {
  char *token = strtok(line, ",");
  int index = 0;
  while (token != NULL && index < size) {
    double temp = atof(token);
    result[index] = temp;
    index++;
    token = strtok(NULL, ",");
  }
}

vector<Triangle> readData(char s[], int resolution[3]) {
  string line;
  ifstream myfile(s);
  vector<Vertex> *vertices = new vector<Vertex>();
  vector<Triangle> *triangles = new vector<Triangle>();
  vector<Vertex>::iterator iter_v = vertices->begin();
  vector<Triangle>::iterator iter_t = triangles->begin();
  if (myfile.is_open()) {
    double max_point[3];
    char curr = 'z';
    while (getline(myfile, line)) {
      if (line[0] == '#') {
        if (line == "#Resolution")
          curr = 'r';
        else if (line == "#Vertices")
          curr = 'v';
        else if (line == "#Triangles")
          curr = 't';
        else if (line == "#Colors")
          curr = 'c';
        else if (line == "#Opacity")
          curr = 'o';
        continue;
      }

      if (curr == 'r') {
        intSplit(const_cast<char *>(line.c_str()), resolution, 3);
        max_point[0] = double(resolution[0]);
        max_point[1] = double(resolution[1]);
        max_point[2] = double(resolution[2]);
      } else if (curr == 'v') {
        double *temp_v = new double[3];
        doubleSplit(const_cast<char *>(line.c_str()), temp_v, 3);

        Vertex VV = *(new Vertex(temp_v[0] * max_point[0],
                                 temp_v[1] * max_point[1],
                                 temp_v[2] * max_point[2]));

        vertices->push_back(VV);
        delete[] temp_v;
      } else if (curr == 't') {
        int *temp_t = new int[3];
        intSplit(const_cast<char *>(line.c_str()), temp_t, 3);

        Triangle TT = *(new Triangle(vertices->at(temp_t[0]),
                                     vertices->at(temp_t[1]),
                                     vertices->at(temp_t[2])));

        triangles->push_back(TT);
        delete[] temp_t;
      } else if (curr == 'c') {
        int *temp_c = new int[4];
        intSplit(const_cast<char *>(line.c_str()), temp_c, 4);

        triangles->at(temp_c[0]).addColor(temp_c[1], temp_c[2], temp_c[3]);

        delete[] temp_c;
      } else if (curr == 'o') {
        double *temp_o = new double[2];
        doubleSplit(const_cast<char *>(line.c_str()), temp_o, 2);
        triangles->at(temp_o[0]).addOpacity(temp_o[1]);

        delete[] temp_o;
      }
    }
    myfile.close();
  }

  else
    cout << "Unable to open file" << s << endl;

  return *triangles;
}
