// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef IMAGE_HPP
#define IMAGE_HPP

#include "Color.hpp"

class Image {
 private:
  int width;
  int height;

 public:
  Image(int _width, int _height) : width(_width), height(_height) {}

  virtual ~Image();

  int getWidth() const { return this->width; }
  int getHeight() const { return this->height; }

  int getNumberOfPixels() const { return this->getWidth() * this->getHeight(); }

  /// \brief Converts x/y location in image to a pixel index.
  int pixelIndex(int x, int y) const { return y * this->getHeight() + x; }

  /// \brief Converts a pixel index to the x/y location.
  void xyIndices(int pixelIndex, int& x, int& y) {
    x = pixelIndex % this->getHeight();
    y = pixelIndex / this->getHeight();
  }

  /// \brief Gets the color of the n'th pixel.
  virtual Color getColor(int pixelIndex) const = 0;

  /// \brief Gets the color at the given x and y location.
  Color getColor(int x, int y) const {
    return this->getColor(this->pixelIndex(x, y));
  }

  /// \brief Sets the color of the n'th pixel.
  virtual void setColor(int pixelIndex, const Color& color) = 0;

  /// \brief Sets the color at the given x and y location.
  void setColor(int x, int y, const Color& color) {
    this->setColor(this->pixelIndex(x, y), color);
  }

  /// \brief Gets the depth of the n'th pixel.
  ///
  /// Return value not defined if this image does not have a depth plane.
  virtual float getDepth(int pixelIndex) const = 0;

  /// \brief Gets the depth at the given x and y location
  ///
  /// Return value not defined if this image does not have a depth plane.
  float getDepth(int x, int y) const {
    return this->getDepth(this->pixelIndex(x, y));
  }

  /// \brief Sets the depth of the n'th pixel.
  ///
  /// If this image does not have a depth plane, this does nothing.
  virtual void setDepth(int pixelIndex, float depth) = 0;

  /// \brief Sets the depth at the given x and y location.
  ///
  /// If this image does not have a depth plane, this does nothing.
  void setDepth(int x, int y, float depth) {
    this->setDepth(this->pixelIndex(x, y), depth);
  }

  /// \brief Clears the image to the given color and depth (if applicable).
  virtual void clear(const Color& color = Color(0, 0, 0, 0),
                     float depth = 1.0f) = 0;

  enum BlendOrder { BLEND_OVER, BLEND_UNDER };

  /// \brief Blend this image with another image
  ///
  /// The result of the blend is placed in this image.
  ///
  /// This operation will almost certainly result in an error if the two images
  /// are not of the same type.
  ///
  /// The blendOrder operation determines which image is "on top." If
  /// blendOrder is BLEND_OVER, then this image is treated as on top. Otherwise
  /// it is treated as on the bottom. Some blend operations (like z-buffer) do
  /// not depend on the blendOrder, so in those cases it will be ignored.
  virtual void blend(const Image* otherImage,
                     BlendOrder blendOrder = BLEND_OVER) = 0;
};

#endif  // IMAGE_HPP
