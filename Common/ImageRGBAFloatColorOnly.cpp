// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "ImageRGBAFloatColorOnly.hpp"

#include <assert.h>

void ImageRGBAFloatColorOnlyFeatures::encodeColor(
    const Color& color, ColorType colorComponents[ColorVecSize]) {
  colorComponents[0] = color.Components[0];
  colorComponents[1] = color.Components[1];
  colorComponents[2] = color.Components[2];
  colorComponents[3] = color.Components[3];
}

Color ImageRGBAFloatColorOnlyFeatures::decodeColor(
    const ColorType colorComponents[ColorVecSize]) {
  return Color(colorComponents[0],
               colorComponents[1],
               colorComponents[2],
               colorComponents[3]);
}

ImageRGBAFloatColorOnly::ImageRGBAFloatColorOnly(int _width, int _height)
    : ImageColorOnly(_width, _height) {}

ImageRGBAFloatColorOnly::ImageRGBAFloatColorOnly(int _width,
                                                 int _height,
                                                 int _regionBegin,
                                                 int _regionEnd)
    : ImageColorOnly(_width, _height, _regionBegin, _regionEnd) {}

std::unique_ptr<Image> ImageRGBAFloatColorOnly::createNew(
    int _width, int _height, int _regionBegin, int _regionEnd) const {
  return std::unique_ptr<Image>(
      new ImageRGBAFloatColorOnly(_width, _height, _regionBegin, _regionEnd));
}

std::unique_ptr<const Image> ImageRGBAFloatColorOnly::shallowCopy() const {
  return std::unique_ptr<const Image>(new ImageRGBAFloatColorOnly(*this));
}
