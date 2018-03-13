// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include <Common/MainLoop.hpp>
#include "IceTBase.hpp"

#include <algorithm>
#include <vector>

int main(int argc, char *argv[]) {
  IceTBase compositor;

  // Suppress compressing images. IceT does that for us (and does not
  // understand the format used by our Image classes).
  std::vector<char *> newargv(argc + 1);
  std::copy(argv, argv + argc, newargv.begin());
  newargv[argc] = "--disable-image-compress";

  return MainLoop(argc + 1, newargv.data(), &compositor);
}
