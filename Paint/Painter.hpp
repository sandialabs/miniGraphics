// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef PAINTER_H
#define PAINTER_H

#include <miniGraphicsConfig.h>

#include <Objects/Image.hpp>
#include <Objects/Mesh.hpp>

#include <glm/mat4x4.hpp>

#include <vector>

class Painter {
 public:
  virtual void paint(const Mesh& mesh,
                     Image* image,
                     const glm::mat4& modelview,
                     const glm::mat4& projection) = 0;

  virtual ~Painter() = default;
};

#endif  // PAINTER_H
