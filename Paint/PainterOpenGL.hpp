// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef PAINTER_OPENGL_H
#define PAINTER_OPENGL_H

#include "Painter.hpp"

#include <stdio.h>
#include <stdlib.h>

class PainterOpenGL : public Painter {
 private:
  struct Internals;
  Internals* internals;

 public:
  PainterOpenGL();
  ~PainterOpenGL();

  void paint(const Mesh& mesh,
             ImageFull& image,
             const glm::mat4x4& modelview,
             const glm::mat4x4& projection) final;
};

#endif  // PAINTER_OPENGL_H
