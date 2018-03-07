// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include <Common/MainLoop.hpp>
#include "DirectSendOverlap.hpp"

int main(int argc, char *argv[]) {
  DirectSendOverlap compositor;
  return MainLoop(argc, argv, &compositor, compositor.getOptionVector());
}
