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
#include <random>
#include <sstream>

#include "mpi.h"

enum optionIndex {
  DUMMY = 100,
  HELP,
  WIDTH,
  HEIGHT,
  NUM_TRIALS,
  YAML_OUTPUT,
  CHECK_IMAGE,
  WRITE_IMAGE,
  PAINTER,
  GEOMETRY,
  DISTRIBUTION,
  OVERLAP,
  COLOR_FORMAT,
  DEPTH_FORMAT,
  CAMERA_THETA,
  CAMERA_PHI,
  CAMERA_ZOOM,
  CAMERA_ANIMATE_ROTATE,
  CAMERA_RANDOM_ROTATE,
  CAMERA_ANIMATE_ALL,
  CAMERA_RANDOM_ALL,
  RANDOM_SEED
};
enum enableIndex { DISABLE, ENABLE };
enum paintType { SIMPLE_RASTER, OPENGL };
enum geometryType { BOX, STL_FILE };
enum distributionType { DUPLICATE, DIVIDE };
enum colorType { COLOR_UBYTE, COLOR_FLOAT };
enum depthType { DEPTH_FLOAT, DEPTH_NONE };
enum cameraMoveType { CAMERA_STILL, CAMERA_ANIMATE, CAMERA_RANDOM };

struct RunOptions {
  int imageWidth;
  int imageHeight;
  int numTrials;
  std::string yamlFilename;
  bool checkImage;
  bool writeImage;
  paintType painter;
  geometryType geometry;
  std::string geometryFile;
  distributionType distribution;
  float overlap;
  colorType colorFormat;
  depthType depthFormat;
  float thetaRotation;
  float phiRotation;
  float zoom;
  cameraMoveType thetaMove;
  cameraMoveType phiMove;
  cameraMoveType zoomMove;
  std::mt19937 randomEngine;

  RunOptions()
      : imageWidth(1100),
        imageHeight(900),
        numTrials(10),
        yamlFilename("timing.yaml"),
        checkImage(true),
        writeImage(false),
        painter(SIMPLE_RASTER),
        geometry(BOX),
        distribution(DUPLICATE),
        overlap(-0.05f),
        colorFormat(COLOR_UBYTE),
        depthFormat(DEPTH_FLOAT),
        thetaRotation(25.0f),
        phiRotation(15.0f),
        zoom(1.0f),
        thetaMove(CAMERA_RANDOM),
        phiMove(CAMERA_RANDOM),
        zoomMove(CAMERA_STILL) {}
};

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

static std::unique_ptr<ImageFull> createImage(const RunOptions& runOptions,
                                              YamlWriter& yaml) {
  yaml.AddDictionaryEntry("image-width", runOptions.imageWidth);
  yaml.AddDictionaryEntry("image-height", runOptions.imageHeight);

  switch (runOptions.depthFormat) {
    case DEPTH_FLOAT:
      yaml.AddDictionaryEntry("depth-buffer-format", "float");
      switch (runOptions.colorFormat) {
        case COLOR_UBYTE:
          yaml.AddDictionaryEntry("color-buffer-format", "byte");
          return std::unique_ptr<ImageFull>(new ImageRGBAUByteColorFloatDepth(
              runOptions.imageWidth, runOptions.imageHeight));
        case COLOR_FLOAT:
          yaml.AddDictionaryEntry("color-buffer-format", "float");
          return std::unique_ptr<ImageFull>(new ImageRGBFloatColorDepth(
              runOptions.imageWidth, runOptions.imageHeight));
      }
      break;
    case DEPTH_NONE:
      yaml.AddDictionaryEntry("depth-buffer-format", "none");
      switch (runOptions.colorFormat) {
        case COLOR_UBYTE:
          yaml.AddDictionaryEntry("color-buffer-format", "byte");
          return std::unique_ptr<ImageFull>(new ImageRGBAUByteColorOnly(
              runOptions.imageWidth, runOptions.imageHeight));
          break;
        case COLOR_FLOAT:
          yaml.AddDictionaryEntry("color-buffer-format", "float");
          return std::unique_ptr<ImageFull>(new ImageRGBAFloatColorOnly(
              runOptions.imageWidth, runOptions.imageHeight));
          break;
      }
      break;
  }
  std::cerr << "Internal error: bad buffer format option" << std::endl;
  exit(1);
  return std::unique_ptr<ImageFull>();
}

static std::unique_ptr<Painter> createPainter(const RunOptions& runOptions,
                                              YamlWriter& yaml) {
  switch (runOptions.painter) {
    case SIMPLE_RASTER:
      yaml.AddDictionaryEntry("painter", "simple");
      return std::unique_ptr<Painter>(new PainterSimple);
#ifdef MINIGRAPHICS_ENABLE_OPENGL
    case OPENGL:
      yaml.AddDictionaryEntry("painter", "OpenGL");
      return std::unique_ptr<Painter>(new PainterOpenGL);
#endif
    default:
      std::cerr << "Internal error: bad painter option" << std::endl;
      exit(1);
      return std::unique_ptr<Painter>();
  }
}

static Mesh createMesh(const RunOptions& runOptions,
                       MPI_Comm communicator,
                       YamlWriter& yaml) {
  int rank;
  MPI_Comm_rank(communicator, &rank);

  Mesh mesh;
  if (rank == 0) {
    switch (runOptions.geometry) {
      case BOX:
        yaml.AddDictionaryEntry("geometry", "box");
        MakeBox(mesh);
        break;
      case STL_FILE:
        yaml.AddDictionaryEntry("geometry", runOptions.geometryFile);
        if (!ReadSTL(runOptions.geometryFile, mesh)) {
          std::cerr << "Error reading STL file " << runOptions.geometryFile
                    << std::endl;
          exit(1);
        }
        break;
    }
  } else {
    // Other ranks read nothing. Rank 0 distributes geometry.
  }

  switch (runOptions.distribution) {
    case DUPLICATE:
      yaml.AddDictionaryEntry("geometry-distribution", "duplicate");
      yaml.AddDictionaryEntry("geometry-overlap", runOptions.overlap);
      meshBroadcast(mesh, runOptions.overlap, communicator);
      break;
    case DIVIDE:
      yaml.AddDictionaryEntry("geometry-distribution", "divide");
      meshScatter(mesh, communicator);
      break;
  }

  if (runOptions.depthFormat == DEPTH_NONE) {
    // If blending colors, make all colors transparent.
    int numTri = mesh.getNumberOfTriangles();
    for (float* colorComponentValue = mesh.getTriangleColorsBuffer(0);
         colorComponentValue != mesh.getTriangleColorsBuffer(numTri);
         ++colorComponentValue) {
      *colorComponentValue *= 0.5f;
    }
  }

  return mesh;
}

static void createTransforms(RunOptions& runOptions,
                             int trial,
                             const GeometryInfo& geometryInfo,
                             YamlWriter& yaml,
                             glm::mat4& modelviewOut,
                             glm::mat4& projectionOut) {
  float animationDistance = static_cast<float>(trial) / runOptions.numTrials;

  float thetaRotation = runOptions.thetaRotation;
  switch (runOptions.thetaMove) {
    case CAMERA_STILL:
      break;
    case CAMERA_ANIMATE:
      thetaRotation += 360.0f * animationDistance - 180.0f;
      break;
    case CAMERA_RANDOM:
      thetaRotation = std::uniform_real_distribution<float>(
          -180, 180)(runOptions.randomEngine);
      break;
  }

  float phiRotation = runOptions.phiRotation;
  switch (runOptions.phiMove) {
    case CAMERA_STILL:
      break;
    case CAMERA_ANIMATE:
      phiRotation += 180.0f * animationDistance - 90.0f;
      break;
    case CAMERA_RANDOM:
      phiRotation = std::uniform_real_distribution<float>(
          -180, 180)(runOptions.randomEngine);
      break;
  }

  float zoom = runOptions.zoom;
  switch (runOptions.zoomMove) {
    case CAMERA_STILL:
      break;
    case CAMERA_ANIMATE:
      zoom += 9.0f * animationDistance;
      break;
    case CAMERA_RANDOM:
      zoom =
          std::uniform_real_distribution<float>(1, 10)(runOptions.randomEngine);
      break;
  }

  yaml.AddDictionaryEntry("theta-rotation", thetaRotation);
  yaml.AddDictionaryEntry("phi-rotation", phiRotation);
  yaml.AddDictionaryEntry("zoom", zoom);

  modelviewOut = glm::mat4(1.0f);

  // Move to in front of camera.
  modelviewOut = glm::translate(modelviewOut,
                                -glm::vec3(0, 0, 1.5f * geometryInfo.distance));

  // Rotate geometry for interesting perspectives.
  modelviewOut =
      glm::rotate(modelviewOut, glm::radians(phiRotation), glm::vec3(1, 0, 0));
  modelviewOut = glm::rotate(
      modelviewOut, glm::radians(thetaRotation), glm::vec3(0, 1, 0));

  // Center geometry at origin.
  modelviewOut = glm::translate(modelviewOut, -geometryInfo.center);

  projectionOut = glm::perspective(
      glm::radians(45.0f / zoom),
      (float)runOptions.imageWidth / (float)runOptions.imageHeight,
      geometryInfo.distance / 2.1f,
      2 * geometryInfo.distance);
}

static MPI_Group createComposeGroup(bool blendIsOrderDependent,
                                    const GeometryInfo& geometryInfo,
                                    const glm::mat4& modelview,
                                    const glm::mat4& projection,
                                    MPI_Comm communicator) {
  int numProc;
  MPI_Comm_size(communicator, &numProc);

  MPI_Group globalGroup;
  MPI_Comm_group(communicator, &globalGroup);

  if (blendIsOrderDependent) {
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
              [](const std::pair<float, int>& a, const std::pair<float, int>& b)
                  -> bool { return (a.first < b.first); });

    std::vector<int> rankOrder;
    rankOrder.reserve(depthList.size());
    for (auto&& depthEntry : depthList) {
      rankOrder.push_back(depthEntry.second);
    }

    MPI_Group composeGroup;
    MPI_Group_incl(globalGroup, numProc, &rankOrder.front(), &composeGroup);
    MPI_Group_free(&globalGroup);
    return composeGroup;
  } else {
    // If ordering is not necessary, just return a group for the communicator.
    return globalGroup;
  }
}

static void doLocalPaint(ImageFull& localImage,
                         Painter& painter,
                         const Mesh& mesh,
                         const glm::mat4& modelview,
                         const glm::mat4& projection,
                         YamlWriter& yaml) {
  Timer timePaint(yaml, "paint-seconds");

  if (localImage.blendIsOrderDependent()) {
    painter.paint(meshVisibilitySort(mesh, modelview, projection),
                  localImage,
                  modelview,
                  projection);
  } else {
    painter.paint(mesh, localImage, modelview, projection);
  }
}

static std::unique_ptr<ImageFull> doComposeImage(ImageFull& localImage,
                                                 Compositor& compositor,
                                                 MPI_Group composeGroup,
                                                 MPI_Comm communicator,
                                                 YamlWriter& yaml) {
  Timer timeCompositePlusCollect(yaml, "composite-seconds");

  std::unique_ptr<Image> compositeImage =
      compositor.compose(&localImage, composeGroup, MPI_COMM_WORLD);

  // TODO: Deal with image compression decompression
  std::unique_ptr<ImageFull> uncompressedCompositeImage(
      dynamic_cast<ImageFull*>(compositeImage.release()));

  return uncompressedCompositeImage->Gather(0, MPI_COMM_WORLD);
}

static void checkImage(const ImageFull& fullCompositeImage,
                       ImageFull& localImage,
                       Painter& painter,
                       const Mesh& fullMesh,
                       const glm::mat4& modelview,
                       const glm::mat4& projection) {
  constexpr float COLOR_THRESHOLD = 0.01f;
  constexpr float BAD_PIXEL_THRESHOLD = 0.02f;

  std::stringstream dummyStream;
  YamlWriter dummyYaml(dummyStream);

  std::cout << "Checking image validity..." << std::flush;
  doLocalPaint(localImage, painter, fullMesh, modelview, projection, dummyYaml);

  int numPixels = localImage.getNumberOfPixels();
  int numBadPixels = 0;
  for (int pixel = 0; pixel < numPixels; ++pixel) {
    Color compositeColor = fullCompositeImage.getColor(pixel);
    Color localColor = localImage.getColor(pixel);
    if ((fabsf(compositeColor.Components[0] - localColor.Components[0]) >
         COLOR_THRESHOLD) ||
        (fabsf(compositeColor.Components[1] - localColor.Components[1]) >
         COLOR_THRESHOLD) ||
        (fabsf(compositeColor.Components[2] - localColor.Components[2]) >
         COLOR_THRESHOLD)) {
      ++numBadPixels;
    }
  }
  std::cout << (100 * numBadPixels) / numPixels << "% bad pixels." << std::endl;
  if (numBadPixels > BAD_PIXEL_THRESHOLD * numPixels) {
    std::cout << "Composite image appears bad!" << std::endl;
    SavePPM(localImage, "reference.ppm");
    SavePPM(fullCompositeImage, "bad_composite.ppm");
    exit(1);
  }
}

static void writeImage(const ImageFull& image, int trial) {
  std::stringstream filename;
  filename << "composite" << std::setfill('0') << std::setw(3) << trial
           << ".ppm";
  SavePPM(image, filename.str());
}

static void run(RunOptions& runOptions,
                Compositor* compositor,
                YamlWriter& yaml) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  int numProc;
  MPI_Comm_size(MPI_COMM_WORLD, &numProc);

  std::unique_ptr<ImageFull> localImage = createImage(runOptions, yaml);
  if (localImage->blendIsOrderDependent()) {
    yaml.AddDictionaryEntry("rendering-order-dependent", "yes");
  } else {
    yaml.AddDictionaryEntry("rendering-order-dependent", "no");
  }

  std::unique_ptr<Painter> painter = createPainter(runOptions, yaml);

  Mesh mesh = createMesh(runOptions, MPI_COMM_WORLD, yaml);

  Mesh fullMesh;
  if (runOptions.checkImage) {
    fullMesh = meshGather(mesh, MPI_COMM_WORLD);
  }

  // Gather rough geometry information
  GeometryInfo geometryInfo;
  geometryInfo.collect(mesh, MPI_COMM_WORLD);

  yaml.AddDictionaryEntry("num-triangles", geometryInfo.numTriangles);

  yaml.StartBlock("trials");

  for (int trial = 0; trial < runOptions.numTrials; ++trial) {
    yaml.StartListItem();
    yaml.AddDictionaryEntry("trial-num", trial);

    glm::mat4 modelview;
    glm::mat4 projection;
    createTransforms(
        runOptions, trial, geometryInfo, yaml, modelview, projection);

    std::unique_ptr<ImageFull> fullCompositeImage;

    {
      Timer timeTotal(yaml, "total-seconds");

      MPI_Group composeGroup =
          createComposeGroup(localImage->blendIsOrderDependent(),
                             geometryInfo,
                             modelview,
                             projection,
                             MPI_COMM_WORLD);

      doLocalPaint(*localImage, *painter, mesh, modelview, projection, yaml);

      // TODO: This barrier should be optional, but is needed for any of the
      // timing of the composition to be useful.
      MPI_Barrier(MPI_COMM_WORLD);

      fullCompositeImage = doComposeImage(
          *localImage, *compositor, composeGroup, MPI_COMM_WORLD, yaml);

      MPI_Group_free(&composeGroup);
    }

    if (runOptions.checkImage && (rank == 0)) {
      checkImage(*fullCompositeImage,
                 *localImage,
                 *painter,
                 fullMesh,
                 modelview,
                 projection);
    }

    if (runOptions.writeImage && (rank == 0)) {
      writeImage(*fullCompositeImage, trial);
    }
  }

  yaml.EndBlock();
}

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
  yaml.StartListItem();

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
    {NUM_TRIALS,   0,             "",  "trials",  PositiveIntArg,
     "  --trials=<num>         Set the number of trials (rendered frames).\n"
     "                         (Default 10)"});
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
     "  --color-ubyte          Store colors in 8-bit channels. (Default)"});
  usage.push_back(
    {COLOR_FORMAT, COLOR_FLOAT,   "",  "color-float", option::Arg::None,
     "  --color-float          Store colors in 32-bit float channels."});
  usage.push_back(
    {DEPTH_FORMAT, DEPTH_FLOAT,   "",  "depth-float", option::Arg::None,
     "  --depth-float          Store depth as 32-bit float. (Default)"});
  usage.push_back(
    {DEPTH_FORMAT, DEPTH_NONE,    "",  "depth-none", option::Arg::None,
     "  --depth-none           Do not use a depth buffer. This option changes\n"
     "                         the compositing to an alpha blending mode.\n"});

  usage.push_back(
    {CAMERA_THETA, CAMERA_STILL,  "",  "camera-theta", FloatArg,
     "  --camera-theta=<angle> Set the camera theta value to a specific value\n"
     "                         in degrees."});
  usage.push_back(
    {CAMERA_PHI,   CAMERA_STILL,  "",  "camera-phi", FloatArg,
     "  --camera-phi=<angle>   Set the camera phi value to a specific value in\n"
     "                         degrees."});
  usage.push_back(
    {CAMERA_ZOOM,  CAMERA_STILL,  "",  "camera-phi", FloatArg,
     "  --camera-zoom=<factor> Set the camera zoom to a specific value.\n"
     "                         (Default 1.0)"});
  usage.push_back(
    {CAMERA_THETA, CAMERA_ANIMATE,"",  "camera-animate-theta", option::Arg::None,
     "  --camera-animate-theta Animate the camera in the theta (horizontal)\n"
     "                         direction."});
  usage.push_back(
    {CAMERA_PHI,   CAMERA_ANIMATE,"",  "camera-animate-phi", option::Arg::None,
     "  --camera-animate-phi   Animate the camera in the phi (vertical)\n"
     "                         direction."});
  usage.push_back(
    {CAMERA_ZOOM,  CAMERA_ANIMATE,"",  "camera-animate-zoom", option::Arg::None,
     "  --camera-animate-zoom  Animate the camera zoom direction."});
  usage.push_back(
    {CAMERA_ANIMATE_ROTATE, 0,    "",  "camera-animate-rotate", option::Arg::None,
     "  --camera-animate-rotate Animates the camera in both the theta and phi\n"
     "                         directions. Equivalent to setting both\n"
     "                         --camera-animate-theta and --camera-animate-phi."});
  usage.push_back(
    {CAMERA_ANIMATE_ALL,0,        "",  "camera-animate", option::Arg::None,
     "  --camera-animate       Animates the camera in the theta, phi, and zoom\n"
     "                         directions. Equivalent to setting all of\n"
     "                         --camera-animate-theta, --camera-animate-phi,\n"
     "                         and --camera-animate-zoom."});
  usage.push_back(
    {CAMERA_THETA, CAMERA_RANDOM, "",  "camera-random-theta", option::Arg::None,
     "  --camera-random-theta  Select a random theta value for each trial.\n"
     "                         (Default)"});
  usage.push_back(
    {CAMERA_PHI,   CAMERA_RANDOM, "",  "camera-random-phi", option::Arg::None,
     "  --camera-random-phi    Select a random phi value for each trial.\n"
     "                         (Default)"});
  usage.push_back(
    {CAMERA_ZOOM,  CAMERA_RANDOM, "",  "camera-random-zoom", option::Arg::None,
     "  --camera-random-zoom   Select a random zoom value for each trial."});
  usage.push_back(
    {CAMERA_RANDOM_ROTATE, 0,     "",  "camera-random-rotate", option::Arg::None,
     "  --camera-random-rotate Randomizes the camera in both the theta and phi\n"
     "                         directions. Equivalent to setting both\n"
     "                         --camera-random-theta and --camera-random-phi."});
  usage.push_back(
    {CAMERA_RANDOM_ALL,0,         "",  "camera-random", option::Arg::None,
     "  --camera-random        Randomizes the camera in the theta, phi, and zoom\n"
     "                         directions. Equivalent to setting all of\n"
     "                         --camera-random-theta, --camera-random-phi,\n"
     "                         and --camera-random-zoom.\n"});

  usage.push_back(
    {RANDOM_SEED,  0,             "",  "random-seed",  PositiveIntArg,
     "  --random-seed=<num>    Set the seed used for the pseudo-random numbers.\n"
     "                         This can be set so that multiple runs will use\n"
     "                         all the same \"random\" parameters.\n"});

  // clang-format on

  for (auto compositorOpt = compositorOptions.begin();
       compositorOpt != compositorOptions.end();
       compositorOpt++) {
    usage.push_back(*compositorOpt);
  }

  usage.push_back({0, 0, 0, 0, 0, 0});

  RunOptions runOptions;

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
    runOptions.imageWidth = atoi(options[WIDTH].arg);
  }

  if (options[HEIGHT]) {
    runOptions.imageHeight = atoi(options[HEIGHT].arg);
  }

  if (options[NUM_TRIALS]) {
    runOptions.numTrials = atoi(options[NUM_TRIALS].arg);
  }

  if (options[YAML_OUTPUT]) {
    runOptions.yamlFilename = options[YAML_OUTPUT].arg;
  }

  if (options[CHECK_IMAGE]) {
    runOptions.checkImage = (options[CHECK_IMAGE].type() == ENABLE);
  }

  if (options[WRITE_IMAGE]) {
    runOptions.writeImage = (options[WRITE_IMAGE].type() == ENABLE);
  }

  if (options[PAINTER]) {
    runOptions.painter = static_cast<paintType>(options[PAINTER].type());
  }

  if (options[GEOMETRY]) {
    runOptions.geometry =
        static_cast<geometryType>(options[GEOMETRY].last()->type());
    runOptions.geometryFile = options[GEOMETRY].last()->arg;
  }

  if (options[DISTRIBUTION]) {
    runOptions.distribution =
        static_cast<distributionType>(options[DISTRIBUTION].last()->type());
  }

  if (options[COLOR_FORMAT]) {
    runOptions.colorFormat =
        static_cast<colorType>(options[COLOR_FORMAT].type());
  }
  if (options[DEPTH_FORMAT]) {
    runOptions.depthFormat =
        static_cast<depthType>(options[DEPTH_FORMAT].type());
  }

  if (options[OVERLAP]) {
    runOptions.overlap = strtof(options[OVERLAP].arg, NULL);
  }

  for (option::Option* thetaOpt = options[CAMERA_THETA]; thetaOpt;
       thetaOpt = thetaOpt->next()) {
    runOptions.thetaMove = static_cast<cameraMoveType>(thetaOpt->type());
    if (runOptions.thetaMove == CAMERA_STILL) {
      runOptions.thetaRotation = strtof(thetaOpt->arg, NULL);
    }
  }

  for (option::Option* phiOpt = options[CAMERA_PHI]; phiOpt;
       phiOpt = phiOpt->next()) {
    runOptions.phiMove = static_cast<cameraMoveType>(phiOpt->type());
    if (runOptions.phiMove == CAMERA_STILL) {
      runOptions.phiRotation = strtof(phiOpt->arg, NULL);
    }
  }

  for (option::Option* zoomOpt = options[CAMERA_ZOOM]; zoomOpt;
       zoomOpt = zoomOpt->next()) {
    runOptions.zoomMove = static_cast<cameraMoveType>(zoomOpt->type());
    if (runOptions.zoomMove == CAMERA_STILL) {
      runOptions.zoom = strtof(zoomOpt->arg, NULL);
    }
  }

  if (options[CAMERA_ANIMATE_ROTATE]) {
    runOptions.thetaMove = CAMERA_ANIMATE;
    runOptions.phiMove = CAMERA_ANIMATE;
  }
  if (options[CAMERA_ANIMATE_ALL]) {
    runOptions.thetaMove = CAMERA_ANIMATE;
    runOptions.phiMove = CAMERA_ANIMATE;
    runOptions.zoomMove = CAMERA_ANIMATE;
  }

  if (options[CAMERA_RANDOM_ROTATE]) {
    runOptions.thetaMove = CAMERA_RANDOM;
    runOptions.phiMove = CAMERA_RANDOM;
  }
  if (options[CAMERA_RANDOM_ALL]) {
    runOptions.thetaMove = CAMERA_RANDOM;
    runOptions.phiMove = CAMERA_RANDOM;
    runOptions.zoomMove = CAMERA_RANDOM;
  }

  int seed = static_cast<int>(
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
  if (options[RANDOM_SEED]) {
    seed = atoi(options[RANDOM_SEED].arg);
  }
  MPI_Bcast(&seed, 1, MPI_INT, 0, MPI_COMM_WORLD);
  yaml.AddDictionaryEntry("random-seed", seed);
  runOptions.randomEngine.seed(seed);

  if (rank == 0) {
    std::cout << "Rank " << rank << " on pid " << getpid() << std::endl;
#if 0
    int ready = 0;
    while (!ready);
#endif
  }

  run(runOptions, compositor, yaml);

  if (rank == 0) {
    std::ofstream yamlFile(runOptions.yamlFilename, std::ios_base::app);
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
