// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "ImageRGBFloatColorDepth.hpp"

#include "ImageSparseColorDepth.hpp"

#include <assert.h>

void ImageRGBFloatColorDepthFeatures::encodeColor(
    const Color& color, ColorType colorComponents[ColorVecSize]) {
  colorComponents[0] = color.Components[0];
  colorComponents[1] = color.Components[1];
  colorComponents[2] = color.Components[2];
}

Color ImageRGBFloatColorDepthFeatures::decodeColor(
    const ColorType colorComponents[ColorVecSize]) {
  return Color(colorComponents[0], colorComponents[1], colorComponents[2]);
}

void ImageRGBFloatColorDepthFeatures::encodeDepth(
    float depth, DepthType depthComponents[1]) {
  depthComponents[0] = depth;
}

float ImageRGBFloatColorDepthFeatures::decodeDepth(
    const DepthType depthComponents[1]) {
  return depthComponents[0];
}

ImageRGBFloatColorDepth::ImageRGBFloatColorDepth(int _width, int _height)
    : ImageColorDepth(_width, _height) {}

ImageRGBFloatColorDepth::ImageRGBFloatColorDepth(int _width,
                                                 int _height,
                                                 int _regionBegin,
                                                 int _regionEnd)
    : ImageColorDepth(_width, _height, _regionBegin, _regionEnd) {}

std::unique_ptr<ImageSparse> ImageRGBFloatColorDepth::compress() const {
  return std::unique_ptr<ImageSparse>(
      new ImageSparseColorDepth<ImageRGBFloatColorDepthFeatures>(*this));
}

std::unique_ptr<Image> ImageRGBFloatColorDepth::createNew(
    int _width, int _height, int _regionBegin, int _regionEnd) const {
  return std::unique_ptr<Image>(
      new ImageRGBFloatColorDepth(_width, _height, _regionBegin, _regionEnd));
}

std::unique_ptr<const Image> ImageRGBFloatColorDepth::shallowCopy() const {
  return std::unique_ptr<const Image>(new ImageRGBFloatColorDepth(*this));
}
