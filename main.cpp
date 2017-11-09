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
#include <sstream>
#include <vector>

#include "Composition/Composition_Example.hpp"
//#include "Rendering/OpenGL_Example.hpp"
#include "Rendering/Renderer_Example.hpp"
//#include "Composition/IceT_Example.hpp"
#include "IO/ReadData.hpp"
#include "IO/SavePPM.hpp"
#include "Objects/ImageRGBAUByteColorFloatDepth.hpp"

int max(int a, int b) {
  if (a > b) return a;
  return b;
}

template <class R_T, class C_T>
void run(R_T R, C_T C, const Mesh& mesh, int imageWidth, int imageHeight) {
  // INITIALIZE IMAGES SPACES
  int numImages = 2;
  std::vector<std::shared_ptr<Image>> images;

  // RENDER SECTION
  clock_t r_begin = clock();

  // TODO: Do renderers in parallel

  for (int imageIndex = 0; imageIndex < numImages; imageIndex++) {
    // INITIALIZE RENDER SPACE
    std::shared_ptr<Image> image;
    image.reset(new ImageRGBAUByteColorFloatDepth(imageWidth, imageHeight));

    Mesh tempMesh = mesh.copySubset(
        imageIndex * (mesh.getNumberOfTriangles() / numImages),
        (imageIndex + 1) * (mesh.getNumberOfTriangles() / numImages));

    R.render(tempMesh, image.get());

    images.push_back(image);
  }
  clock_t r_end = clock();
  double r_time_spent = (double)(r_end - r_begin) / CLOCKS_PER_SEC;
  cout << "RENDER: " << r_time_spent << " seconds" << endl;

  // SAVE FOR SANITY CHECK
  for (int d = 0; d < numImages; d++) {
    std::stringstream filename;
    filename << "rendered" << d << ".ppm";
    SavePPM(*images[d], filename.str());
  }

  // COMPOSITION SECTION
  clock_t c_begin = clock();
  for (int imageIndex = 1; imageIndex < numImages; ++imageIndex) {
    images[0]->blend(images[imageIndex].get());
  }
  clock_t c_end = clock();

  double c_time_spent = (double)(c_end - c_begin) / CLOCKS_PER_SEC;
  printf("COMPOSITION: %f seconds\n", c_time_spent);

  // SAVE FOR SANITY CHECK
  SavePPM(*images[0], "composite.ppm");
}

int main(int argv, char* argc[]) {
  // LOAD TRIANGLES
  std::string filename("TEST_TRIANGLE.dat");
  //  std::string filename("triangles.dat");
  int imageWidth;
  int imageHeight;

  Mesh mesh;
  if (!readData(filename, mesh, imageWidth, imageHeight)) {
    cerr << "Could not read triangles" << endl;
    return 1;
  }

  // INITIALIZE RENDER
  Renderer_Example R;
  //  OpenGL_Example R;

  // INITIALIZE COMPOSITION
  Composition_Example C;
  //    IceT_Example C;

  run(R, C, mesh, imageWidth, imageHeight);

  return 0;
}
