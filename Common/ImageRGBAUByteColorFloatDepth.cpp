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
    : ImageColorDepth(_width, _height) {}

ImageRGBAUByteColorFloatDepth::ImageRGBAUByteColorFloatDepth(int _width,
                                                             int _height,
                                                             int _regionBegin,
                                                             int _regionEnd)
    : ImageColorDepth(_width, _height, _regionBegin, _regionEnd) {}

Color ImageRGBAUByteColorFloatDepth::getColor(int pixelIndex) const {
  assert(pixelIndex >= 0);
  assert(pixelIndex < this->getNumberOfPixels());

  const unsigned char *colorArray =
      reinterpret_cast<const unsigned char *>(this->getColorBuffer(pixelIndex));

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

  unsigned char *colorArray =
      reinterpret_cast<unsigned char *>(this->getColorBuffer(pixelIndex));

  colorArray[0] = color.GetComponentAsByte(0);
  colorArray[1] = color.GetComponentAsByte(1);
  colorArray[2] = color.GetComponentAsByte(2);
  colorArray[3] = color.GetComponentAsByte(3);
}

float ImageRGBAUByteColorFloatDepth::getDepth(int pixelIndex) const {
  assert(pixelIndex >= 0);
  assert(pixelIndex < this->getNumberOfPixels());

  return *this->getDepthBuffer(pixelIndex);
}

void ImageRGBAUByteColorFloatDepth::setDepth(int pixelIndex, float depth) {
  assert(pixelIndex >= 0);
  assert(pixelIndex < this->getNumberOfPixels());

  *this->getDepthBuffer(pixelIndex) = depth;
}

std::unique_ptr<Image> ImageRGBAUByteColorFloatDepth::createNew(
    int _width, int _height, int _regionBegin, int _regionEnd) const {
  return std::unique_ptr<Image>(new ImageRGBAUByteColorFloatDepth(
      _width, _height, _regionBegin, _regionEnd));
}

std::unique_ptr<const Image> ImageRGBAUByteColorFloatDepth::shallowCopy()
    const {
  return std::unique_ptr<const Image>(new ImageRGBAUByteColorFloatDepth(*this));
}
