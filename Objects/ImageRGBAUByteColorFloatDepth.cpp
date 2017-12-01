// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "ImageRGBAUByteColorFloatDepth.hpp"

#include <assert.h>

ImageRGBAUByteColorFloatDepth::ImageRGBAUByteColorFloatDepth(int _width,
                                                             int _height)
    : Image(_width, _height),
      colorBuffer(new std::vector<unsigned int>(_width * _height)),
      depthBuffer(new std::vector<float>(_width * _height)) {}

ImageRGBAUByteColorFloatDepth::ImageRGBAUByteColorFloatDepth(int _width,
                                                             int _height,
                                                             int _regionBegin,
                                                             int _regionEnd)
    : Image(_width, _height, _regionBegin, _regionEnd),
      colorBuffer(new std::vector<unsigned int>(_regionEnd - _regionBegin)),
      depthBuffer(new std::vector<float>(_regionEnd - _regionBegin)) {}

Color ImageRGBAUByteColorFloatDepth::getColor(int pixelIndex) const {
  assert(pixelIndex >= 0);
  assert(pixelIndex < this->getNumberOfPixels());

  const unsigned char *colorArray = this->getColorBuffer() + 4 * pixelIndex;

  Color color;
  color.SetComponentFromByte(0, colorArray[0]);
  color.SetComponentFromByte(1, colorArray[1]);
  color.SetComponentFromByte(2, colorArray[2]);
  color.SetComponentFromByte(3, colorArray[3]);

  return color;
}

void ImageRGBAUByteColorFloatDepth::setColor(int pixelIndex,
                                             const Color &color) {
  assert(pixelIndex >= 0);
  assert(pixelIndex < this->getNumberOfPixels());

  unsigned char *colorArray = this->getColorBuffer() + 4 * pixelIndex;

  colorArray[0] = color.GetComponentAsByte(0);
  colorArray[1] = color.GetComponentAsByte(1);
  colorArray[2] = color.GetComponentAsByte(2);
  colorArray[3] = color.GetComponentAsByte(3);
}

float ImageRGBAUByteColorFloatDepth::getDepth(int pixelIndex) const {
  assert(pixelIndex >= 0);
  assert(pixelIndex < this->getNumberOfPixels());

  return this->getDepthBuffer()[pixelIndex];
}

void ImageRGBAUByteColorFloatDepth::setDepth(int pixelIndex, float depth) {
  assert(pixelIndex >= 0);
  assert(pixelIndex < this->getNumberOfPixels());

  this->getDepthBuffer()[pixelIndex] = depth;
}

void ImageRGBAUByteColorFloatDepth::clear(const Color &color, float depth) {
  int numPixels = this->getNumberOfPixels();
  if (numPixels < 1) {
    return;
  }

  // Encode the color by calling setColor for the first pixel.
  this->setColor(0, color);

  unsigned int *cBuffer = this->getEncodedColorBuffer();
  float *dBuffer = this->getDepthBuffer();

  unsigned int encodedColor = cBuffer[0];

  for (int index = 0; index < numPixels; ++index) {
    cBuffer[index] = encodedColor;
    dBuffer[index] = depth;
  }
}

std::unique_ptr<Image> ImageRGBAUByteColorFloatDepth::blend(
    const Image *_otherImage) const {
  const ImageRGBAUByteColorFloatDepth *otherImage =
      dynamic_cast<const ImageRGBAUByteColorFloatDepth *>(_otherImage);
  assert((otherImage != NULL) && "Attempting to blend invalid images.");

  int numPixels = this->getNumberOfPixels();
  assert((numPixels == otherImage->getNumberOfPixels()) &&
         "Attempting to blend images of different sizes.");

  const unsigned int *topColorBuffer = this->getEncodedColorBuffer();
  const float *topDepthBuffer = this->getDepthBuffer();
  const unsigned int *bottomColorBuffer = otherImage->getEncodedColorBuffer();
  const float *bottomDepthBuffer = otherImage->getDepthBuffer();

  std::unique_ptr<ImageRGBAUByteColorFloatDepth> outImage(
      new ImageRGBAUByteColorFloatDepth(this->getWidth(),
                                        this->getHeight(),
                                        this->getRegionBegin(),
                                        this->getRegionEnd()));
  unsigned int *outColorBuffer = outImage->getEncodedColorBuffer();
  float *outDepthBuffer = outImage->getDepthBuffer();

  for (int index = 0; index < numPixels; ++index) {
    if (bottomDepthBuffer[index] < topDepthBuffer[index]) {
      outColorBuffer[index] = bottomColorBuffer[index];
      outDepthBuffer[index] = bottomDepthBuffer[index];
    } else {
      outColorBuffer[index] = topColorBuffer[index];
      outDepthBuffer[index] = topDepthBuffer[index];
    }
  }

  return std::unique_ptr<Image>(outImage.release());
}

std::unique_ptr<Image> ImageRGBAUByteColorFloatDepth::createNew(
    int _width, int _height, int _regionBegin, int _regionEnd) const {
  return std::unique_ptr<Image>(new ImageRGBAUByteColorFloatDepth(
      _width, _height, _regionBegin, _regionEnd));
}

std::unique_ptr<Image> ImageRGBAUByteColorFloatDepth::copySubrange(
    int subregionBegin, int subregionEnd) const {
  assert(subregionBegin <= subregionEnd);
  std::unique_ptr<ImageRGBAUByteColorFloatDepth> subImage(
      new ImageRGBAUByteColorFloatDepth(this->getWidth(),
                                        this->getHeight(),
                                        subregionBegin + this->getRegionBegin(),
                                        subregionEnd + this->getRegionBegin()));
  std::copy(this->getEncodedColorBuffer() + subregionBegin,
            this->getEncodedColorBuffer() + subregionEnd,
            subImage->getEncodedColorBuffer());
  std::copy(this->getDepthBuffer() + subregionBegin,
            this->getDepthBuffer() + subregionEnd,
            subImage->getDepthBuffer());

  return std::unique_ptr<Image>(subImage.release());
}

std::unique_ptr<const Image> ImageRGBAUByteColorFloatDepth::shallowCopy()
    const {
  return std::unique_ptr<const Image>(new ImageRGBAUByteColorFloatDepth(*this));
}

std::unique_ptr<Image> ImageRGBAUByteColorFloatDepth::Gather(
    int recvRank, MPI_Comm communicator) const {
  int rank;
  MPI_Comm_rank(communicator, &rank);

  int numProc;
  MPI_Comm_size(communicator, &numProc);

  std::vector<int> allRegionBegin(numProc);
  int regionBegin = this->getRegionBegin();
  MPI_Gather(&regionBegin,
             1,
             MPI_INT,
             &allRegionBegin.front(),
             1,
             MPI_INT,
             recvRank,
             communicator);

  std::vector<int> allRegionCounts(numProc);
  int numPixels = this->getNumberOfPixels();
  MPI_Gather(&numPixels,
             1,
             MPI_INT,
             &allRegionCounts.front(),
             1,
             MPI_INT,
             recvRank,
             communicator);

  std::unique_ptr<ImageRGBAUByteColorFloatDepth> recvImage(
      new ImageRGBAUByteColorFloatDepth(this->getWidth(), this->getHeight()));

  MPI_Gatherv(this->getEncodedColorBuffer(),
              numPixels,
              MPI_UNSIGNED,
              recvImage->getEncodedColorBuffer(),
              &allRegionCounts.front(),
              &allRegionBegin.front(),
              MPI_UNSIGNED,
              recvRank,
              communicator);

  return std::unique_ptr<Image>(recvImage.release());
}

static const int COLOR_BUFFER_TAG = 12900;
static const int DEPTH_BUFFER_TAG = 12901;

std::vector<MPI_Request> ImageRGBAUByteColorFloatDepth::ISend(
    int destRank, MPI_Comm communicator) const {
  std::vector<MPI_Request> requests =
      this->ISendMetaData(destRank, communicator);

  MPI_Request colorRequest;
  MPI_Isend(&this->colorBuffer->front(),
            this->getNumberOfPixels(),
            MPI_UNSIGNED,
            destRank,
            COLOR_BUFFER_TAG,
            communicator,
            &colorRequest);
  requests.push_back(colorRequest);

  MPI_Request depthRequest;
  MPI_Isend(&this->depthBuffer->front(),
            this->getNumberOfPixels(),
            MPI_UNSIGNED,
            destRank,
            DEPTH_BUFFER_TAG,
            communicator,
            &depthRequest);
  requests.push_back(depthRequest);

  return requests;
}

std::vector<MPI_Request> ImageRGBAUByteColorFloatDepth::IReceive(
    int destRank, MPI_Comm communicator) {
  std::vector<MPI_Request> requests =
      this->IReceiveMetaData(destRank, communicator);

  MPI_Request colorRequest;
  MPI_Irecv(&this->colorBuffer->front(),
            this->getNumberOfPixels(),
            MPI_UNSIGNED,
            destRank,
            COLOR_BUFFER_TAG,
            communicator,
            &colorRequest);
  requests.push_back(colorRequest);

  MPI_Request depthRequest;
  MPI_Irecv(&this->depthBuffer->front(),
            this->getNumberOfPixels(),
            MPI_UNSIGNED,
            destRank,
            DEPTH_BUFFER_TAG,
            communicator,
            &depthRequest);
  requests.push_back(depthRequest);

  return requests;
}
