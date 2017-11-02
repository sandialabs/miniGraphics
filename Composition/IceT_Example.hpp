// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef ICET_EXAMPLE_H
#define ICET_EXAMPLE_H

#include <GL/gl.h>
#include <GL/glut.h>
#include "../Objects/Composition.hpp"

#include <IceT.h>
#include <IceTDevImage.h>
#include <IceTMPI.h>

#include <time.h>
#include <iostream>
using namespace std;

class IceT_Example : public Composition {
 private:
 public:
  void composition(
      int, int, int**, int**, int**, float**, int*, int*, int*, float*);
};
#endif
