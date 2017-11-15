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

#include "miniGraphicsConfig.h"

#include "Rendering/Renderer_Example.hpp"
#include "IO/ReadData.hpp"
#include "IO/SavePPM.hpp"
#include "Objects/ImageRGBAUByteColorFloatDepth.hpp"

#ifdef MINIGRAPHICS_ENABLE_OPENGL
#include "Rendering/OpenGL_Example.hpp"
#endif

#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vector_relational.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include <optionparser.h>

static option::ArgStatus PositiveIntArg(const option::Option& option,
                                        bool messageOnError) {
  if (option.arg != nullptr) {
    int value = atoi(option.arg);
    if (value < 1) {
      if (messageOnError) {
        std::cerr << "Option " << option.name
                  << " requires a positive integer argument. Argument '"
                  << option.arg << "' is not valid." << std::endl;
      }
      return option::ARG_ILLEGAL;
    } else {
      return option::ARG_OK;
    }
  } else {
    if (messageOnError) {
      std::cerr << "Option " << option.name << " requires an integer argument."
                << std::endl;
    }
    return option::ARG_ILLEGAL;
  }
}

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

void run(Renderer* renderer,
         const Mesh& mesh,
         int imageWidth,
         int imageHeight,
         bool writeImages) {
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

    renderer->render(tempMesh, image.get(), modelview, projection);

    images.push_back(image);
  }
  clock_t r_end = clock();
  double r_time_spent = (double)(r_end - r_begin) / CLOCKS_PER_SEC;
  std::cout << "RENDER: " << r_time_spent << " seconds" << std::endl;

  // SAVE FOR SANITY CHECK
  if (writeImages) {
    for (int d = 0; d < numImages; d++) {
      std::stringstream filename;
      filename << "rendered" << d << ".ppm";
      SavePPM(*images[d], filename.str());
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

  // SAVE FOR SANITY CHECK
  if (writeImages) {
    SavePPM(*images[0], "composite.ppm");
  }
}

enum optionIndex { DUMMY, HELP, WIDTH, HEIGHT, WRITE_IMAGE, RENDERER };
enum enableIndex { DISABLE, ENABLE };
enum renderType { SIMPLE_RASTER, OPENGL };

int main(int argc, char* argv[]) {
  std::stringstream usagestringstream("USAGE: ");
  usagestringstream << argv[0] << " [options] <data_file>\n\n";
  usagestringstream << "Options:";

  std::string usagestring = usagestringstream.str();

  std::vector<option::Descriptor> usage;
  // clang-format off
  usage.push_back(
    {DUMMY,       0,             "",  "",      option::Arg::None, usagestring.c_str()});
  usage.push_back(
    {HELP,        0,             "h", "help",   option::Arg::None,
     "  --help, -h             Print this message and exit."});
  usage.push_back(
    {WIDTH,       0,             "",  "width",  PositiveIntArg,
     "  --width=<num>          Set the width of the image (default 1100)."});
  usage.push_back(
    {HEIGHT,      0,             "",  "height", PositiveIntArg,
     "  --height=<num>         Set the height of the image (default 900)."});
  usage.push_back(
    {WRITE_IMAGE, ENABLE,        "",  "enable-write-image", option::Arg::None,
     "  --enable-write-image   Turn on writing of composited image (default)."});
  usage.push_back(
    {WRITE_IMAGE, DISABLE,       "",  "disable-write-image", option::Arg::None,
     "  --disable-write-image  Turn off writing of composited image."});
  usage.push_back(
    {RENDERER,    SIMPLE_RASTER, "",  "render-simple-raster", option::Arg::None,
     "  --render-simple-raster Use simple triangle rasterization when\n"
     "                         rendering (default)."});
#ifdef MINIGRAPHICS_ENABLE_OPENGL
  usage.push_back(
    {RENDERER,    OPENGL,        "",  "render-opengl", option::Arg::None,
     "  --render-opengl        Use OpenGL hardware when rendering."});
#endif
  usage.push_back({0, 0, 0, 0, 0, 0});
  // clang-format on

  int imageWidth = 1100;
  int imageHeight = 900;
  bool writeImages = true;
  std::auto_ptr<Renderer> renderer(new Renderer_Example);

  option::Stats stats(&usage.front(), argc - 1, argv + 1);  // Skip program name
  std::vector<option::Option> options(stats.options_max);
  std::vector<option::Option> buffer(stats.buffer_max);
  option::Parser parse(
      &usage.front(), argc - 1, argv + 1, &options.front(), &buffer.front());

  if (parse.error()) {
    return 1;
  }

  if (options[HELP]) {
    option::printUsage(std::cout, &usage.front());
    return 0;
  }

  if (options[DUMMY]) {
    std::cerr << "Unknown option: " << options[DUMMY].name << std::endl;
    option::printUsage(std::cerr, &usage.front());
    return 1;
  }

  if (options[WIDTH]) {
    imageWidth = atoi(options[WIDTH].arg);
  }

  if (options[HEIGHT]) {
    imageHeight = atoi(options[HEIGHT].arg);
  }

  if (options[WRITE_IMAGE]) {
    writeImages = (options[WRITE_IMAGE].last()->type() == ENABLE);
  }

  if (options[RENDERER]) {
    switch (options[RENDERER].last()->type()) {
      case SIMPLE_RASTER:
        // Renderer initialized to simple raster already.
        break;
#ifdef MINIGRAPHICS_ENABLE_OPENGL
      case OPENGL:
        renderer.reset(new OpenGL_Example);
        break;
#endif
      default:
        std::cerr << "Internal error: bad render option." << std::endl;
        return 1;
    }
  }

  // LOAD TRIANGLES
  std::string filename("TEST_TRIANGLE.dat");
  //  std::string filename("triangles.dat");

  Mesh mesh;
  if (!readData(filename, mesh)) {
    std::cerr << "Could not read triangles" << std::endl;
    return 1;
  }

  run(renderer.get(), mesh, imageWidth, imageHeight, writeImages);

  return 0;
}
