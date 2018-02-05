// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef IMAGERGBAFLOATCOLORONLY_HPP
#define IMAGERGBAFLOATCOLORONLY_HPP

#include "ImageColorOnly.hpp"

class ImageRGBAFloatColorOnly : public ImageColorOnly<float, 4> {
 public:
  ImageRGBAFloatColorOnly(int _width, int _height);
  ImageRGBAFloatColorOnly(int _width,
                          int _height,
                          int _regionBegin,
                          int _regionEnd);
  ~ImageRGBAFloatColorOnly() = default;

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

#endif  // IMAGERGBAFLOATCOLORONLY_HPP
