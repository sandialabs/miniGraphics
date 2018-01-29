// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "ImageRGBFloatColorDepth.hpp"

#include <assert.h>

ImageRGBFloatColorDepth::ImageRGBFloatColorDepth(int _width, int _height)
    : ImageColorDepth(_width, _height) {}

ImageRGBFloatColorDepth::ImageRGBFloatColorDepth(int _width,
                                                 int _height,
                                                 int _regionBegin,
                                                 int _regionEnd)
    : ImageColorDepth(_width, _height, _regionBegin, _regionEnd) {}

Color ImageRGBFloatColorDepth::getColor(int pixelIndex) const {
  assert(pixelIndex >= 0);
  assert(pixelIndex < this->getNumberOfPixels());

  const float *colorArray = this->getColorBuffer(pixelIndex);
  return Color(colorArray[0], colorArray[1], colorArray[2]);
}

void ImageRGBFloatColorDepth::setColor(int pixelIndex, const Color &color) {
  assert(pixelIndex >= 0);
  assert(pixelIndex < this->getNumberOfPixels());

  float *colorArray = this->getColorBuffer(pixelIndex);

  colorArray[0] = color.Components[0];
  colorArray[1] = color.Components[1];
  colorArray[2] = color.Components[2];
}

float ImageRGBFloatColorDepth::getDepth(int pixelIndex) const {
  assert(pixelIndex >= 0);
  assert(pixelIndex < this->getNumberOfPixels());

  return *this->getDepthBuffer(pixelIndex);
}

void ImageRGBFloatColorDepth::setDepth(int pixelIndex, float depth) {
  assert(pixelIndex >= 0);
  assert(pixelIndex < this->getNumberOfPixels());

  *this->getDepthBuffer(pixelIndex) = depth;
}

std::unique_ptr<Image> ImageRGBFloatColorDepth::createNew(
    int _width, int _height, int _regionBegin, int _regionEnd) const {
  return std::unique_ptr<Image>(
      new ImageRGBFloatColorDepth(_width, _height, _regionBegin, _regionEnd));
}

std::unique_ptr<const Image> ImageRGBFloatColorDepth::shallowCopy() const {
  return std::unique_ptr<const Image>(new ImageRGBFloatColorDepth(*this));
}
