// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "MainLoop.hpp"

#include "miniGraphicsConfig.h"

#include <Common/ImageRGBAUByteColorFloatDepth.hpp>
#include <Common/MakeBox.hpp>
#include <Common/MeshHelper.hpp>
#include <Common/ReadSTL.hpp>
#include <Common/SavePPM.hpp>
#include <Common/Timer.hpp>
#include <Common/YamlWriter.hpp>

#include <Paint/PainterSimple.hpp>
#ifdef MINIGRAPHICS_ENABLE_OPENGL
#include <Paint/PainterOpenGL.hpp>
#endif

#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vector_relational.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifndef MINIGRAPHICS_WIN32
#include <sys/types.h>
#include <unistd.h>
#else
#include <process.h>
#define getpid _getpid
#endif

#include <fstream>
#include <memory>
#include <sstream>

#include "mpi.h"

static void run(Painter* painter,
                Compositor* compositor,
                const Mesh& mesh,
                int imageWidth,
                int imageHeight,
                bool writeImages,
                YamlWriter& yaml) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  int numProc;
  MPI_Comm_size(MPI_COMM_WORLD, &numProc);

  // Gather rough geometry information
  glm::vec3 boundsMin = mesh.getBoundsMin();
  glm::vec3 boundsMax = mesh.getBoundsMax();
  MPI_Allreduce(MPI_IN_PLACE,
                glm::value_ptr(boundsMin),
                3,
                MPI_FLOAT,
                MPI_MIN,
                MPI_COMM_WORLD);
  MPI_Allreduce(MPI_IN_PLACE,
                glm::value_ptr(boundsMax),
                3,
                MPI_FLOAT,
                MPI_MAX,
                MPI_COMM_WORLD);

  glm::vec3 width = boundsMax - boundsMin;
  glm::vec3 center = 0.5f * (boundsMax + boundsMin);
  float dist = glm::sqrt(glm::dot(width, width));

  int numTriangles = mesh.getNumberOfTriangles();
  MPI_Allreduce(
      MPI_IN_PLACE, &numTriangles, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
  yaml.AddDictionaryEntry("num-triangles", numTriangles);

  // Set up projection matrices
  float thetaRotation = 25.0f;
  float phiRotation = 15.0f;
  float zoom = 1.0f;

  glm::mat4 modelview = glm::mat4(1.0f);

  // Move to in front of camera.
  modelview = glm::translate(modelview, -glm::vec3(0, 0, 1.5f * dist));

  // Rotate geometry for interesting perspectives.
  modelview =
      glm::rotate(modelview, glm::radians(phiRotation), glm::vec3(1, 0, 0));
  modelview =
      glm::rotate(modelview, glm::radians(thetaRotation), glm::vec3(0, 1, 0));

  // Center geometry at origin.
  modelview = glm::translate(modelview, -center);

  glm::mat4 projection =
      glm::perspective(glm::radians(45.0f / zoom),
                       (float)imageWidth / (float)imageHeight,
                       dist / 3,
                       2 * dist);

  ImageRGBAUByteColorFloatDepth localImage(imageWidth, imageHeight);
  std::unique_ptr<Image> compositeImage;
  std::unique_ptr<Image> fullCompositeImage;

  {
    Timer timeTotal(yaml, "total-seconds");

    // Paint SECTION
    {
      Timer timePaint(yaml, "paint-seconds");

      painter->paint(mesh, &localImage, modelview, projection);
    }

    // TODO: This barrier should be optional, but is needed for any of the
    // timing below to be useful.
    MPI_Barrier(MPI_COMM_WORLD);

    // COMPOSITION SECTION
    {
      Timer timeCompositePlusCollect(yaml, "composite-seconds");

      compositeImage = compositor->compose(&localImage, MPI_COMM_WORLD);

      fullCompositeImage = compositeImage->Gather(0, MPI_COMM_WORLD);
    }
  }

  // SAVE FOR SANITY CHECK
  if (writeImages) {
    std::stringstream filename;
    filename << "local_painting" << rank << ".ppm";
    SavePPM(localImage, filename.str());

    if (rank == 0) {
      SavePPM(*fullCompositeImage, "composite.ppm");
    }
  }
}

enum optionIndex {
  DUMMY = 100,
  HELP,
  WIDTH,
  HEIGHT,
  YAML_OUTPUT,
  WRITE_IMAGE,
  PAINTER,
  GEOMETRY,
  DISTRIBUTION,
  OVERLAP
};
enum enableIndex { DISABLE, ENABLE };
enum paintType { SIMPLE_RASTER, OPENGL };
enum geometryType { BOX, STL_FILE };
enum distributionType { DUPLICATE, DIVIDE };

int MainLoop(int argc,
             char* argv[],
             Compositor* compositor,
             const option::Descriptor* compositorOptions) {
  std::vector<option::Descriptor> compositorOptionsVector;

  if (compositorOptions != nullptr) {
    for (const option::Descriptor* compositorOptionsItem = compositorOptions;
         compositorOptionsItem->shortopt != NULL;
         compositorOptionsItem++) {
      compositorOptionsVector.push_back(*compositorOptionsItem);
    }
  }

  return MainLoop(argc, argv, compositor, compositorOptionsVector);
}

int MainLoop(int argc,
             char* argv[],
             Compositor* compositor,
             const std::vector<option::Descriptor>& compositorOptions) {
  std::stringstream yamlStream;
  YamlWriter yaml(yamlStream);

  // TODO: Make this tied to the actual compositing algorithm
  yaml.AddDictionaryEntry("composite-algorithm", "binary swap");

  MPI_Init(&argc, &argv);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  int numProc;
  MPI_Comm_size(MPI_COMM_WORLD, &numProc);
  yaml.AddDictionaryEntry("num-processes", numProc);

  std::stringstream usagestringstream;
  usagestringstream << "USAGE: " << argv[0] << " [options]\n\n";
  usagestringstream << "Options:";

  std::string usagestring = usagestringstream.str();

  std::vector<option::Descriptor> usage;
  // clang-format off
  usage.push_back(
    {DUMMY,       0,             "",  "",      option::Arg::None, usagestring.c_str()});
  usage.push_back(
    {HELP,         0,             "h", "help",   option::Arg::None,
     "  --help, -h             Print this message and exit.\n"});

  usage.push_back(
    {WIDTH,        0,             "",  "width",  PositiveIntArg,
     "  --width=<num>          Set the width of the image. (Default 1100)"});
  usage.push_back(
    {HEIGHT,       0,             "",  "height", PositiveIntArg,
     "  --height=<num>         Set the height of the image. (Default 900)\n"});

  usage.push_back(
    {YAML_OUTPUT,  0,             "", "yaml-output", NonemptyStringArg,
     "  --yaml-output=<file>   Specify the filename of the YAML output file\n"
     "                         containing timing information.\n"
     "                         (Default timing.yaml)\n"});

  usage.push_back(
    {WRITE_IMAGE,  ENABLE,        "",  "enable-write-image", option::Arg::None,
     "  --enable-write-image   Turn on writing of composited image. (Default)"});
  usage.push_back(
    {WRITE_IMAGE,  DISABLE,       "",  "disable-write-image", option::Arg::None,
     "  --disable-write-image  Turn off writing of composited image.\n"});

#ifdef MINIGRAPHICS_ENABLE_OPENGL
  usage.push_back(
    {PAINTER,      OPENGL,        "",  "paint-opengl", option::Arg::None,
     "  --paint-opengl         Use OpenGL hardware when painting."});
#endif
  usage.push_back(
    {PAINTER,      SIMPLE_RASTER, "",  "paint-simple-raster", option::Arg::None,
     "  --paint-simple-raster  Use simple triangle rasterization when painting.\n"
     "                         (Default)\n"});

  usage.push_back(
    {GEOMETRY,     BOX,           "",  "box", option::Arg::None,
     "  --box                  Render a box as the geometry. (Default)"});
  usage.push_back(
    {GEOMETRY,     STL_FILE,      "",  "stl-file", NonemptyStringArg,
     "  --stl-file=<file>      Render the geometry in the given STL file.\n"});

  usage.push_back(
    {DISTRIBUTION, DUPLICATE,     "",  "duplicate-geometry", option::Arg::None,
     "  --duplicate-geometry   Duplicates the geometry read or created on each\n"
     "                         process. The data are offset in a 3D grid\n"
     "                         pattern. (Default)"});
  usage.push_back(
    {DISTRIBUTION, DIVIDE,       "",  "divide-geometry", option::Arg::None,
     "  --divide-geometry      Divides the geometry read or created by\n"
     "                         partitioning the triangles among the processes."});
  usage.push_back(
    {OVERLAP,      0     ,       "",  "overlap", FloatArg,
     "  --overlap=<num>        When duplicating geometry, determine how much\n"
     "                         the geometry overlaps neighboring processes.\n"
     "                         A value of 0 makes the geometry flush. A value\n"
     "                         of 1 completely overlaps all geometry. Negative\n"
     "                         values space the geometry appart. Has no effect\n"
     "                         with --divide-geometry option. (Default -0.05)\n"});
  // clang-format on

  for (auto compositorOpt = compositorOptions.begin();
       compositorOpt != compositorOptions.end();
       compositorOpt++) {
    usage.push_back(*compositorOpt);
  }

  usage.push_back({0, 0, 0, 0, 0, 0});

  int imageWidth = 1100;
  int imageHeight = 900;
  std::string yamlFilename("timing.yaml");
  bool writeImages = true;
  std::auto_ptr<Painter> painter(new PainterSimple);
  float overlap = -0.05f;

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

  if (parse.nonOptionsCount() > 0) {
    std::cerr << "Unknown option: " << parse.nonOption(0) << std::endl;
    option::printUsage(std::cerr, &usage.front());
    return 1;
  }

  if (!compositor->setOptions(options, yaml)) {
    option::printUsage(std::cerr, &usage.front());
    return 1;
  }

  if (options[WIDTH]) {
    imageWidth = atoi(options[WIDTH].arg);
  }
  yaml.AddDictionaryEntry("image-width", imageWidth);

  if (options[HEIGHT]) {
    imageHeight = atoi(options[HEIGHT].arg);
  }
  yaml.AddDictionaryEntry("image-height", imageHeight);

  if (options[WRITE_IMAGE]) {
    writeImages = (options[WRITE_IMAGE].last()->type() == ENABLE);
  }

  if (options[PAINTER]) {
    switch (options[PAINTER].last()->type()) {
      case SIMPLE_RASTER:
        // Painter initialized to simple raster already.
        yaml.AddDictionaryEntry("painter", "simple");
        break;
#ifdef MINIGRAPHICS_ENABLE_OPENGL
      case OPENGL:
        painter.reset(new PainterOpenGL);
        yaml.AddDictionaryEntry("painter", "OpenGL");
        break;
#endif
      default:
        std::cerr << "Internal error: bad painter option." << std::endl;
        return 1;
    }
  } else {
    yaml.AddDictionaryEntry("painter", "simple");
  }

  // LOAD TRIANGLES
  Mesh mesh;
  if (rank == 0) {
    if (options[GEOMETRY] && (options[GEOMETRY].last()->type() != BOX)) {
      std::string filename(options[GEOMETRY].last()->arg);
      yaml.AddDictionaryEntry("geometry", filename);
      switch (options[GEOMETRY].last()->type()) {
        case STL_FILE:
          if (!ReadSTL(filename, mesh)) {
            std::cerr << "Error reading file " << filename << std::endl;
            return 1;
          }
          break;
        default:
          std::cerr << "Invalid geometry type?" << std::endl;
          return 1;
      }
    } else {
      yaml.AddDictionaryEntry("geometry", "box");
      MakeBox(mesh);
    }
    std::cout << "Rank 0 on pid " << getpid() << std::endl;
  } else {
    // Other ranks read nothing. Rank 0 distributes geometry.
  }

  if (options[OVERLAP]) {
    overlap = strtof(options[OVERLAP].arg, NULL);
  }

  if (options[DISTRIBUTION] &&
      (options[DISTRIBUTION].last()->type() == DIVIDE)) {
    meshScatter(mesh, MPI_COMM_WORLD);
    yaml.AddDictionaryEntry("geometry-distribution", "divide");
  } else {
    meshBroadcast(mesh, overlap, MPI_COMM_WORLD);
    yaml.AddDictionaryEntry("geometry-distribution", "duplicate");
    yaml.AddDictionaryEntry("geometry-overlap", overlap);
  }

  run(painter.get(),
      compositor,
      mesh,
      imageWidth,
      imageHeight,
      writeImages,
      yaml);

  if (options[YAML_OUTPUT]) {
    yamlFilename = options[YAML_OUTPUT].arg;
  }
  if (rank == 0) {
    std::ofstream yamlFile(yamlFilename);
    yamlFile << yamlStream.str();
  }

  MPI_Finalize();

  return 0;
}


option::ArgStatus PositiveIntArg(const option::Option& option,
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

option::ArgStatus FloatArg(const option::Option& option, bool messageOnError) {
  if (option.arg != nullptr) {
    const char* arg = option.arg;
    char* endarg;
    strtof(arg, &endarg);
    if (endarg == arg) {
      if (messageOnError) {
        std::cerr << "Option " << option.name
                  << " requires a floating point number argument. Argument '"
                  << option.arg << "' is not valid." << std::endl;
      }
      return option::ARG_ILLEGAL;
    } else {
      return option::ARG_OK;
    }
  } else {
    if (messageOnError) {
      std::cerr << "Option " << option.name << " requires a float argument."
                << std::endl;
    }
    return option::ARG_ILLEGAL;
  }
}

option::ArgStatus NonemptyStringArg(const option::Option& option,
                                    bool messageOnError) {
  if ((option.arg != nullptr) && (option.arg[0] != '\0')) {
    return option::ARG_OK;
  } else {
    if (messageOnError) {
      std::cerr << "Option " << option.name << " requires an argument."
                << std::endl;
    }
    return option::ARG_ILLEGAL;
  }
}
