// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef IMAGERGBAUBYTECOLORONLY_HPP
#define IMAGERGBAUBYTECOLORONLY_HPP

#include "ImageColorOnly.hpp"

class ImageRGBAUByteColorOnly : public ImageColorOnly<unsigned int, 1> {
 public:
  ImageRGBAUByteColorOnly(int _width, int _height);
  ImageRGBAUByteColorOnly(int _width,
                          int _height,
                          int _regionBegin,
                          int _regionEnd);
  ~ImageRGBAUByteColorOnly() = default;

  Color getColor(int pixelIndex) const final;

  void setColor(int pixelIndex, const Color& color) final;

  float getDepth(int pixelIndex) const final;

  void setDepth(int pixelIndex, float depth) final;

  std::unique_ptr<Image> blend(const Image* _otherImage) const final;

  std::unique_ptr<Image> createNew(int _width,
                                   int _height,
                                   int _regionBegin,
                                   int _regionEnd) const final;

  std::unique_ptr<const Image> shallowCopy() const final;
};

#endif  // ImageRGBAUByteColorOnly_HPP
