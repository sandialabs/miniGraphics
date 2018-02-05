// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "ImageRGBAFloatColorOnly.hpp"

#include <assert.h>

ImageRGBAFloatColorOnly::ImageRGBAFloatColorOnly(int _width, int _height)
    : ImageColorOnly(_width, _height) {}

ImageRGBAFloatColorOnly::ImageRGBAFloatColorOnly(int _width,
                                                 int _height,
                                                 int _regionBegin,
                                                 int _regionEnd)
    : ImageColorOnly(_width, _height, _regionBegin, _regionEnd) {}

Color ImageRGBAFloatColorOnly::getColor(int pixelIndex) const {
  assert(pixelIndex >= 0);
  assert(pixelIndex < this->getNumberOfPixels());

  const float *colorArray = this->getColorBuffer(pixelIndex);
  return Color(colorArray[0], colorArray[1], colorArray[2], colorArray[3]);
}

void ImageRGBAFloatColorOnly::setColor(int pixelIndex, const Color &color) {
  assert(pixelIndex >= 0);
  assert(pixelIndex < this->getNumberOfPixels());

  float *colorArray = this->getColorBuffer(pixelIndex);

  colorArray[0] = color.Components[0];
  colorArray[1] = color.Components[1];
  colorArray[2] = color.Components[2];
  colorArray[3] = color.Components[3];
}

float ImageRGBAFloatColorOnly::getDepth(int) const {
  // No depth
  return 1.0f;
}

void ImageRGBAFloatColorOnly::setDepth(int, float) {
  // No depth
}

std::unique_ptr<Image> ImageRGBAFloatColorOnly::blend(
    const Image *_otherImage) const {
  return this->blendImpl(_otherImage,
                         [](const float topColor[4],
                            const float bottomColor[4],
                            float outColor[4]) {
                           for (int component = 0; component < 4; ++component) {
                             outColor[component] =
                                 topColor[component] +
                                 bottomColor[component] * (1.0f - topColor[3]);
                           }
                         });
}

std::unique_ptr<Image> ImageRGBAFloatColorOnly::createNew(
    int _width, int _height, int _regionBegin, int _regionEnd) const {
  return std::unique_ptr<Image>(
      new ImageRGBAFloatColorOnly(_width, _height, _regionBegin, _regionEnd));
}

std::unique_ptr<const Image> ImageRGBAFloatColorOnly::shallowCopy() const {
  return std::unique_ptr<const Image>(new ImageRGBAFloatColorOnly(*this));
}
