// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef IMAGEFULL_HPP
#define IMAGEFULL_HPP

#include "Image.hpp"

class ImageSparse;

class ImageFull : public Image {
 public:
  ImageFull(int _width, int _height)
      : Image(_width, _height, 0, _width * _height) {}
  ImageFull(int _width, int _height, int _regionBegin, int _regionEnd)
      : Image(_width, _height, _regionBegin, _regionEnd) {}

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

  virtual std::unique_ptr<ImageSparse> compress() const = 0;

  /// \brief Gathers all images to a single image.
  ///
  /// Given an MPI communicator and a destination rank, collects all images
  /// to the destination rank. It is assumed that all images contain a
  /// distinct subregion.
  virtual std::unique_ptr<ImageFull> Gather(int recvRank,
                                            MPI_Comm communicator) const = 0;
};

#endif  // IMAGEFULL_HPP
