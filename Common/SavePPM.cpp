// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "SavePPM.hpp"

#include <fstream>

bool SavePPM(const ImageFull& image, const std::string& filename) {
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
