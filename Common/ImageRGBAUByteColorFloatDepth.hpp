// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef IMAGERGBAUBYTECOLORFLOATDEPTH_HPP
#define IMAGERGBAUBYTECOLORFLOATDEPTH_HPP

#include "ImageColorDepth.hpp"

struct ImageRGBAUByteColorFloatDepthFeatures {
  using ColorType = unsigned int;
  using DepthType = float;
  static constexpr int ColorVecSize = 1;

  static bool closer(const DepthType& distance1, const DepthType& distance2) {
    return distance1 < distance2;
  }

  static void encodeColor(const Color& color,
                          ColorType colorComponents[ColorVecSize]);
  static Color decodeColor(const ColorType colorComponents[ColorVecSize]);

  static void encodeDepth(float depth, DepthType depthComponents[1]);
  static float decodeDepth(const DepthType depthComponents[1]);
};

class ImageRGBAUByteColorFloatDepth
    : public ImageColorDepth<ImageRGBAUByteColorFloatDepthFeatures> {
 public:
  ImageRGBAUByteColorFloatDepth(int _width, int _height);
  ImageRGBAUByteColorFloatDepth(int _width,
                                int _height,
                                int _regionBegin,
                                int _regionEnd);
  ~ImageRGBAUByteColorFloatDepth() = default;

  std::unique_ptr<Image> createNew(int _width,
                                   int _height,
                                   int _regionBegin,
                                   int _regionEnd) const final;

  std::unique_ptr<const Image> shallowCopy() const final;
};

#endif  // IMAGERGBAUBYTECOLORFLOATDEPTH_HPP
