// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include <Common/ImageRGBAFloatColorOnly.hpp>
#include <Common/ImageRGBAUByteColorFloatDepth.hpp>
#include <Common/ImageRGBAUByteColorOnly.hpp>
#include <Common/ImageRGBFloatColorDepth.hpp>
#include <Common/ImageSparse.hpp>
#include <Common/SavePPM.hpp>

#include <cmath>
#include <iostream>
#include <string>
#include <type_traits>

#include <mpi.h>

#define TEST_ASSERT(condition) \
  CheckAssert(condition, #condition, __FILE__, __LINE__);

static void CheckAssert(bool condition,
                        const std::string& conditionStr,
                        const std::string& filename,
                        int line) {
  if (condition) {
    std::cout << "    OK (" << conditionStr << ")" << std::endl;
  } else {
    std::cerr << "    *** FAILED! *** (" << conditionStr << "), " << filename
              << ":" << line << std::endl;
    exit(1);
  }
}

using ImageTypes = std::tuple<ImageRGBAFloatColorOnly,
                              ImageRGBAUByteColorFloatDepth,
                              ImageRGBAUByteColorOnly,
                              ImageRGBFloatColorDepth>;

template <typename ImageType>
using ImageIsColorDepth = std::is_base_of<ImageColorDepthBase, ImageType>;

template <typename ImageType>
using ImageIsColorOnly = std::is_base_of<ImageColorOnlyBase, ImageType>;

constexpr int IMAGE_WIDTH = 100;
constexpr int IMAGE_HEIGHT = 100;
constexpr int BORDER = 10;

static void compareImages(const ImageFull& image1, const ImageFull& image2) {
  constexpr float COLOR_THRESHOLD = 0.01f;
  constexpr float BAD_PIXEL_THRESHOLD = 0.01f;

  TEST_ASSERT(image1.getNumberOfPixels() == image2.getNumberOfPixels());

  int numPixels = image1.getNumberOfPixels();
  int numBadPixels = 0;
  for (int pixel = 0; pixel < numPixels; ++pixel) {
    Color color1 = image1.getColor(pixel);
    Color color2 = image2.getColor(pixel);
    if ((std::abs(color1.Components[0] - color2.Components[0]) >
         COLOR_THRESHOLD) ||
        (std::abs(color1.Components[1] - color2.Components[1]) >
         COLOR_THRESHOLD) ||
        (std::abs(color1.Components[2] - color2.Components[2]) >
         COLOR_THRESHOLD)) {
      ++numBadPixels;
    }
  }

  if (!(numBadPixels <= BAD_PIXEL_THRESHOLD * numPixels) &&
      (image1.getRegionBegin() == 0) &&
      (image1.getRegionEnd() == image1.getNumberOfPixels())) {
    SavePPM(image1, "image1.ppm");
    SavePPM(image2, "image2.ppm");
  }
  TEST_ASSERT(numBadPixels <= BAD_PIXEL_THRESHOLD * numPixels);
}

static void compareImages(const ImageSparse& image1,
                          const ImageSparse& image2) {
  compareImages(*image1.uncompress(), *image2.uncompress());
}

static void compareImages(const ImageFull& image1, const ImageSparse& image2) {
  compareImages(image1, *image2.uncompress());
}

static void compareImages(const ImageSparse& image1, const ImageFull& image2) {
  compareImages(*image1.uncompress(), image2);
}

static void compareImages(const Image& image1, const Image& image2) {
  const ImageFull* imageFull1 = dynamic_cast<const ImageFull*>(&image1);
  const ImageFull* imageFull2 = dynamic_cast<const ImageFull*>(&image2);
  const ImageSparse* imageSparse1 = dynamic_cast<const ImageSparse*>(&image1);
  const ImageSparse* imageSparse2 = dynamic_cast<const ImageSparse*>(&image2);
  if (imageFull1 != nullptr) {
    if (imageFull2 != nullptr) {
      compareImages(*imageFull1, *imageFull2);
    } else {
      assert(imageSparse2 != nullptr);
      compareImages(*imageFull1, *imageSparse2);
    }
  } else {
    assert(imageSparse1 != nullptr);
    if (imageFull2 != nullptr) {
      compareImages(*imageSparse1, *imageFull2);
    } else {
      assert(imageSparse2 != nullptr);
      compareImages(*imageSparse1, *imageSparse2);
    }
  }
}

template <typename ImageType>
static std::unique_ptr<ImageType> createColorDepthImage1(int regionBegin,
                                                         int regionEnd) {
  std::unique_ptr<ImageType> image(
      new ImageType(IMAGE_WIDTH, IMAGE_HEIGHT, regionBegin, regionEnd));
  image->clear();
  Color color(1.0f, 0.0f, 0.0f);
  for (int pixelIndex = regionBegin; pixelIndex < regionEnd; ++pixelIndex) {
    int x = pixelIndex % IMAGE_WIDTH;
    int y = pixelIndex / IMAGE_WIDTH;
    if ((x > BORDER) && (x < IMAGE_WIDTH - BORDER) && (y > BORDER) &&
        (y < IMAGE_HEIGHT - BORDER) && (x <= y)) {
      image->setColor(pixelIndex - regionBegin, color);
      image->setDepth(pixelIndex - regionBegin,
                      static_cast<float>(x) / IMAGE_WIDTH);
    }
  }

  return image;
}

template <typename ImageType>
static std::unique_ptr<ImageType> createColorOnlyImage1(int regionBegin,
                                                        int regionEnd) {
  std::unique_ptr<ImageType> image(
      new ImageType(IMAGE_WIDTH, IMAGE_HEIGHT, regionBegin, regionEnd));
  image->clear();
  Color color(0.5f, 0.0f, 0.0f, 0.5f);
  for (int pixelIndex = regionBegin; pixelIndex < regionEnd; ++pixelIndex) {
    int x = pixelIndex % IMAGE_WIDTH;
    int y = pixelIndex / IMAGE_WIDTH;
    if ((x > BORDER) && (x < IMAGE_WIDTH - BORDER) && (y > BORDER) &&
        (y < IMAGE_HEIGHT - BORDER) && (x <= y)) {
      image->setColor(pixelIndex - regionBegin, color);
    }
  }

  return image;
}

template <typename ImageType>
static std::unique_ptr<ImageType> createImage1Impl(int regionBegin,
                                                   int regionEnd,
                                                   std::true_type) {
  return createColorDepthImage1<ImageType>(regionBegin, regionEnd);
}

template <typename ImageType>
static std::unique_ptr<ImageType> createImage1Impl(int regionBegin,
                                                   int regionEnd,
                                                   std::false_type) {
  return createColorOnlyImage1<ImageType>(regionBegin, regionEnd);
}

template <typename ImageType>
static std::unique_ptr<ImageType> createImage1(int regionBegin = 0,
                                               int regionEnd = IMAGE_WIDTH *
                                                               IMAGE_HEIGHT) {
  return createImage1Impl<ImageType>(
      regionBegin, regionEnd, ImageIsColorDepth<ImageType>());
}

template <typename ImageType>
static std::unique_ptr<ImageType> createColorDepthImage2(int regionBegin,
                                                         int regionEnd) {
  std::unique_ptr<ImageType> image(
      new ImageType(IMAGE_WIDTH, IMAGE_HEIGHT, regionBegin, regionEnd));
  image->clear();
  Color color(0.0f, 0.0f, 1.0f);
  for (int pixelIndex = regionBegin; pixelIndex < regionEnd; ++pixelIndex) {
    int x = pixelIndex % IMAGE_WIDTH;
    int y = pixelIndex / IMAGE_WIDTH;
    if ((x > BORDER) && (x < IMAGE_WIDTH - BORDER) && (y > BORDER) &&
        (y < IMAGE_HEIGHT - BORDER) && (x <= (IMAGE_HEIGHT - y))) {
      image->setColor(pixelIndex - regionBegin, color);
      image->setDepth(pixelIndex - regionBegin,
                      0.5f * static_cast<float>(IMAGE_WIDTH - x) / IMAGE_WIDTH);
    }
  }

  return image;
}

template <typename ImageType>
static std::unique_ptr<ImageType> createColorOnlyImage2(int regionBegin,
                                                        int regionEnd) {
  std::unique_ptr<ImageType> image(
      new ImageType(IMAGE_WIDTH, IMAGE_HEIGHT, regionBegin, regionEnd));
  image->clear();
  Color color(0.0f, 0.0f, 0.5f, 0.5f);
  for (int pixelIndex = regionBegin; pixelIndex < regionEnd; ++pixelIndex) {
    int x = pixelIndex % IMAGE_WIDTH;
    int y = pixelIndex / IMAGE_WIDTH;
    if ((x > BORDER) && (x < IMAGE_WIDTH - BORDER) && (y > BORDER) &&
        (y < IMAGE_HEIGHT - BORDER) && (x <= (IMAGE_HEIGHT - y))) {
      image->setColor(pixelIndex - regionBegin, color);
    }
  }

  return image;
}

template <typename ImageType>
static std::unique_ptr<ImageType> createImage2Impl(int regionBegin,
                                                   int regionEnd,
                                                   std::true_type) {
  return createColorDepthImage2<ImageType>(regionBegin, regionEnd);
}

template <typename ImageType>
static std::unique_ptr<ImageType> createImage2Impl(int regionBegin,
                                                   int regionEnd,
                                                   std::false_type) {
  return createColorOnlyImage2<ImageType>(regionBegin, regionEnd);
}

template <typename ImageType>
static std::unique_ptr<ImageType> createImage2(int regionBegin = 0,
                                               int regionEnd = IMAGE_WIDTH *
                                                               IMAGE_HEIGHT) {
  return createImage2Impl<ImageType>(
      regionBegin, regionEnd, ImageIsColorDepth<ImageType>());
}

template <typename ImageType>
static std::unique_ptr<ImageType> createColorDepthImageCombined(int regionBegin,
                                                                int regionEnd) {
  std::unique_ptr<ImageType> image(
      new ImageType(IMAGE_WIDTH, IMAGE_HEIGHT, regionBegin, regionEnd));
  image->clear();
  Color color1(1.0f, 0.0f, 0.0f);
  Color color2(0.0f, 0.0f, 1.0f);
  for (int pixelIndex = regionBegin; pixelIndex < regionEnd; ++pixelIndex) {
    int x = pixelIndex % IMAGE_WIDTH;
    int y = pixelIndex / IMAGE_WIDTH;
    if ((x > BORDER) && (x < IMAGE_WIDTH - BORDER) && (y > BORDER) &&
        (y < IMAGE_HEIGHT - BORDER)) {
      if ((x <= y) && (x > (IMAGE_HEIGHT - y) || (x < (IMAGE_WIDTH - x) / 2))) {
        image->setColor(pixelIndex - regionBegin, color1);
        image->setDepth(pixelIndex - regionBegin,
                        static_cast<float>(x) / IMAGE_WIDTH);
      } else if (x <= (IMAGE_HEIGHT - y)) {
        image->setColor(pixelIndex - regionBegin, color2);
        image->setDepth(
            pixelIndex - regionBegin,
            0.5f * static_cast<float>(IMAGE_WIDTH - x) / IMAGE_WIDTH);
      }
    }
  }

  return image;
}

template <typename ImageType>
static std::unique_ptr<ImageType> createColorOnlyImageCombined(int regionBegin,
                                                               int regionEnd) {
  std::unique_ptr<ImageType> image(
      new ImageType(IMAGE_WIDTH, IMAGE_HEIGHT, regionBegin, regionEnd));
  image->clear();
  Color color1(0.5f, 0.0f, 0.0f, 0.5f);
  Color color2(0.0f, 0.0f, 0.5f, 0.5f);
  Color colorBlend(0.5f, 0.0f, 0.25f, 0.75f);
  for (int pixelIndex = regionBegin; pixelIndex < regionEnd; ++pixelIndex) {
    int x = pixelIndex % IMAGE_WIDTH;
    int y = pixelIndex / IMAGE_WIDTH;
    if ((x > BORDER) && (x < IMAGE_WIDTH - BORDER) && (y > BORDER) &&
        (y < IMAGE_HEIGHT - BORDER)) {
      if (x <= y) {
        if (x <= (IMAGE_HEIGHT - y)) {
          image->setColor(pixelIndex - regionBegin, colorBlend);
        } else {
          image->setColor(pixelIndex - regionBegin, color1);
        }
      } else {
        if (x <= (IMAGE_HEIGHT - y)) {
          image->setColor(pixelIndex - regionBegin, color2);
        }
      }
    }
  }

  return image;
}

template <typename ImageType>
static std::unique_ptr<ImageType> createImageCombinedImpl(int regionBegin,
                                                          int regionEnd,
                                                          std::true_type) {
  return createColorDepthImageCombined<ImageType>(regionBegin, regionEnd);
}

template <typename ImageType>
static std::unique_ptr<ImageType> createImageCombinedImpl(int regionBegin,
                                                          int regionEnd,
                                                          std::false_type) {
  return createColorOnlyImageCombined<ImageType>(regionBegin, regionEnd);
}

template <typename ImageType>
static std::unique_ptr<ImageType> createImageCombined(
    int regionBegin = 0, int regionEnd = IMAGE_WIDTH * IMAGE_HEIGHT) {
  return createImageCombinedImpl<ImageType>(
      regionBegin, regionEnd, ImageIsColorDepth<ImageType>());
}

template <typename ImageType>
static void TestCompressUncompress() {
  std::cout << "  Compress/Uncompress" << std::endl;

  std::unique_ptr<ImageType> fullImage = createImage1<ImageType>();
  std::unique_ptr<ImageSparse> sparseImage = fullImage->compress();

  compareImages(*fullImage, *sparseImage->uncompress());
}

template <typename ImageType>
static void TestShallowCopy() {
  std::cout << "  Shallow copy" << std::endl;

  std::unique_ptr<ImageSparse> image = createImage1<ImageType>()->compress();

  std::unique_ptr<Image> imageCopy = image->shallowCopy();
  TEST_ASSERT(dynamic_cast<ImageSparse*>(imageCopy.get()) != nullptr);
  compareImages(*image, *imageCopy);

  std::cout << "  Shallow copy empty image" << std::endl;
  image->clear();
  imageCopy = image->shallowCopy();
  TEST_ASSERT(dynamic_cast<ImageSparse*>(imageCopy.get()) != nullptr);
  compareImages(*image, *imageCopy);
}

template <typename ImageType>
static void TestDeepCopy() {
  std::cout << "  Deep copy" << std::endl;

  std::unique_ptr<ImageSparse> image = createImage1<ImageType>()->compress();

  std::unique_ptr<Image> imageCopy = image->deepCopy();
  TEST_ASSERT(dynamic_cast<ImageSparse*>(imageCopy.get()) != nullptr);
  compareImages(*image, *imageCopy);

  std::cout << "  Deep copy empty image" << std::endl;
  image->clear();
  imageCopy = image->deepCopy();
  TEST_ASSERT(dynamic_cast<ImageSparse*>(imageCopy.get()) != nullptr);
  compareImages(*image, *imageCopy);
}

template <typename ImageType>
static void TryTransfer(const ImageType& srcImage) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  std::unique_ptr<Image> destImage = srcImage.createNew();
  std::vector<MPI_Request> recvRequests =
      destImage->IReceive(rank, MPI_COMM_WORLD);

  std::vector<MPI_Request> sendRequests = srcImage.ISend(rank, MPI_COMM_WORLD);

  std::vector<MPI_Status> statuses(recvRequests.size());
  MPI_Waitall(recvRequests.size(), &recvRequests.front(), &statuses.front());
  compareImages(srcImage, *destImage);

  statuses.resize(sendRequests.size());
  MPI_Waitall(sendRequests.size(), &sendRequests.front(), &statuses.front());
}

template <typename ImageType>
static void TestTransfer() {
  std::cout << "  Transfer regular image" << std::endl;
  std::unique_ptr<ImageSparse> srcImage = createImage1<ImageType>()->compress();
  TryTransfer(*srcImage);


  std::cout << "  Transfer clear image" << std::endl;
  srcImage->clear();
  TryTransfer(*srcImage);

  std::cout << "  Transfer empty image" << std::endl;
  TryTransfer(*ImageType(0, 0).compress());
}

template <typename ImageType>
static void TestSubrange() {
  std::cout << "  Subrange" << std::endl;

  constexpr int MID1 = IMAGE_WIDTH * IMAGE_HEIGHT / 3;
  constexpr int MID2 = IMAGE_WIDTH * IMAGE_HEIGHT / 2;

  std::unique_ptr<ImageSparse> srcImage = createImage1<ImageType>()->compress();

  std::cout << "    Mid 1" << std::endl;
  std::unique_ptr<Image> subImage = srcImage->copySubrange(0, MID1);
  compareImages(*subImage, *createImage1<ImageType>(0, MID1));

  std::cout << "    Mid 2" << std::endl;
  subImage = srcImage->copySubrange(MID1, MID2);
  compareImages(*subImage, *createImage1<ImageType>(MID1, MID2));

  std::cout << "    End" << std::endl;
  subImage = srcImage->copySubrange(MID2, IMAGE_WIDTH * IMAGE_HEIGHT);
  compareImages(*subImage, *createImage1<ImageType>(MID2));
}

template <typename ImageType>
static void TestBlend() {
  std::unique_ptr<ImageSparse> topImage = createImage1<ImageType>()->compress();
  std::unique_ptr<ImageSparse> bottomImage =
      createImage2<ImageType>()->compress();

  std::cout << "  Blend non-empty" << std::endl;
  std::unique_ptr<Image> blendImage = topImage->blend(*bottomImage);
  compareImages(*blendImage, *createImageCombined<ImageType>());

  std::unique_ptr<Image> emptyImage = topImage->createNew();
  emptyImage->clear();

  std::cout << "  Blend top empty" << std::endl;
  blendImage = emptyImage->blend(*bottomImage);
  compareImages(*blendImage, *bottomImage);

  std::cout << "  Blend bottom empty" << std::endl;
  blendImage = topImage->blend(*emptyImage);
  compareImages(*blendImage, *topImage);

  std::cout << "  Blend both empty" << std::endl;
  blendImage = emptyImage->blend(*emptyImage);
  compareImages(*blendImage, *emptyImage);
}

template <typename ImageType>
static void DoImageTest(const std::string& imageTypeName) {
  std::cout << imageTypeName << std::endl;
  TestCompressUncompress<ImageType>();
  TestShallowCopy<ImageType>();
  TestDeepCopy<ImageType>();
  TestTransfer<ImageType>();
  TestSubrange<ImageType>();
  TestBlend<ImageType>();
}

#define DO_IMAGE_TEST(ImageType) DoImageTest<ImageType>(#ImageType)

int ImageSparseTest(int argc, char* argv[]) {
  MPI_Init(&argc, &argv);

  DO_IMAGE_TEST(ImageRGBAFloatColorOnly);
  DO_IMAGE_TEST(ImageRGBAUByteColorFloatDepth);
  DO_IMAGE_TEST(ImageRGBAUByteColorOnly);
  DO_IMAGE_TEST(ImageRGBFloatColorDepth);

  MPI_Finalize();

  return 0;
}