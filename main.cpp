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

#include <memory>
#include <vector>

#include "Composition/Composition_Example.hpp"
//#include "Rendering/OpenGL_Example.hpp"
#include "Rendering/Renderer_Example.hpp"
//#include "Composition/IceT_Example.hpp"
#include "IO/ReadData.hpp"
#include "Objects/ImageRGBAUByteColorFloatDepth.hpp"

int max(int a, int b) {
  if (a > b) return a;
  return b;
}

template <class R_T, class C_T>
void run(R_T R, C_T C, vector<Triangle> triangles, int* resolution) {
  // INITIALIZE IMAGES SPACES
  int numImages = 2;
  std::vector<std::shared_ptr<Image>> images;

  // RENDER SECTION
  clock_t r_begin = clock();

  // TODO: Do renderers in parallel

  for (int imageIndex = 0; imageIndex < numImages; imageIndex++) {
    // INITIALIZE RENDER SPACE
    // Yeah, I don't understand resolution. Indices 1 and 2 appear to be the
    // y and x resolution of the image. Index 0 is some bizarre scaling of the
    // depth.
    std::shared_ptr<Image> image;
    image.reset(
        new ImageRGBAUByteColorFloatDepth(resolution[2], resolution[1]));

    std::vector<Triangle> tempTriangles(
        triangles.begin() + imageIndex * (triangles.size() / numImages),
        triangles.begin() + (imageIndex + 1) * (triangles.size() / numImages));

    R.render(tempTriangles, image.get());

    images.push_back(image);
  }
  clock_t r_end = clock();
  double r_time_spent = (double)(r_end - r_begin) / CLOCKS_PER_SEC;
  cout << "RENDER: " << r_time_spent << " seconds" << endl;

  // PRINT FOR SANITY CHECK
  for (int d = 0; d < numImages; d++) {
    cout << "IMAGE: " << d << endl;
    for (int j = 0; j < resolution[1]; j++) {
      for (int k = 0; k < resolution[2]; k++) {
        Color color = images[d]->getColor(k, j);
        int max_color =
            max(color.GetComponentAsByte(0),
                max(color.GetComponentAsByte(1), color.GetComponentAsByte(2)));
        cout << "(" << max_color << ')';
      }
      cout << endl;
    }
  }

  // COMPOSITION SECTION
  clock_t c_begin = clock();
  for (int imageIndex = 1; imageIndex < numImages; ++imageIndex) {
    images[0]->blend(images[imageIndex].get());
  }
  clock_t c_end = clock();

  double c_time_spent = (double)(c_end - c_begin) / CLOCKS_PER_SEC;
  printf("COMPOSITION: %f seconds\n", c_time_spent);

  // PRINT FOR SANITY CHECK
  int count2 = 0;
  for (int j = 0; j < resolution[1]; j++) {
    for (int k = 0; k < resolution[2]; k++) {
      Color color = images[0]->getColor(k, j);
      int max_color =
          max(color.GetComponentAsByte(0),
              max(color.GetComponentAsByte(1), color.GetComponentAsByte(2)));
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
