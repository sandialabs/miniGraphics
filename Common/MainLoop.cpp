// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "MainLoop.hpp"

#include "miniGraphicsConfig.h"

#include <Common/ImageRGBAFloatColorOnly.hpp>
#include <Common/ImageRGBAUByteColorFloatDepth.hpp>
#include <Common/ImageRGBAUByteColorOnly.hpp>
#include <Common/ImageRGBFloatColorDepth.hpp>
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

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>

#include "mpi.h"

struct GeometryInfo {
  glm::vec3 boundsMin;
  glm::vec3 boundsMax;

  glm::vec3 width;
  glm::vec3 center;
  float distance;

  int numTriangles;

  std::vector<glm::vec3> centroids;

  void collect(const Mesh& mesh, MPI_Comm communicator) {
    this->boundsMin = mesh.getBoundsMin();
    this->boundsMax = mesh.getBoundsMax();
    MPI_Allreduce(MPI_IN_PLACE,
                  glm::value_ptr(this->boundsMin),
                  3,
                  MPI_FLOAT,
                  MPI_MIN,
                  communicator);
    MPI_Allreduce(MPI_IN_PLACE,
                  glm::value_ptr(this->boundsMax),
                  3,
                  MPI_FLOAT,
                  MPI_MAX,
                  communicator);

    this->width = this->boundsMax - this->boundsMin;
    this->center = 0.5f * (this->boundsMax + this->boundsMin);
    this->distance = glm::sqrt(glm::dot(this->width, this->width));

    this->numTriangles = mesh.getNumberOfTriangles();
    MPI_Allreduce(
        MPI_IN_PLACE, &this->numTriangles, 1, MPI_INT, MPI_SUM, communicator);

    int numProc;
    MPI_Comm_size(communicator, &numProc);

    glm::vec3 localCentroid =
        0.5f * (mesh.getBoundsMax() + mesh.getBoundsMin());
    this->centroids.resize(numProc);
    MPI_Allgather(glm::value_ptr(localCentroid),
                  3,
                  MPI_FLOAT,
                  glm::value_ptr(this->centroids.front()),
                  3,
                  MPI_FLOAT,
                  communicator);
  }
};

static void run(Painter* painter,
                Compositor* compositor,
                const Mesh& mesh,
                Image* imageBuffer,
                bool checkImages,
                bool writeImages,
                YamlWriter& yaml) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  int numProc;
  MPI_Comm_size(MPI_COMM_WORLD, &numProc);

  Mesh fullMesh;
  if (checkImages) {
    fullMesh = meshGather(mesh, MPI_COMM_WORLD);
  }

  int imageWidth = imageBuffer->getWidth();
  int imageHeight = imageBuffer->getHeight();

  // Gather rough geometry information
  GeometryInfo geometryInfo;
  geometryInfo.collect(mesh, MPI_COMM_WORLD);

  yaml.AddDictionaryEntry("num-triangles", geometryInfo.numTriangles);

  // Set up projection matrices
  float thetaRotation = 25.0f;
  float phiRotation = 15.0f;
  float zoom = 1.0f;

  glm::mat4 modelview = glm::mat4(1.0f);

  // Move to in front of camera.
  modelview =
      glm::translate(modelview, -glm::vec3(0, 0, 1.5f * geometryInfo.distance));

  // Rotate geometry for interesting perspectives.
  modelview =
      glm::rotate(modelview, glm::radians(phiRotation), glm::vec3(1, 0, 0));
  modelview =
      glm::rotate(modelview, glm::radians(thetaRotation), glm::vec3(0, 1, 0));

  // Center geometry at origin.
  modelview = glm::translate(modelview, -geometryInfo.center);

  glm::mat4 projection =
      glm::perspective(glm::radians(45.0f / zoom),
                       (float)imageWidth / (float)imageHeight,
                       geometryInfo.distance / 2.1f,
                       2 * geometryInfo.distance);

  std::unique_ptr<Image> localImage = imageBuffer->createNew(
      imageWidth, imageHeight, 0, imageWidth * imageHeight);
  std::unique_ptr<Image> compositeImage;
  std::unique_ptr<Image> fullCompositeImage;

  {
    Timer timeTotal(yaml, "total-seconds");

    MPI_Group composeGroup;
    if (localImage->blendIsOrderDependent()) {
      // Determine (approximate) visibility order of process by sorting the
      // depth of the transformed centroids.
      std::vector<std::pair<float, int>> depthList(numProc);
      for (int proc = 0; proc < numProc; proc++) {
        glm::vec4 centroid(geometryInfo.centroids[proc], 1.0f);
        centroid = modelview * centroid;
        centroid = projection * centroid;
        float depth = centroid.z / centroid.w;
        depthList[proc] = std::pair<float, int>(depth, proc);
      }

      std::sort(depthList.begin(),
                depthList.end(),
                [](const std::pair<float, int>& a,
                   const std::pair<float, int>& b) -> bool {
                  return (a.first < b.first);
                });

      std::vector<int> rankOrder;
      rankOrder.reserve(depthList.size());
      for (auto&& depthEntry : depthList) {
        rankOrder.push_back(depthEntry.second);
      }

      MPI_Group globalGroup;
      MPI_Comm_group(MPI_COMM_WORLD, &globalGroup);
      MPI_Group_incl(globalGroup, numProc, &rankOrder.front(), &composeGroup);
      MPI_Group_free(&globalGroup);
    } else {
      MPI_Comm_group(MPI_COMM_WORLD, &composeGroup);
    }

    // Paint SECTION
    {
      Timer timePaint(yaml, "paint-seconds");

      if (localImage->blendIsOrderDependent()) {
        painter->paint(meshVisibilitySort(mesh, modelview, projection),
                       localImage.get(),
                       modelview,
                       projection);
      } else {
        painter->paint(mesh, localImage.get(), modelview, projection);
      }
    }

    // TODO: This barrier should be optional, but is needed for any of the
    // timing below to be useful.
    MPI_Barrier(MPI_COMM_WORLD);

    // COMPOSITION SECTION
    {
      Timer timeCompositePlusCollect(yaml, "composite-seconds");

      compositeImage =
          compositor->compose(localImage.get(), composeGroup, MPI_COMM_WORLD);

      fullCompositeImage = compositeImage->Gather(0, MPI_COMM_WORLD);
    }

    MPI_Group_free(&composeGroup);
  }

  if (checkImages && (rank == 0)) {
    const float COLOR_THRESHOLD = 0.01;
    const float BAD_PIXEL_THRESHOLD = 0.02;

    std::cout << "Checking image validity..." << std::flush;
    if (localImage->blendIsOrderDependent()) {
      painter->paint(meshVisibilitySort(fullMesh, modelview, projection),
                     localImage.get(),
                     modelview,
                     projection);
    } else {
      painter->paint(fullMesh, localImage.get(), modelview, projection);
    }

    int numPixels = localImage->getNumberOfPixels();
    int numBadPixels = 0;
    for (int pixel = 0; pixel < numPixels; ++pixel) {
      Color compositeColor = fullCompositeImage->getColor(pixel);
      Color localColor = localImage->getColor(pixel);
      if ((fabsf(compositeColor.Components[0] - localColor.Components[0]) >
           COLOR_THRESHOLD) ||
          (fabsf(compositeColor.Components[1] - localColor.Components[1]) >
           COLOR_THRESHOLD) ||
          (fabsf(compositeColor.Components[2] - localColor.Components[2]) >
           COLOR_THRESHOLD)) {
        ++numBadPixels;
      }
    }
    std::cout << (100 * numBadPixels) / numPixels << "% bad pixels."
              << std::endl;
    if (numBadPixels > BAD_PIXEL_THRESHOLD * numPixels) {
      std::cout << "Composite image appears bad!" << std::endl;
      SavePPM(*localImage, "reference.ppm");
      SavePPM(*fullCompositeImage, "bad_composite.ppm");
      exit(1);
    }
  }

  if (writeImages && (rank == 0)) {
    //    std::stringstream filename;
    //    filename << "local_painting" << rank << ".ppm";
    //    SavePPM(*localImage, filename.str());

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
  CHECK_IMAGE,
  WRITE_IMAGE,
  PAINTER,
  GEOMETRY,
  DISTRIBUTION,
  OVERLAP,
  COLOR_FORMAT,
  DEPTH_FORMAT
};
enum enableIndex { DISABLE, ENABLE };
enum paintType { SIMPLE_RASTER, OPENGL };
enum geometryType { BOX, STL_FILE };
enum distributionType { DUPLICATE, DIVIDE };
enum colorType { COLOR_UBYTE, COLOR_FLOAT };
enum depthType { DEPTH_FLOAT, DEPTH_NONE };

int MainLoop(int argc,
             char* argv[],
             Compositor* compositor,
             const option::Descriptor* compositorOptions,
             const char* appName) {
  std::vector<option::Descriptor> compositorOptionsVector;

  if (compositorOptions != nullptr) {
    for (const option::Descriptor* compositorOptionsItem = compositorOptions;
         compositorOptionsItem->shortopt != NULL;
         compositorOptionsItem++) {
      compositorOptionsVector.push_back(*compositorOptionsItem);
    }
  }

  return MainLoop(argc, argv, compositor, compositorOptionsVector, appName);
}

int MainLoop(int argc,
             char* argv[],
             Compositor* compositor,
             const std::vector<option::Descriptor>& compositorOptions,
             const char* appName) {
  std::stringstream yamlStream;
  YamlWriter yaml(yamlStream);

  yaml.AddDictionaryEntry("composite-algorithm", appName);

  auto startTime = std::chrono::system_clock::now();
  auto startTime_t = std::chrono::system_clock::to_time_t(startTime);
  yaml.AddDictionaryEntry("start-time",
                          std::put_time(std::localtime(&startTime_t), "%c"));

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
    {CHECK_IMAGE,  ENABLE,        "",  "enable-check-image", option::Arg::None,
     "  --enable-check-image   Turn on checking of composited image. When on,\n"
     "                         rank 0 collects all geometry and also paints\n"
     "                         everything locally. It then compares the local\n"
     "                         and parallel versions, and exits if they are\n"
     "                         different. (Default)"});
  usage.push_back(
    {CHECK_IMAGE,  DISABLE,       "",  "disable-check-image", option::Arg::None,
     "  --disable-check-image  Turn off checking of composited image.\n"});

  usage.push_back(
    {WRITE_IMAGE,  ENABLE,        "",  "enable-write-image", option::Arg::None,
     "  --enable-write-image   Turn on writing of composited image."});
  usage.push_back(
    {WRITE_IMAGE,  DISABLE,       "",  "disable-write-image", option::Arg::None,
     "  --disable-write-image  Turn off writing of composited image. (Default)\n"});

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

  usage.push_back(
    {COLOR_FORMAT, COLOR_UBYTE,   "",  "color-ubyte", option::Arg::None,
     "  --color-ubyte          Store colors in 8-bit channels (Default)."});
  usage.push_back(
    {COLOR_FORMAT, COLOR_FLOAT,   "",  "color-float", option::Arg::None,
     "  --color-float          Store colors in 32-bit float channels."});
  usage.push_back(
    {DEPTH_FORMAT, DEPTH_FLOAT,   "",  "depth-float", option::Arg::None,
     "  --depth-float          Store depth as 32-bit float (Default)."});
  usage.push_back(
    {DEPTH_FORMAT, DEPTH_NONE,    "",  "depth-none", option::Arg::None,
     "  --depth-none           Do not use a depth buffer. This option changes\n"
     "                         the compositing to an alpha blending mode.\n"});
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
  bool checkImages = true;
  bool writeImages = false;
  std::auto_ptr<Painter> painter(new PainterSimple);
  float overlap = -0.05f;
  colorType colorFormat = COLOR_UBYTE;
  depthType depthFormat = DEPTH_FLOAT;

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

  if (options[CHECK_IMAGE]) {
    checkImages = (options[CHECK_IMAGE].last()->type() == ENABLE);
  }

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

  if (options[COLOR_FORMAT]) {
    colorFormat = static_cast<colorType>(options[COLOR_FORMAT].last()->type());
  }
  if (options[DEPTH_FORMAT]) {
    depthFormat = static_cast<depthType>(options[DEPTH_FORMAT].last()->type());
  }
  std::unique_ptr<Image> imageBuffer;
  switch (depthFormat) {
    case DEPTH_FLOAT:
      yaml.AddDictionaryEntry("depth-buffer-format", "float");
      switch (colorFormat) {
        case COLOR_UBYTE:
          yaml.AddDictionaryEntry("color-buffer-format", "byte");
          imageBuffer = std::unique_ptr<Image>(
              new ImageRGBAUByteColorFloatDepth(imageWidth, imageHeight));
          break;
        case COLOR_FLOAT:
          yaml.AddDictionaryEntry("color-buffer-format", "float");
          imageBuffer = std::unique_ptr<Image>(
              new ImageRGBFloatColorDepth(imageWidth, imageHeight));
          break;
      }
      break;
    case DEPTH_NONE:
      yaml.AddDictionaryEntry("depth-buffer-format", "none");
      switch (colorFormat) {
        case COLOR_UBYTE:
          yaml.AddDictionaryEntry("color-buffer-format", "byte");
          imageBuffer = std::unique_ptr<Image>(
              new ImageRGBAUByteColorOnly(imageWidth, imageHeight));
          break;
        case COLOR_FLOAT:
          yaml.AddDictionaryEntry("color-buffer-format", "float");
          imageBuffer = std::unique_ptr<Image>(
              new ImageRGBAFloatColorOnly(imageWidth, imageHeight));
          break;
      }
      break;
  }

  if (imageBuffer->blendIsOrderDependent()) {
    yaml.AddDictionaryEntry("rendering-order-dependent", "yes");
  } else {
    yaml.AddDictionaryEntry("rendering-order-dependent", "no");
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
#if 0
    int ready = 0;
    while (!ready);
#endif
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

  if (imageBuffer->blendIsOrderDependent()) {
    // If blending colors, make all colors transparent.
    int numTri = mesh.getNumberOfTriangles();
    for (float* colorComponentValue = mesh.getTriangleColorsBuffer(0);
         colorComponentValue != mesh.getTriangleColorsBuffer(numTri);
         ++colorComponentValue) {
      *colorComponentValue *= 0.5f;
    }
  }

  run(painter.get(),
      compositor,
      mesh,
      imageBuffer.get(),
      checkImages,
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
