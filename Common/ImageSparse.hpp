// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef IMAGESPARSE_HPP
#define IMAGESPARSE_HPP

#include "Image.hpp"

class ImageFull;

class ImageSparse : public Image {
 protected:
  struct RunLengthRegion {
    int backgroundPixels;
    int foregroundPixels;

    RunLengthRegion() : backgroundPixels(0), foregroundPixels(0) {}
  };

 public:
  ImageSparse(int _width, int _height)
      : Image(_width, _height, 0, _width * _height) {}
  ImageSparse(int _width, int _height, int _regionBegin, int _regionEnd)
      : Image(_width, _height, _regionBegin, _regionEnd) {}

  virtual std::unique_ptr<ImageFull> uncompress() const = 0;
};

#endif  // IMAGESPARSE_HPP
