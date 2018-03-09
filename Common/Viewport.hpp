// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

#include <algorithm>

/// \brief Provides a viewport in an image
///
/// A viewport in an image specifies some rectangular region within that image.
///
class Viewport {
  int minX;
  int minY;
  int maxX;
  int maxY;

 public:
  Viewport(int _minX, int _minY, int _maxX, int _maxY)
      : minX(_minX), minY(_minY), maxX(_maxX), maxY(_maxY) {}

  int getMinX() const { return this->minX; }
  int getMinY() const { return this->minY; }
  int getMaxX() const { return this->maxY; }
  int getMaxY() const { return this->maxY; }

  int getWidth() const { return this->maxX - this->minX + 1; }
  int getHeight() const { return this->maxY - this->minY + 1; }

  Viewport intersectWith(const Viewport& other) const {
    return Viewport(std::max(this->minX, other.minX),
                    std::max(this->minY, other.minY),
                    std::min(this->maxX, other.maxX),
                    std::min(this->maxY, other.maxY));
  }

  Viewport unionWith(const Viewport& other) const {
    return Viewport(std::min(this->minX, other.minX),
                    std::min(this->minY, other.minY),
                    std::max(this->maxX, other.maxX),
                    std::max(this->maxY, other.maxY));
  }
};

#endif  // VIEWPORT_HPP
