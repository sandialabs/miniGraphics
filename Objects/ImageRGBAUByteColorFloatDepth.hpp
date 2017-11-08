// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef IMAGERGBAUBYTECOLORFLOATDEPTH_HPP
#define IMAGERGBAUBYTECOLORFLOATDEPTH_HPP

#include "Image.hpp"

#include <memory>
#include <vector>

class ImageRGBAUByteColorFloatDepth : public Image {
 private:
  std::shared_ptr<std::vector<unsigned int>> colorBuffer;
  std::shared_ptr<std::vector<float>> depthBuffer;

  unsigned int *getEncodedColorBuffer() { return &this->colorBuffer->front(); }

  const unsigned int *getEncodedColorBuffer() const {
    return &this->colorBuffer->front();
  }

 public:
  ImageRGBAUByteColorFloatDepth(int _width, int _height);
  ~ImageRGBAUByteColorFloatDepth() = default;

  Color getColor(int pixelIndex) const final;

  void setColor(int pixelIndex, const Color &color) final;

  float getDepth(int pixelIndex) const final;

  void setDepth(int pixelIndex, float depth) final;

  unsigned char *getColorBuffer() {
    return reinterpret_cast<unsigned char *>(this->getEncodedColorBuffer());
  }

  const unsigned char *getColorBuffer() const {
    return reinterpret_cast<const unsigned char *>(
        this->getEncodedColorBuffer());
  }

  float *getDepthBuffer() { return &this->depthBuffer->front(); }

  const float *getDepthBuffer() const { return &this->depthBuffer->front(); }

  void clear(const Color &color = Color(0, 0, 0, 0), float depth = 1.0f) final;

  void blend(const Image *_otherImage, BlendOrder) final;
};

#endif  // IMAGERGBAUBYTECOLORFLOATDEPTH_HPP
