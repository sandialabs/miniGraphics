// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

// Include standard headers
#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#include "Composition/Composition_Example.hpp"
//#include "Rendering/OpenGL_Example.hpp"
#include "Rendering/Renderer_Example.hpp"
//#include "Composition/IceT_Example.hpp"
#include "IO/ReadData.hpp"

using namespace std;

int max(int a, int b) {
  if (a > b) return a;
  return b;
}

template <class R_T, class C_T>
void run(R_T R, C_T C, vector<Triangle> triangles, int* resolution) {
  dynamic_cast<Renderer*>(&R);
  dynamic_cast<Composition*>(&C);

  int init_red = 0;
  int init_green = 0;
  int init_blue = 0;
  int init_depth = resolution[0] + 3.0f;

  // INITIALIZE IMAGES SPACES
  int num_ims = 2;
  int** im_red = new int*[num_ims];
  int** im_green = new int*[num_ims];
  int** im_blue = new int*[num_ims];
  float** im_depth = new float*[num_ims];

  // RENDER SECTION
  clock_t r_begin = clock();

  // TODO: Do renderers in parallel

  int r_size = resolution[1] * resolution[2];
  for (int im = 0; im < num_ims; im++) {
    // INITIALIZE RENDER SPACE
    int* s_red = new int[r_size];
    int* s_green = new int[r_size];
    int* s_blue = new int[r_size];
    float* s_depth = new float[r_size];
    for (int i = 0; i < r_size; i++) {
      s_red[i] = init_red;
      s_green[i] = init_green;
      s_blue[i] = init_blue;
      s_depth[i] = init_depth;
    }
    vector<Triangle>* temp_ts = new vector<Triangle>(
        triangles.begin() + im * (triangles.size() / num_ims),
        triangles.begin() + (im + 1) * (triangles.size() / num_ims));

    R.render(temp_ts, resolution, s_red, s_green, s_blue, s_depth);

    im_red[im] = s_red;
    im_green[im] = s_green;
    im_blue[im] = s_blue;
    im_depth[im] = s_depth;
  }
  clock_t r_end = clock();
  double r_time_spent = (double)(r_end - r_begin) / CLOCKS_PER_SEC;
  cout << "RENDER: " << r_time_spent << " seconds" << endl;

  // PRINT FOR SANITY CHECK
  for (int d = 0; d < num_ims; d++) {
    int count = 0;
    cout << "IMAGE: " << d << endl;
    for (int j = 0; j < resolution[1]; j++) {
      for (int k = 0; k < resolution[2]; k++) {
        int max_color =
            max(im_red[d][count], max(im_green[d][count], im_blue[d][count]));
        cout << "(" << max_color << ')';
        //				cout << " (" << im_red[d][count] << ','
        //<<
        // im_green[d][count] << ',' << im_blue[d][count] << ',' <<
        // im_depth[d][count] << ')';
        count++;
      }
      cout << endl;
    }
  }

  // INITIALIZE COMPOSITION SPACE
  int c_size = resolution[1] * resolution[2];
  int* c_red = new int[c_size];
  int* c_green = new int[c_size];
  int* c_blue = new int[c_size];
  float* c_depth = new float[c_size];
  for (int i = 0; i < c_size; i++) {
    c_red[i] = init_blue;
    c_green[i] = init_green;
    c_blue[i] = init_blue;
    c_depth[i] = init_depth;
  }

  // COMPOSITION SECTION
  clock_t c_begin = clock();
  C.composition(num_ims,
                c_size,
                im_red,
                im_green,
                im_blue,
                im_depth,
                c_red,
                c_green,
                c_blue,
                c_depth);
  clock_t c_end = clock();

  double c_time_spent = (double)(c_end - c_begin) / CLOCKS_PER_SEC;
  printf("COMPOSITION: %f seconds\n", c_time_spent);

  // PRINT FOR SANITY CHECK
  int count2 = 0;
  for (int j = 0; j < resolution[1]; j++) {
    for (int k = 0; k < resolution[2]; k++) {
      int max_color = max(c_red[count2], max(c_green[count2], c_blue[count2]));
      cout << "(" << max_color << ')';

      //			cout << " (" << c_red[count2] << ',' <<
      // c_green[count2] << ',' << c_blue[count2] << ')';
      count2++;
    }
    cout << endl;
  }

  // (OPTIONAL) DISPLAY IMAGE SECTION
  // TODO: set up framework
}

int main(int argv, char* argc[]) {
  // LOAD TRIANGLES
  char s[] = "TEST_TRIANGLE.dat";
  //    char s[] = "triangles.dat";
  int resolution[3] = {0, 0, 0};

  vector<Triangle> triangles = readData(s, resolution);
  if (triangles.size() < 1) {
    cerr << "Could not read triangles" << endl;
    return 1;
  }

  // INITIALIZE RENDER
  Renderer_Example R;
  //  OpenGL_Example R;

  // INITIALIZE COMPOSITION
  Composition_Example C;
  //    IceT_Example C;

  run(R, C, triangles, resolution);

  return 0;
}
