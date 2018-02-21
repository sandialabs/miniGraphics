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

struct ImageRGBAFloatColorOnlyFeatures {
  using ColorType = float;
  static constexpr int ColorVecSize = 4;

  static void blend(const ColorType topColor[ColorVecSize],
                    const ColorType bottomColor[ColorVecSize],
                    ColorType outColor[ColorVecSize]) {
    for (int component = 0; component < 4; ++component) {
      outColor[component] =
          topColor[component] + bottomColor[component] * (1.0f - topColor[3]);
    }
  }

  static void encodeColor(const Color& color,
                          ColorType colorComponents[ColorVecSize]);
  static Color decodeColor(const ColorType colorComponents[ColorVecSize]);
};

class ImageRGBAFloatColorOnly
    : public ImageColorOnly<ImageRGBAFloatColorOnlyFeatures> {
 public:
  ImageRGBAFloatColorOnly(int _width, int _height);
  ImageRGBAFloatColorOnly(int _width,
                          int _height,
                          int _regionBegin,
                          int _regionEnd);
  ~ImageRGBAFloatColorOnly() = default;

  std::unique_ptr<ImageSparse> compress() const final;

  std::unique_ptr<Image> createNew(int _width,
                                   int _height,
                                   int _regionBegin,
                                   int _regionEnd) const final;

  std::unique_ptr<const Image> shallowCopy() const final;
};

#endif  // IMAGERGBAFLOATCOLORONLY_HPP
