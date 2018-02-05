// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef IMAGERGBFLOATCOLORDEPTH_HPP
#define IMAGERGBFLOATCOLORDEPTH_HPP

#include "ImageColorDepth.hpp"

class ImageRGBFloatColorDepth : public ImageColorDepth<float, 3, float> {
 public:
  ImageRGBFloatColorDepth(int _width, int _height);
  ImageRGBFloatColorDepth(int _width,
                          int _height,
                          int _regionBegin,
                          int _regionEnd);
  ~ImageRGBFloatColorDepth() = default;

  Color getColor(int pixelIndex) const final;

  void setColor(int pixelIndex, const Color &color) final;

  float getDepth(int pixelIndex) const final;

  void setDepth(int pixelIndex, float depth) final;

  std::unique_ptr<Image> createNew(int _width,
                                   int _height,
                                   int _regionBegin,
                                   int _regionEnd) const final;

  std::unique_ptr<const Image> shallowCopy() const final;
};

#endif  // IMAGERGBFLOATCOLORDEPTH_HPP
