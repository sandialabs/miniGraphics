// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "ImageRGBAUByteColorOnly.hpp"

#include <assert.h>

ImageRGBAUByteColorOnly::ImageRGBAUByteColorOnly(int _width, int _height)
    : ImageColorOnly(_width, _height) {}

ImageRGBAUByteColorOnly::ImageRGBAUByteColorOnly(int _width,
                                                 int _height,
                                                 int _regionBegin,
                                                 int _regionEnd)
    : ImageColorOnly(_width, _height, _regionBegin, _regionEnd) {}

Color ImageRGBAUByteColorOnly::getColor(int pixelIndex) const {
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

void ImageRGBAUByteColorOnly::setColor(int pixelIndex, const Color &color) {
  assert(pixelIndex >= 0);
  assert(pixelIndex < this->getNumberOfPixels());

  unsigned char *colorArray =
      reinterpret_cast<unsigned char *>(this->getColorBuffer(pixelIndex));

  colorArray[0] = color.GetComponentAsByte(0);
  colorArray[1] = color.GetComponentAsByte(1);
  colorArray[2] = color.GetComponentAsByte(2);
  colorArray[3] = color.GetComponentAsByte(3);
}

float ImageRGBAUByteColorOnly::getDepth(int) const {
  // No depth
  return 1.0f;
}

void ImageRGBAUByteColorOnly::setDepth(int, float) {
  // No depth
}

std::unique_ptr<Image> ImageRGBAUByteColorOnly::blend(
    const Image *_otherImage) const {
  return this->blendImpl(
      _otherImage,
      [](const unsigned int *topColorEncoded,
         const unsigned int *bottomColorEncoded,
         unsigned int *outColorEncoded) {
        const unsigned char *topColor =
            reinterpret_cast<const unsigned char *>(topColorEncoded);
        const unsigned char *bottomColor =
            reinterpret_cast<const unsigned char *>(bottomColorEncoded);
        unsigned char *outColor =
            reinterpret_cast<unsigned char *>(outColorEncoded);
        float bottomScale = 1.0f - topColor[3] / 255.0f;
        for (int component = 0; component < 4; ++component) {
          outColor[component] =
              topColor[component] +
              static_cast<unsigned char>(bottomColor[component] * bottomScale);
        }
      });
}

std::unique_ptr<Image> ImageRGBAUByteColorOnly::createNew(
    int _width, int _height, int _regionBegin, int _regionEnd) const {
  return std::unique_ptr<Image>(
      new ImageRGBAUByteColorOnly(_width, _height, _regionBegin, _regionEnd));
}

std::unique_ptr<const Image> ImageRGBAUByteColorOnly::shallowCopy() const {
  return std::unique_ptr<const Image>(new ImageRGBAUByteColorOnly(*this));
}
