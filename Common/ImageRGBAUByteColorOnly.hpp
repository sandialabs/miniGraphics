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

struct ImageRGBAUByteColorOnlyFeatures {
  using ColorType = unsigned int;
  static constexpr int ColorVecSize = 1;

  static void blend(const ColorType topColorEncoded[ColorVecSize],
                    const ColorType bottomColorEncoded[ColorVecSize],
                    ColorType outColorEncoded[ColorVecSize]) {
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
  }

  static void encodeColor(const Color &color,
                          ColorType colorComponents[ColorVecSize]);
  static Color decodeColor(const ColorType colorComponents[ColorVecSize]);
};

class ImageRGBAUByteColorOnly
    : public ImageColorOnly<ImageRGBAUByteColorOnlyFeatures> {
 public:
  ImageRGBAUByteColorOnly(int _width, int _height);
  ImageRGBAUByteColorOnly(int _width,
                          int _height,
                          int _regionBegin,
                          int _regionEnd);
  ~ImageRGBAUByteColorOnly() = default;

  std::unique_ptr<ImageSparse> compress() const final;

 protected:
  std::unique_ptr<Image> createNewImpl(int _width,
                                       int _height,
                                       int _regionBegin,
                                       int _regionEnd) const final;

  std::unique_ptr<const Image> shallowCopyImpl() const final;
};

#endif  // ImageRGBAUByteColorOnly_HPP
