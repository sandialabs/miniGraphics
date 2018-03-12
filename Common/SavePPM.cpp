// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "SavePPM.hpp"

#include <Common/ImageFull.hpp>
#include <Common/ImageSparse.hpp>

#include <fstream>

static bool doSavePPM(const ImageFull &image, const std::string &filename) {
  std::ofstream file(filename.c_str(),
                     std::ios_base::binary | std::ios_base::out);

  file << "P6" << std::endl;
  file << image.getWidth() << " " << image.getHeight() << std::endl;
  file << 255 << std::endl;

  for (int y = image.getHeight() - 1; y >= 0; --y) {
    for (int x = 0; x < image.getWidth(); ++x) {
      Color color = image.getColor(x, y);
      file << color.GetComponentAsByte(0);
      file << color.GetComponentAsByte(1);
      file << color.GetComponentAsByte(2);
    }
  }

  file.close();
  return !file.fail();
}

static bool doSavePPM(const Image &image, const std::string &filename) {
  const ImageFull *fullImage = dynamic_cast<const ImageFull *>(&image);
  if (fullImage != nullptr) {
    return doSavePPM(*fullImage, filename);
  }

  const ImageSparse *sparseImage = dynamic_cast<const ImageSparse *>(&image);
  if (sparseImage != nullptr) {
    return doSavePPM(*sparseImage->uncompress(), filename);
  }

  return false;
}

bool SavePPM(const Image &image, const std::string &filename) {
  int totalPixels = image.getWidth() * image.getHeight();

  if ((image.getRegionBegin() == 0) && (image.getRegionEnd() == totalPixels)) {
    return doSavePPM(image, filename);
  }

  // If we only have a region of the image, blend it to a clear image to fill
  // it to its width and height.
  std::unique_ptr<Image> blankImage = image.createNew(0, totalPixels);
  blankImage->clear();
  return doSavePPM(*image.blend(*blankImage), filename);
}
