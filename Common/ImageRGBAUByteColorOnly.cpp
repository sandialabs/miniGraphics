// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "ImageRGBAUByteColorOnly.hpp"

#include "ImageSparseColorOnly.hpp"

#include <assert.h>

void ImageRGBAUByteColorOnlyFeatures::encodeColor(
    const Color &color, ColorType colorComponents[ColorVecSize]) {
  unsigned char *colorArray =
      reinterpret_cast<unsigned char *>(colorComponents);

  colorArray[0] = color.GetComponentAsByte(0);
  colorArray[1] = color.GetComponentAsByte(1);
  colorArray[2] = color.GetComponentAsByte(2);
  colorArray[3] = color.GetComponentAsByte(3);
}

Color ImageRGBAUByteColorOnlyFeatures::decodeColor(
    const ColorType colorComponents[ColorVecSize]) {
  const unsigned char *colorArray =
      reinterpret_cast<const unsigned char *>(colorComponents);

  Color color;
  color.SetComponentFromByte(0, colorArray[0]);
  color.SetComponentFromByte(1, colorArray[1]);
  color.SetComponentFromByte(2, colorArray[2]);
  color.SetComponentFromByte(3, colorArray[3]);

  return color;
}

ImageRGBAUByteColorOnly::ImageRGBAUByteColorOnly(int _width, int _height)
    : ImageColorOnly(_width, _height) {}

ImageRGBAUByteColorOnly::ImageRGBAUByteColorOnly(int _width,
                                                 int _height,
                                                 int _regionBegin,
                                                 int _regionEnd)
    : ImageColorOnly(_width, _height, _regionBegin, _regionEnd) {}

std::unique_ptr<ImageSparse> ImageRGBAUByteColorOnly::compress() const {
  return std::unique_ptr<ImageSparse>(
      new ImageSparseColorOnly<ImageRGBAUByteColorOnlyFeatures>(*this));
}

std::unique_ptr<Image> ImageRGBAUByteColorOnly::createNewImpl(
    int _width, int _height, int _regionBegin, int _regionEnd) const {
  return std::unique_ptr<Image>(
      new ImageRGBAUByteColorOnly(_width, _height, _regionBegin, _regionEnd));
}

std::unique_ptr<const Image> ImageRGBAUByteColorOnly::shallowCopyImpl() const {
  return std::unique_ptr<const Image>(new ImageRGBAUByteColorOnly(*this));
}
