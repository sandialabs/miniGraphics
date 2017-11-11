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

#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vector_relational.hpp>

#include <glm/gtc/matrix_transform.hpp>

static glm::mat4x4 identityTransform() { return glm::mat4x4(1.0f); }

static void print(const glm::vec3& vec) {
  std::cout << vec[0] << "\t" << vec[1] << "\t" << vec[2] << std::endl;
}

static void print(const glm::mat4x4& matrix) {
  std::cout << matrix[0][0] << "\t" << matrix[1][0] << "\t" << matrix[2][0]
            << "\t" << matrix[3][0] << std::endl;
  std::cout << matrix[0][1] << "\t" << matrix[1][1] << "\t" << matrix[2][1]
            << "\t" << matrix[3][1] << std::endl;
  std::cout << matrix[0][2] << "\t" << matrix[1][2] << "\t" << matrix[2][2]
            << "\t" << matrix[3][2] << std::endl;
  std::cout << matrix[0][3] << "\t" << matrix[1][3] << "\t" << matrix[2][3]
            << "\t" << matrix[3][3] << std::endl;
}

template <class R_T, class C_T>
void run(R_T R, C_T C, const Mesh& mesh, int imageWidth, int imageHeight) {
  // INITIALIZE IMAGES SPACES
  int numImages = 2;
  std::vector<std::shared_ptr<Image>> images;

  // SET UP PROJECTION MATRICES
  glm::vec3 boundsMin = mesh.getBoundsMin();
  glm::vec3 boundsMax = mesh.getBoundsMax();
  glm::vec3 width = boundsMax - boundsMin;
  glm::vec3 center = 0.5f * (boundsMax + boundsMin);
  float dist = glm::sqrt(glm::dot(width, width));

  float thetaRotation = 25.0f;
  float phiRotation = 15.0f;
  float zoom = 1.0f;

  glm::mat4x4 modelview = identityTransform();

  // Move to in front of camera.
  modelview = glm::translate(modelview, -glm::vec3(0, 0, 1.5f * dist));

  // Rotate geometry for interesting perspectives.
  modelview =
      glm::rotate(modelview, glm::radians(phiRotation), glm::vec3(1, 0, 0));
  modelview =
      glm::rotate(modelview, glm::radians(thetaRotation), glm::vec3(0, 1, 0));

  // Center geometry at origin.
  modelview = glm::translate(modelview, -center);

  glm::mat4x4 projection =
      glm::perspective(glm::radians(45.0f / zoom),
                       (float)imageWidth / (float)imageHeight,
                       dist / 3,
                       2 * dist);

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

    R.render(tempMesh, image.get(), modelview, projection);

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
