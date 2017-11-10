// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "ReadData.hpp"

#include <stdlib.h>

#include <fstream>
#include <iostream>

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

void floatSplit(char line[], float result[], int size) {
  char *token = strtok(line, ",");
  int index = 0;
  while (token != NULL && index < size) {
    float temp = atof(token);
    result[index] = temp;
    index++;
    token = strtok(NULL, ",");
  }
}

bool readData(const std::string &filename,
              Mesh &mesh,
              int &imageWidth,
              int &imageHeight) {
  std::string line;
  std::ifstream myfile(filename);
  if (!myfile.is_open()) {
    return false;
  }

  float max_point[3];
  char curr = 'z';
  while (getline(myfile, line)) {
    if (line[0] == '#') {
      if (line == "#Resolution") {
        curr = 'r';
      } else if (line == "#Vertices") {
        curr = 'v';
      } else if (line == "#Triangles") {
        curr = 't';
      } else if (line == "#Colors") {
        curr = 'c';
      } else if (line == "#Opacity") {
        curr = 'o';
      } else {
        std::cerr << "Unexpected section: " << line << std::endl;
      }
      continue;
    }

    if (curr == 'r') {
      floatSplit(const_cast<char *>(line.c_str()), max_point, 3);
      // Odd ordering of x, y, z
      imageWidth = (int)max_point[2];
      imageHeight = (int)max_point[1];
    } else if (curr == 'v') {
      float temp_v[3];
      floatSplit(const_cast<char *>(line.c_str()), temp_v, 3);

      // Correct weird z, y, x order
      glm::vec3 vertex(temp_v[2], temp_v[1], temp_v[0]);

      mesh.addVertex(vertex);
    } else if (curr == 't') {
      int temp_t[3];
      intSplit(const_cast<char *>(line.c_str()), temp_t, 3);

      mesh.addTriangle(temp_t);
    } else if (curr == 'c') {
      int *temp_c = new int[4];
      intSplit(const_cast<char *>(line.c_str()), temp_c, 4);

      mesh.setColor(temp_c[0], Color(temp_c[1], temp_c[2], temp_c[3]));
    } else if (curr == 'o') {
      float *temp_o = new float[2];
      floatSplit(const_cast<char *>(line.c_str()), temp_o, 2);

      int triangleIndex = (int)temp_o[0];
      Color color = mesh.getTriangle(triangleIndex).color;
      color.Components[3] = temp_o[1];
      mesh.setColor(triangleIndex, color);
    }
  }
  myfile.close();

  return true;
}
